// Copyright Epic Games, Inc. All Rights Reserved.

#include "EditorBatchGenerationSubsystem.h"
#include "MetaHumanCharacter.h"
#include "MetaHumanBodyType.h"
#include "Misc/DateTime.h"
#include "Containers/Ticker.h"

void UEditorBatchGenerationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Initialized"));
	// Register ticker delegate - this is how we "tick" in the editor!
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UEditorBatchGenerationSubsystem::TickStateMachine),
		CheckIntervalConfig
	);
	UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Tick interval set to %.1f seconds"), CheckIntervalConfig);
}

void UEditorBatchGenerationSubsystem::Deinitialize()
{
	// Stop any running generation
	StopBatchGeneration();

	Super::Deinitialize();
	UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Deinitialized"));
}

// ============================================================================
// Public API
// ============================================================================

void UEditorBatchGenerationSubsystem::StartBatchGeneration(
	bool bLoopMode,
	FString OutputPath,
	EMetaHumanQualityLevel QualityLevel,
	float CheckInterval,
	float LoopDelay)
{
	if (IsRunning())
	{
		UE_LOG(LogTemp, Warning, TEXT("Batch generation already running!"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("=== EditorBatchGenerationSubsystem: Starting Batch Generation ==="));
	UE_LOG(LogTemp, Log, TEXT("  Loop Mode: %s"), bLoopMode ? TEXT("Enabled") : TEXT("Disabled"));
	UE_LOG(LogTemp, Log, TEXT("  Output Path: %s"), *OutputPath);
	UE_LOG(LogTemp, Log, TEXT("  Check Interval: %.1f seconds"), CheckInterval);

	// Store configuration
	bLoopGenerationEnabled = bLoopMode;
	OutputPathConfig = OutputPath;
	QualityLevelConfig = QualityLevel;
	CheckIntervalConfig = CheckInterval;
	LoopDelayConfig = LoopDelay;

	// Reset state
	GeneratedCount = 0;
	LastErrorMessage.Empty();

	// Start state machine
	TransitionToState(EBatchGenState::Preparing);
}

void UEditorBatchGenerationSubsystem::StopBatchGeneration()
{
	if (!IsRunning() && CurrentState != EBatchGenState::Error)
	{
		UE_LOG(LogTemp, Warning, TEXT("No batch generation running"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Stopping batch generation"));

	// Reset state
	TransitionToState(EBatchGenState::Idle);
	GeneratedCharacter.Reset();
	CurrentCharacterName.Empty();
}

FString UEditorBatchGenerationSubsystem::GetCurrentStateString() const
{
	switch (CurrentState)
	{
		case EBatchGenState::Idle: return TEXT("Idle");
		case EBatchGenState::Preparing: return TEXT("Preparing Character");
		case EBatchGenState::WaitingForRig: return TEXT("Waiting for AutoRig");
		case EBatchGenState::Assembling: return TEXT("Assembling Character");
		case EBatchGenState::Complete: return TEXT("Complete");
		case EBatchGenState::Error: return TEXT("Error");
		default: return TEXT("Unknown");
	}
}

void UEditorBatchGenerationSubsystem::GetStatusInfo(EBatchGenState& OutState, FString& OutCharacterName, int32& OutGeneratedCount) const
{
	OutState = CurrentState;
	OutCharacterName = CurrentCharacterName;
	OutGeneratedCount = GeneratedCount;
}

// ============================================================================
// State Machine Implementation
// ============================================================================

bool UEditorBatchGenerationSubsystem::TickStateMachine(float DeltaTime)
{
	// This function is called by FTSTicker periodically
	// It's the equivalent of Tick() but works in the editor!

	if (bShouldProcessState)
	{

		UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Tick, current state: %s)"), *GetCurrentStateString());
		switch (CurrentState)
		{
			case EBatchGenState::Idle:
				HandleIdleState();
				break;
			case EBatchGenState::Preparing:
				HandlePreparingState();
				break;
			case EBatchGenState::WaitingForRig:
				HandleWaitingForRigState();
				break;
			case EBatchGenState::Assembling:
				HandleAssemblingState();
				break;
			case EBatchGenState::Complete:
				HandleCompleteState();
				break;
			case EBatchGenState::Error:
				HandleErrorState();
				break;
		}
	}

	// Handle loop delay timer
	if (CurrentState == EBatchGenState::Complete && bLoopGenerationEnabled && LoopDelayTimer > 0.0f)
	{
		LoopDelayTimer -= DeltaTime;
		if (LoopDelayTimer <= 0.0f)
		{
			UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Loop delay finished, starting new generation"));
			TransitionToState(EBatchGenState::Preparing);
		}
	}

	// Return true to keep ticking
	return true;
}

void UEditorBatchGenerationSubsystem::TransitionToState(EBatchGenState NewState)
{
	if (CurrentState == NewState)
		return;

	UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: State transition: %s -> %s"),
		*GetCurrentStateString(), *UEnum::GetValueAsString(NewState));

	CurrentState = NewState;
	bShouldProcessState = true; // Process immediately on state change
}

void UEditorBatchGenerationSubsystem::HandleIdleState()
{
	// Nothing to do in idle state
}

void UEditorBatchGenerationSubsystem::HandlePreparingState()
{
	UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: === Starting Character Preparation ==="));

	FMetaHumanBodyParametricConfig BodyConfig;
	FMetaHumanAppearanceConfig AppearanceConfig;
	GenerateRandomCharacterConfigs(BodyConfig, AppearanceConfig, CurrentCharacterName);

	UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Character Name: %s"), *CurrentCharacterName);
	UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Body Type: %s"), *UEnum::GetValueAsString(BodyConfig.BodyType));
	UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Output Path: %s"), *OutputPathConfig);

	UMetaHumanCharacter* Character = nullptr;
	bool bSuccess = UMetaHumanParametricGenerator::PrepareAndRigCharacter(
		CurrentCharacterName,
		OutputPathConfig,
		BodyConfig,
		AppearanceConfig,
		Character
	);

	if (bSuccess && Character)
	{
		GeneratedCharacter = Character;
		UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: ✓ Preparation complete, AutoRig started"));
		UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Transitioning to WaitingForRig state"));
		UMetaHumanParametricGenerator::DownloadTextureSourceData(Character);
		TransitionToState(EBatchGenState::WaitingForRig);
	}
	else
	{
		LastErrorMessage = TEXT("Failed to prepare character or start AutoRig");
		UE_LOG(LogTemp, Error, TEXT("EditorBatchGenerationSubsystem: ✗ %s"), *LastErrorMessage);
		TransitionToState(EBatchGenState::Error);
	}
}

void UEditorBatchGenerationSubsystem::HandleWaitingForRigState()
{
	if (!GeneratedCharacter.IsValid())
	{
		LastErrorMessage = TEXT("Character reference lost while waiting for rig");
		UE_LOG(LogTemp, Error, TEXT("EditorBatchGenerationSubsystem: ✗ %s"), *LastErrorMessage);
		TransitionToState(EBatchGenState::Error);
		return;
	}

	// Check rigging status
	FString RigStatus = UMetaHumanParametricGenerator::GetRiggingStatusString(GeneratedCharacter.Get());

	// Log status periodically
	UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Checking rig status... %s"), *RigStatus);

	// Check if rigged
	if (RigStatus.Contains(TEXT("Rigged")))
	{
		UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: ✓ AutoRig complete!"));

		UMetaHumanCharacterEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();
		if (!EditorSubsystem)
		{
			UE_LOG(LogTemp, Error, TEXT("EditorBatchGenerationSubsystem: Failed to get MetaHumanCharacterEditorSubsystem"));
			LastErrorMessage = TEXT("Failed to get MetaHumanCharacterEditorSubsystem");
			TransitionToState(EBatchGenState::Error);
		}
		if (!EditorSubsystem->IsRequestingHighResolutionTextures(GeneratedCharacter.Get()))
		{
			TransitionToState(EBatchGenState::Assembling);
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: ✓ AutoRig complete! Downloading Texture."));
		}
	}
	else if (RigStatus.Contains(TEXT("Unrigged")) && !RigStatus.Contains(TEXT("RigPending")))
	{
		// If it went back to Unrigged (not RigPending), that means it failed
		LastErrorMessage = TEXT("AutoRig failed - character is unrigged");
		UE_LOG(LogTemp, Error, TEXT("EditorBatchGenerationSubsystem: ✗ %s"), *LastErrorMessage);
		TransitionToState(EBatchGenState::Error);
	}
	// Otherwise, still waiting (RigPending) - will check again on next tick
	UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: AutoRig is still pending... %s"), *RigStatus);
}

void UEditorBatchGenerationSubsystem::HandleAssemblingState()
{
	if (!GeneratedCharacter.IsValid())
	{
		LastErrorMessage = TEXT("Character reference lost during assembly");
		UE_LOG(LogTemp, Error, TEXT("EditorBatchGenerationSubsystem: ✗ %s"), *LastErrorMessage);
		TransitionToState(EBatchGenState::Error);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: === Starting Character Assembly ==="));

	// Call Step 2: Assemble
	bool bSuccess = UMetaHumanParametricGenerator::AssembleCharacter(
		GeneratedCharacter.Get(),
		OutputPathConfig,
		QualityLevelConfig
	);

	if (bSuccess)
	{
		GeneratedCount++;
		UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: ✓✓✓ Character generation complete! ✓✓✓"));
		UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Character '%s' saved to %s"), *CurrentCharacterName, *OutputPathConfig);
		UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Total characters generated: %d"), GeneratedCount);
		TransitionToState(EBatchGenState::Complete);
	}
	else
	{
		LastErrorMessage = TEXT("Failed to assemble character");
		UE_LOG(LogTemp, Error, TEXT("EditorBatchGenerationSubsystem: ✗ %s"), *LastErrorMessage);
		TransitionToState(EBatchGenState::Error);
	}
}

void UEditorBatchGenerationSubsystem::HandleCompleteState()
{
	// Log completion (only once per character generation)
	UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: === Generation Complete ==="));

	if (bLoopGenerationEnabled)
	{
		UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Loop mode enabled - will start next character in %.1f seconds"), LoopDelayConfig);
		LoopDelayTimer = LoopDelayConfig;
	}
}

void UEditorBatchGenerationSubsystem::HandleErrorState()
{
	// Log error state
	UE_LOG(LogTemp, Error, TEXT("EditorBatchGenerationSubsystem: === Error State ==="));
	UE_LOG(LogTemp, Error, TEXT("EditorBatchGenerationSubsystem: Error: %s"), *LastErrorMessage);
	UE_LOG(LogTemp, Error, TEXT("EditorBatchGenerationSubsystem: Call StopBatchGeneration() or StartBatchGeneration() to retry"));
}

// ============================================================================
// Random Parameter Generation
// ============================================================================

void UEditorBatchGenerationSubsystem::GenerateRandomCharacterConfigs(
	FMetaHumanBodyParametricConfig& OutBodyConfig,
	FMetaHumanAppearanceConfig& OutAppearanceConfig,
	FString& OutCharacterName)
{
	auto RandomChoice = [](const TArray<FString>& Array) -> FString
	{
		if (Array.Num() == 0)
		{
			UE_LOG(LogTemp, Error, TEXT("EditorBatchGenerationSubsystem: RandomChoice: Array is empty"));
			return FString("");
		}
		int32 index = FMath::RandRange(0, Array.Num() - 1);
		UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: RandomChoice: Selected %s from Array"), *Array[index]);
		return Array[index];
	};
	int32 RandomBodyTypeIndex = FMath::RandRange(0, 17);
	OutBodyConfig.BodyType = static_cast<EMetaHumanBodyType>(RandomBodyTypeIndex);
	// OutBodyConfig.BodyType = EMetaHumanBodyType::BlendableBody;

	OutBodyConfig.GlobalDeltaScale = 1.0f;
	OutBodyConfig.bUseParametricBody = true;
	OutBodyConfig.BodyMeasurements.Empty();

	float MasculineFeminine = FMath::FRandRange(-1.5f, 1.5f);
	bool bIsFemale = true;
	if (MasculineFeminine < 0.0f) 
	{
		bIsFemale = false;
	}
	OutBodyConfig.BodyMeasurements.Add(TEXT("Masculine/Feminine"), MasculineFeminine);
	OutBodyConfig.BodyMeasurements.Add(TEXT("Muscularity"), FMath::FRandRange(-1.5f, 1.5f));
	OutBodyConfig.BodyMeasurements.Add(TEXT("Fat"), FMath::FRandRange(-0.5f, 1.0f));
	OutBodyConfig.BodyMeasurements.Add(TEXT("Height"), FMath::FRandRange(150.0f, 185.0f));

	OutBodyConfig.QualityLevel = QualityLevelConfig;

	int32 EthnicityRoll = FMath::RandRange(1, 100);
	FString EthnicityCode;

	if (EthnicityRoll <= 90)
	{
		OutAppearanceConfig.SkinSettings.Skin.U = FMath::FRandRange(0.25f, 0.4f);
		OutAppearanceConfig.SkinSettings.Skin.V = FMath::FRandRange(0.0f, 0.3f);

		OutAppearanceConfig.EyesSettings.EyeLeft.Iris.PrimaryColorU = 0.0f;
		OutAppearanceConfig.EyesSettings.EyeLeft.Iris.PrimaryColorV = 0.0f;
		OutAppearanceConfig.EyesSettings.EyeLeft.Iris.SecondaryColorU = 0.0f;
		OutAppearanceConfig.EyesSettings.EyeLeft.Iris.SecondaryColorV = 0.0f;

		OutAppearanceConfig.EyesSettings.EyeRight.Iris.PrimaryColorU = 0.0f;
		OutAppearanceConfig.EyesSettings.EyeRight.Iris.PrimaryColorV = 0.0f;
		OutAppearanceConfig.EyesSettings.EyeRight.Iris.SecondaryColorU = 0.0f;
		OutAppearanceConfig.EyesSettings.EyeRight.Iris.SecondaryColorV = 0.0f;

		OutAppearanceConfig.WardrobeConfig.HairParameters->Melanin = 1.0f;
		EthnicityCode = TEXT("AS"); // Asian
	}
	else if (EthnicityRoll <= 95)
	{
		OutAppearanceConfig.SkinSettings.Skin.U = FMath::FRandRange(0.0f, 0.2f);
		OutAppearanceConfig.SkinSettings.Skin.V = FMath::FRandRange(0.4f, 1.0f);
		EthnicityCode = TEXT("AF"); // African
	}
	else
	{
		OutAppearanceConfig.SkinSettings.Skin.U = FMath::FRandRange(0.6f, 1.0f);
		OutAppearanceConfig.SkinSettings.Skin.V = FMath::FRandRange(0.0f, 1.0f);
		EthnicityCode = TEXT("EU"); // European
	}

	// OutAppearanceConfig.WardrobeConfig.HairParameters->Redness = FMath::FRandRange(0.0f, 1.0f);
	OutAppearanceConfig.WardrobeConfig.HairParameters->Roughness = FMath::FRandRange(0.0f, 1.0f);
	OutAppearanceConfig.WardrobeConfig.HairParameters->Whiteness = FMath::FRandRange(0.0f, 1.0f);
	OutAppearanceConfig.WardrobeConfig.HairParameters->Lightness = FMath::FRandRange(0.0f, 1.0f);

	// Randomize wardrobe colors
	OutAppearanceConfig.WardrobeConfig.ColorConfig.PrimaryColorShirt = FLinearColor(
		FMath::FRandRange(0.0f, 1.0f),  // R
		FMath::FRandRange(0.0f, 1.0f),  // G
		FMath::FRandRange(0.0f, 1.0f),  // B
		1.0f  // A
	);
	OutAppearanceConfig.WardrobeConfig.ColorConfig.PrimaryColorShort = FLinearColor(
		FMath::FRandRange(0.0f, 1.0f),  // R
		FMath::FRandRange(0.0f, 1.0f),  // G
		FMath::FRandRange(0.0f, 1.0f),  // B
		1.0f  // A
	);

	OutAppearanceConfig.SkinSettings.Skin.Roughness = FMath::FRandRange(0.0f, 1.0f);
	OutAppearanceConfig.SkinSettings.Skin.bShowTopUnderwear = true;
	OutAppearanceConfig.SkinSettings.Skin.BodyTextureIndex = FMath::RandRange(0, 8);
	// see I:\UE_5.6\Engine\Plugins\MetaHuman\MetaHumanCharacter\Content\Optional\TextureSynthesis\TS-1.3-D_UE_res-1024_nchr-153\texture_attributes.json
	//a['attributes']['Face Stubble']['values']          
	// [0, 3, 1, 1, 2, 0, 3, 0, 3, 1, 0, 1, 0, 2, 0, 3, 0, 0, 0, 1, 0, 2, 0, 2, 0, 2, 0, 2, 2, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 
    //  0, 1, 0, 1, 1, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 1, 2, 0, 0, 1, 0, 3, 0, 2, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 0, 2, 2, 0, 0, 2, 0, 0, 3, 0, 0, 0, 2, 0, 3, 2, 0, 0, 0, 1, 0, 3, 0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 1, 0, 2, 2, 2, 0, 3, 0, 0, 1, 0, 2, 1, 0, 1, 1, 0, 2, 0, 1, 0, 3]
	// 0-1 for female, 0-3 for male
	const TArray<int32> FaceTextureStubbleMapp = {0, 3, 1, 1, 2, 0, 3, 0, 3, 1, 0, 1, 0, 2, 0, 3, 0, 0, 0, 1, 0, 2, 0, 2, 0, 2, 0, 2, 2, 0, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 2, 0, 0, 2, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 1, 2, 0, 0, 1, 0, 3, 0, 2, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 0, 2, 2, 0, 0, 2, 0, 0, 3, 0, 0, 0, 2, 0, 3, 2, 0, 0, 0, 1, 0, 3, 0, 0, 0, 0, 0, 0, 2, 1, 0, 0, 0, 0, 0, 1, 0, 2, 2, 2, 0, 3, 0, 0, 1, 0, 2, 1, 0, 1, 1, 0, 2, 0, 1, 0, 3};
	TArray<int32> FemaleFaceTextureIndexSet;
	TArray<int32> MaleFaceTextureIndexSet;
	for (int32 Index = 0; Index < FaceTextureStubbleMapp.Num(); ++Index)
	{
		int32 StubbleValue = FaceTextureStubbleMapp[Index];
		// if (StubbleValue >= 0 && StubbleValue <= 1)
		if (StubbleValue >= 0 && StubbleValue <= 0)
		{
			FemaleFaceTextureIndexSet.Add(Index);
		}
		if (StubbleValue >= 0 && StubbleValue <= 3)
		{
			MaleFaceTextureIndexSet.Add(Index);
		}
	}
	int32 RandomIndex;
	if (bIsFemale)
	{
		RandomIndex = FemaleFaceTextureIndexSet[FMath::RandRange(0, FemaleFaceTextureIndexSet.Num() - 1)];
	}
	else
	{
		RandomIndex = MaleFaceTextureIndexSet[FMath::RandRange(0, MaleFaceTextureIndexSet.Num() - 1)];
	}
	OutAppearanceConfig.SkinSettings.Skin.FaceTextureIndex = RandomIndex;
	// OutAppearanceConfig.SkinSettings.Skin.FaceTextureIndex = FMath::RandRange(0, 152);

	if (EthnicityCode == TEXT("AS")) // Asian
	{
		OutAppearanceConfig.SkinSettings.Freckles.Density = FMath::FRandRange(0.0f, 0.5f);
		OutAppearanceConfig.SkinSettings.Freckles.Strength = FMath::FRandRange(0.0f, 0.5f);
	}
	else
	{
		OutAppearanceConfig.SkinSettings.Freckles.Density = FMath::FRandRange(0.0f, 1.0f);
		OutAppearanceConfig.SkinSettings.Freckles.Strength = FMath::FRandRange(0.0f, 1.0f);
	}
	OutAppearanceConfig.SkinSettings.Freckles.Saturation = FMath::FRandRange(0.0f, 1.0f);
	OutAppearanceConfig.SkinSettings.Freckles.ToneShift = FMath::FRandRange(0.0f, 1.0f);

	int32 FrecklesRoll = FMath::RandRange(1, 100);
	if (FrecklesRoll <= 70)
	{
		OutAppearanceConfig.SkinSettings.Freckles.Mask = EMetaHumanCharacterFrecklesMask::None;
	}
	else
	{
		OutAppearanceConfig.SkinSettings.Freckles.Mask = static_cast<EMetaHumanCharacterFrecklesMask>(FMath::RandRange(1, 3));
	}

	OutAppearanceConfig.HeadModelSettings.Eyelashes.bEnableGrooms = false;

	// {
	// 	// random hair from UE Metahuman Plugin Content
	// 	FString BaseHairPath = TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair");
	// 	FString RandomHairItem = UMetaHumanParametricGenerator::GetRandomWardrobeItemFromPath(TEXT("Hair"), BaseHairPath);
	// 	OutAppearanceConfig.WardrobeConfig.HairPath = RandomHairItem;
	// 	UE_LOG(LogTemp, Log, TEXT("Generated random hair item: %s"), *RandomHairItem);
	// }
	{
		// Random hair from predefined list instead of MetaHuman plugin
		// TArray<FString> AllHairPaths = {
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_UpdoBuns.WI_Hair_S_UpdoBuns"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_UpdoBraids.WI_Hair_S_UpdoBraids"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_Updo.WI_Hair_S_Updo"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_SweptUp.WI_Hair_S_SweptUp"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_SlickBack.WI_Hair_S_SlickBack"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_SideSweptFringe.WI_Hair_S_SideSweptFringe"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_RecedeMessy.WI_Hair_S_RecedeMessy"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_PulledBack.WI_Hair_S_PulledBack"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_Pixie.WI_Hair_S_Pixie"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_Messy.WI_Hair_S_Messy"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_LowPonytail.WI_Hair_S_LowPonytail"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_HairLoss.WI_Hair_S_HairLoss"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_CurlyFade.WI_Hair_S_CurlyFade"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_Cornrows.WI_Hair_S_Cornrows"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_CoilBuzzCut.WI_Hair_S_CoilBuzzCut"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_Coil.WI_Hair_S_Coil"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_Clean.WI_Hair_S_Clean"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_Casual.WI_Hair_S_Casual"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_BuzzCut.WI_Hair_S_BuzzCut"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_BrushCut.WI_Hair_S_BrushCut"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_BobLayered.WI_Hair_S_BobLayered"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_BaldingStubble.WI_Hair_S_BaldingStubble"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_AfroFade.WI_Hair_S_AfroFade"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_360Waves.WI_Hair_S_360Waves"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_TwistedBraids.WI_Hair_M_TwistedBraids"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_SideSweptFringe.WI_Hair_M_SideSweptFringe"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_Mohawk.WI_Hair_M_Mohawk"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_Layered.WI_Hair_M_Layered"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_FauxMohawk.WI_Hair_M_FauxMohawk"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_BobStraight.WI_Hair_M_BobStraight"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_BobSlick.WI_Hair_M_BobSlick"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_BobMessy.WI_Hair_M_BobMessy"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_BobCurly.WI_Hair_M_BobCurly"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_BobBangs.WI_Hair_M_BobBangs"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_L_StraightBangs.WI_Hair_L_StraightBangs"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_L_Straight.WI_Hair_L_Straight"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_L_MessyClumps.WI_Hair_L_MessyClumps"),
		// 	TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_L_AfroCurly.WI_Hair_L_AfroCurly")
		// };

		TArray<FString> MaleHairPaths = {
			// 短发
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_SlickBack.WI_Hair_S_SlickBack"),
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_SweptUp.WI_Hair_S_SweptUp"),
			// TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_PulledBack.WI_Hair_S_PulledBack"), //狂怒 男主发型
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_Messy.WI_Hair_S_Messy"),
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_HairLoss.WI_Hair_S_HairLoss"),
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_CurlyFade.WI_Hair_S_CurlyFade"),  // 短卷
			// TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_CoilBuzzCut.WI_Hair_S_CoilBuzzCut"), //
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_BuzzCut.WI_Hair_S_BuzzCut"),
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_BrushCut.WI_Hair_S_BrushCut"),
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_Clean.WI_Hair_S_Clean"),
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_360Waves.WI_Hair_S_360Waves"),  // 短寸
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_Casual.WI_Hair_S_Casual"),  // 商务短发
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_Coil.WI_Hair_S_Coil"),

			// 中短发 
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_Pixie.WI_Hair_S_Pixie"),  // 类似碎盖 带刘海
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_SideSweptFringe.WI_Hair_S_SideSweptFringe"),  // 普通三七分


			// TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_RecedeMessy.WI_Hair_S_RecedeMessy"),  // 秃
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_BaldingStubble.WI_Hair_S_BaldingStubble"), // 更秃
			// TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_AfroFade.WI_Hair_S_AfroFade"),  // 短蓬松卷,
			
			// TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_Mohawk.WI_Hair_M_Mohawk"), // cyber phonk 发型 
			// TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_FauxMohawk.WI_Hair_M_FauxMohawk"), // cyber phonk 发型
			
		};
		TArray<FString> FemaleHairPaths = {
			// 中长发
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_LowPonytail.WI_Hair_S_LowPonytail"), // 类似学生头
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_L_StraightBangs.WI_Hair_L_StraightBangs"),
			// TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_L_Straight.WI_Hair_L_Straight"),  // 容易看起来像西方人
			TEXT("/Game/MHPKG/hair_l_highponytail/WI_Hair_L_HighPonytail.WI_Hair_L_HighPonytail"),

			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_UpdoBuns.WI_Hair_S_UpdoBuns"), // 樱桃 短扎
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_UpdoBraids.WI_Hair_S_UpdoBraids"),  // 樱桃 短扎
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_Updo.WI_Hair_S_Updo"), // 樱桃 短扎
			// TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_Layered.WI_Hair_M_Layered")  // 西方男生微卷到肩

			// bob 短发系列
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_BobStraight.WI_Hair_M_BobStraight"), // 直发蘑菇头
			// TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_BobSlick.WI_Hair_M_BobSlick"),
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_BobMessy.WI_Hair_M_BobMessy"),  // 
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_BobCurly.WI_Hair_M_BobCurly"),  // 到肩 微卷
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_BobBangs.WI_Hair_M_BobBangs"),  // 到颈 哆啦/盖茨比Daisy头
			// TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_BobLayered.WI_Hair_S_BobLayered")  // 到颈 微卷

			// TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_TwistedBraids.WI_Hair_M_TwistedBraids"), // 脏辫
	
			// TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_L_MessyClumps.WI_Hair_L_MessyClumps"),  // 指环王精灵女王发型
			// TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_L_AfroCurly.WI_Hair_L_AfroCurly") //爆炸头
		};
		TArray<FString> UnisexHairPaths = {
			
			
			
			// TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_S_Cornrows.WI_Hair_S_Cornrows"), // 脏辫背头
			TEXT("/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_M_SideSweptFringe.WI_Hair_M_SideSweptFringe"),  // 颈部长度 三七分 颈后微卷
			
			
		};
		TArray<FString> FinalHairPaths;
		if (bIsFemale)
		{
			FinalHairPaths = FemaleHairPaths;
			FinalHairPaths.Append(UnisexHairPaths);
		}
		else
		{
			FinalHairPaths = MaleHairPaths;
			FinalHairPaths.Append(UnisexHairPaths);
		}

		int32 Index = FMath::RandRange(0, FinalHairPaths.Num() - 1);
		OutAppearanceConfig.WardrobeConfig.HairPath = FinalHairPaths[Index];
		UE_LOG(LogTemp, Log, TEXT("Generated random hair item: %s"), *FinalHairPaths[Index]);
	}



	// {
	// 	// random clothing from UE Metahuman Plugin Content
	// 	FString BaseClothingPath = TEXT("/MetaHumanCharacter/Optional/Clothing");
	// 	// Get random clothing item
	// 	FString RandomClothingItem = UMetaHumanParametricGenerator::GetRandomWardrobeItemFromPath(TEXT("Outfits"), BaseClothingPath);
	// 	OutAppearanceConfig.WardrobeConfig.ClothingPaths.Empty();
	// 	OutAppearanceConfig.WardrobeConfig.ClothingPaths.Add(RandomClothingItem);
	// 	UE_LOG(LogTemp, Log, TEXT("Generated random clothing item: %s"), *RandomClothingItem);
	// }
	{
		int32 Roll;
		OutAppearanceConfig.WardrobeConfig.ClothingPaths.Empty();
		

		const TArray<FString> UpperAndLowerCloth = {
			TEXT("/MetaHumanCharacter/Optional/Clothing/WI_DefaultGarment.WI_DefaultGarment")
		};
		const TArray<FString> UpperCloth = {
			// New Ones
			"/Game/GoodWI/Upper/WI_Puffer_Jacket.WI_Puffer_Jacket", //
			// "/Game/GoodWI/Upper/WI_Shirts.WI_Shirts",  //下摆太长，容易穿模
			"/Game/GoodWI/Upper/WI_Sweater.WI_Sweater",
			"/Game/GoodWI/Upper/WI_Tank_Top.WI_Tank_Top",
			"/Game/GoodWI/Upper/WI_Track_Suit.WI_Track_Suit",


			"/Game/GoodWI/Upper/WI_Red_Shirt.WI_Red_Shirt",
			"/Game/GoodWI/Upper/WI_SweaterNew.WI_SweaterNew",
		};
		const TArray<FString> LowerCloth = {
			// New Ones
			"/Game/GoodWI/Lower/WI_Bonkers.WI_Bonkers",
			"/Game/GoodWI/Lower/WI_Cargo.WI_Cargo",
			"/Game/GoodWI/Lower/WI_Jeans.WI_Jeans",
			"/Game/GoodWI/Lower/WI_Pant.WI_Pant",  // Warning: this may cause collision with UpperCloth
			"/Game/GoodWI/Lower/WI_Track_Pant.WI_Track_Pant",

			"/Game/GoodWI/Lower/WI_Baggy_Pants.WI_Baggy_Pants",
			"/Game/GoodWI/Lower/WI_Cyber_Punk_Pants.WI_Cyber_Punk_Pants",
			"/Game/GoodWI/Lower/WI_Jeans2.WI_Jeans2",
			"/Game/GoodWI/Lower/WI_Jeans_1.WI_Jeans_1",
			"/Game/GoodWI/Lower/WI_Jeans_3.WI_Jeans_3",
			"/Game/GoodWI/Lower/WI_Colorful_Sweats.WI_Colorful_Sweats",
		};

		const TArray<FString> Shoes = {
			"/Game/GoodWI/Shoes/WI_Short_Boots.WI_Short_Boots"
		};

		const TArray<FString> FullSuit = { };

		const TArray<FString> OtherItems = {
			"/Game/GoodWI/OtherItems/WI_Bag.WI_Bag"
		};
		
		Roll = FMath::RandRange(1, 100);
		if (Roll <= 20) // use UpperAndLowerCloth
		{
		 	OutAppearanceConfig.WardrobeConfig.ClothingPaths.Add(RandomChoice(UpperAndLowerCloth));
		}
		else
		{
			OutAppearanceConfig.WardrobeConfig.ClothingPaths.Add(RandomChoice(UpperCloth));
			OutAppearanceConfig.WardrobeConfig.ClothingPaths.Add(RandomChoice(LowerCloth));
		}

		
		// Shoes
		Roll = FMath::RandRange(1, 100);
		if (Roll <= 15) // Do not wear shoes
		{
			// pass
		}
		else
		{
			OutAppearanceConfig.WardrobeConfig.ClothingPaths.Add(RandomChoice(Shoes));
		}
		

		// OtherItems
		Roll = FMath::RandRange(1, 100);
		if (Roll <= 10) // wear other items
		{
			OutAppearanceConfig.WardrobeConfig.ClothingPaths.Add(RandomChoice(OtherItems));
		}

	}

	// Generate character name based on ethnicity and gender
	FDateTime Now = FDateTime::Now();
	FString GenderCode = bIsFemale ? TEXT("F") : TEXT("M");

	OutCharacterName = FString::Printf(TEXT("%s-%s-BatchGen-%02d%02d_%02d%02d%02d"),
		*EthnicityCode,
		*GenderCode,
		Now.GetMonth(), Now.GetDay(),
		Now.GetHour(), Now.GetMinute(), Now.GetSecond());

	// Log the character info for debugging
	UE_LOG(LogTemp, Log, TEXT("Generated character: %s (Ethnicity: %s, Gender: %s)"),
		*OutCharacterName, *EthnicityCode, bIsFemale ? TEXT("Female") : TEXT("Male"));
}


