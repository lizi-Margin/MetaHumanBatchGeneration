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

	// Remove ticker delegate
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}

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

	// Generate random parameters
	FMetaHumanBodyParametricConfig BodyConfig = GenerateRandomBodyConfig();
	FMetaHumanAppearanceConfig AppearanceConfig = GenerateRandomAppearanceConfig();
	CurrentCharacterName = GenerateUniqueCharacterName();

	UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Character Name: %s"), *CurrentCharacterName);
	UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Body Type: %s"), *UEnum::GetValueAsString(BodyConfig.BodyType));
	UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Output Path: %s"), *OutputPathConfig);

	// Call Step 1: Prepare and Rig
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
		UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: ✓ AutoRig complete! Proceeding to assembly"));
		TransitionToState(EBatchGenState::Assembling);
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
	// Log completion (only once)
	static bool bLoggedCompletion = false;
	if (!bLoggedCompletion)
	{
		UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: === Generation Complete ==="));

		if (bLoopGenerationEnabled)
		{
			UE_LOG(LogTemp, Log, TEXT("EditorBatchGenerationSubsystem: Loop mode enabled - will start next character in %.1f seconds"), LoopDelayConfig);
			LoopDelayTimer = LoopDelayConfig;
		}

		bLoggedCompletion = true;
	}

	// Reset flag when leaving complete state
	if (CurrentState != EBatchGenState::Complete)
	{
		bLoggedCompletion = false;
	}
}

void UEditorBatchGenerationSubsystem::HandleErrorState()
{
	// Log error (only once)
	static bool bLoggedError = false;
	if (!bLoggedError)
	{
		UE_LOG(LogTemp, Error, TEXT("EditorBatchGenerationSubsystem: === Error State ==="));
		UE_LOG(LogTemp, Error, TEXT("EditorBatchGenerationSubsystem: Error: %s"), *LastErrorMessage);
		UE_LOG(LogTemp, Error, TEXT("EditorBatchGenerationSubsystem: Call StopBatchGeneration() or StartBatchGeneration() to retry"));
		bLoggedError = true;
	}

	// Reset flag when leaving error state
	if (CurrentState != EBatchGenState::Error)
	{
		bLoggedError = false;
	}
}

// ============================================================================
// Random Parameter Generation
// ============================================================================

FMetaHumanBodyParametricConfig UEditorBatchGenerationSubsystem::GenerateRandomBodyConfig()
{
	FMetaHumanBodyParametricConfig Config;

	// Randomly select body type
	int32 RandomBodyTypeIndex = FMath::RandRange(0, 17); // 18 body types (0-17)
	Config.BodyType = static_cast<EMetaHumanBodyType>(RandomBodyTypeIndex);

	// Global delta scale (usually keep at 1.0 for full effect)
	Config.GlobalDeltaScale = 1.0f;

	// Enable parametric body
	Config.bUseParametricBody = true;

	// Generate random body measurements
	Config.BodyMeasurements.Empty();
	Config.BodyMeasurements.Add(TEXT("Height"), FMath::FRandRange(150.0f, 195.0f));
	Config.BodyMeasurements.Add(TEXT("Chest"), FMath::FRandRange(75.0f, 120.0f));
	Config.BodyMeasurements.Add(TEXT("Waist"), FMath::FRandRange(60.0f, 100.0f));
	Config.BodyMeasurements.Add(TEXT("Hips"), FMath::FRandRange(80.0f, 120.0f));
	Config.BodyMeasurements.Add(TEXT("ShoulderWidth"), FMath::FRandRange(35.0f, 55.0f));
	Config.BodyMeasurements.Add(TEXT("ArmLength"), FMath::FRandRange(55.0f, 75.0f));
	Config.BodyMeasurements.Add(TEXT("LegLength"), FMath::FRandRange(75.0f, 105.0f));

	// Quality level
	Config.QualityLevel = QualityLevelConfig;

	return Config;
}

FMetaHumanAppearanceConfig UEditorBatchGenerationSubsystem::GenerateRandomAppearanceConfig()
{
	FMetaHumanAppearanceConfig Config;
	return Config;

	// // Random skin tone (UV coordinates in 0-1 range)
	// Config.SkinToneU = FMath::FRandRange(0.0f, 1.0f);
	// Config.SkinToneV = FMath::FRandRange(0.0f, 1.0f);

	// // Skin roughness (0.5 - 1.5 is a reasonable range)
	// Config.SkinRoughness = FMath::FRandRange(0.0f, 2.0f);

	// // Random iris pattern (there are multiple Iris patterns, let's use a random enum value)
	// int32 RandomIrisPattern = FMath::RandRange(0, 9);   // !!!
	// Config.IrisPattern = static_cast<EMetaHumanCharacterEyesIrisPattern>(RandomIrisPattern);

	// // Random iris color
	// Config.IrisPrimaryColorU = FMath::FRandRange(0.0f, 1.0f);
	// Config.IrisPrimaryColorV = FMath::FRandRange(0.0f, 1.0f);

	// // Random eyelashes type
	// int32 RandomEyelashType = FMath::RandRange(0, 6);  // !!!
	// Config.EyelashesType = static_cast<EMetaHumanCharacterEyelashesType>(RandomEyelashType);

	// // Enable eyelash grooms (randomly)
	// Config.bEnableEyelashGrooms = FMath::RandBool();

	// return Config;
}

FString UEditorBatchGenerationSubsystem::GenerateUniqueCharacterName()
{
	FDateTime Now = FDateTime::Now();

	// Format: RandomChar_MMDD_HHMMSS
	FString Name = FString::Printf(TEXT("RandomChar_%02d%02d_%02d%02d%02d"),
		Now.GetMonth(), Now.GetDay(),
		Now.GetHour(), Now.GetMinute(), Now.GetSecond());

	return Name;
}
