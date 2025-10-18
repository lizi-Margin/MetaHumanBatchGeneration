// Copyright Epic Games, Inc. All Rights Reserved.
// MetaHuman Parametric Generator - Implementation
//
// Complete parametric MetaHuman character generation workflow implementation

#include "MetaHumanParametricGenerator.h"

#include "MetaHumanCharacter.h"
#include "MetaHumanCharacterEditorSubsystem.h"
#include "MetaHumanCharacterBodyIdentity.h"
#include "MetaHumanCollection.h"
#include "MetaHumanBodyType.h"
#include "MetaHumanAssetIOUtility.h"
#include "MetaHumanAssemblyPipelineManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/SkeletalMeshComponent.h"
#include "HAL/PlatformProcess.h"
#include "Tasks/Task.h"
#include "Misc/DateTime.h"
#include "Cloud/MetaHumanARServiceRequest.h"
#include "Cloud/MetaHumanServiceRequest.h"  // For ServiceAuthentication namespace
#include <UObject/UnrealType.h>


UMetaHumanCharacterEditorSubsystem* UMetaHumanParametricGenerator::getEditorSubsystem()
{	
	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = nullptr;
	if (IsInGameThread())
	{
		// EditorSubsystem = GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();
		EditorSubsystem = UMetaHumanCharacterEditorSubsystem::Get();
		return EditorSubsystem;
	}
	else
	{
		// If not in the game thread, we return false directly to let the main thread handle it
		UE_LOG(LogTemp, Warning, TEXT("getEditorSubsystem called from background thread - returning null. MetaHuman operations must run on game thread."));
		return nullptr;
	}
}


// ============================================================================
// Two-Step Generation Workflow - Step 1: Prepare and Rig Character
// ============================================================================

bool UMetaHumanParametricGenerator::PrepareAndRigCharacter(
	const FString& CharacterName,
	const FString& OutputPath,
	const FMetaHumanBodyParametricConfig& BodyConfig,
	const FMetaHumanAppearanceConfig& AppearanceConfig,
	UMetaHumanCharacter*& OutCharacter)
{
	UE_LOG(LogTemp, Log, TEXT("=== Step 1: Prepare and Rig Character ==="));
	UE_LOG(LogTemp, Log, TEXT("Character Name: %s"), *CharacterName);
	UE_LOG(LogTemp, Log, TEXT("Output Path: %s"), *OutputPath);

	// Step 0: Check authentication
	UE_LOG(LogTemp, Log, TEXT("[Step 0/4] Verifying MetaHuman cloud services authentication..."));
	if (!EnsureCloudServicesLogin())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to authenticate with MetaHuman cloud services!"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("[Step 0/4] ✓ Authentication verified"));

	// Step 1: Create base character
	UE_LOG(LogTemp, Log, TEXT("[Step 1/4] Creating base MetaHuman Character asset..."));
	UMetaHumanCharacter* Character = CreateBaseCharacter(
		OutputPath,
		CharacterName,
		EMetaHumanCharacterTemplateType::MetaHuman
	);

	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create base character!"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("[Step 1/4] ✓ Base character created"));

	// Step 2: Configure body and appearance
	UE_LOG(LogTemp, Log, TEXT("[Step 2/4] Configuring body parameters and appearance..."));
	if (!ConfigureBodyParameters(Character, BodyConfig))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to configure body parameters!"));
		return false;
	}

	if (!ConfigureAppearance(Character, AppearanceConfig))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to configure appearance!"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("[Step 2/4] ✓ Configuration complete"));

	// Step 3: Download texture source data
	UE_LOG(LogTemp, Log, TEXT("[Step 3/4] Downloading texture source data..."));
	if (!DownloadTextureSourceData(Character))
	{
		UE_LOG(LogTemp, Warning, TEXT("Warning: Failed to download texture source data"));
		// Continue anyway - might work without high-res textures
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[Step 3/4] ✓ Texture source data downloaded"));
	}

	// Step 4: Start AutoRig (ASYNC - returns immediately!)
	UE_LOG(LogTemp, Log, TEXT("[Step 4/4] Starting AutoRig (async cloud operation)..."));

	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = getEditorSubsystem();
	if (!EditorSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get editor subsystem"));
		return false;
	}

	// Check if already rigged
	EMetaHumanCharacterRigState RigState = EditorSubsystem->GetRiggingState(Character);
	if (RigState == EMetaHumanCharacterRigState::Rigged)
	{
		UE_LOG(LogTemp, Log, TEXT("Character already rigged, ready for assembly"));
		OutCharacter = Character;
		return true;
	}

	// Remove old rig if exists
	check(Character->IsCharacterValid());
	if (Character->HasFaceDNA())
	{
		Character->Modify();
		EditorSubsystem->RemoveFaceRig(Character);
		UE_LOG(LogTemp, Log, TEXT("Removed old face rig"));
	}

	// Start AutoRig (asynchronous - returns immediately!)
	EditorSubsystem->AutoRigFace(Character, UE::MetaHuman::ERigType::JointsAndBlendshapes);

	UE_LOG(LogTemp, Log, TEXT("[Step 4/4] ✓ AutoRig started (running in background)"));
	UE_LOG(LogTemp, Log, TEXT("=== Step 1 Complete - AutoRig is now running in the background ==="));
	UE_LOG(LogTemp, Log, TEXT("Use GetRiggingStatusString() to check progress"));
	UE_LOG(LogTemp, Log, TEXT("When rigged, call AssembleCharacter() to finish"));

	OutCharacter = Character;
	return true;
}

// ============================================================================
// Two-Step Generation Workflow - Step 2: Assemble Character
// ============================================================================

bool UMetaHumanParametricGenerator::AssembleCharacter(
	UMetaHumanCharacter* Character,
	const FString& OutputPath,
	EMetaHumanQualityLevel QualityLevel)
{
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid character for assembly"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("=== Step 2: Assemble Character ==="));
	UE_LOG(LogTemp, Log, TEXT("Character: %s"), *Character->GetName());
	UE_LOG(LogTemp, Log, TEXT("Output Path: %s"), *OutputPath);

	// Check if rigged
	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = getEditorSubsystem();
	if (!EditorSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get editor subsystem"));
		return false;
	}

	EMetaHumanCharacterRigState RigState = EditorSubsystem->GetRiggingState(Character);
	if (RigState != EMetaHumanCharacterRigState::Rigged)
	{
		const TCHAR* StateString = TEXT("Unknown");
		switch (RigState)
		{
			case EMetaHumanCharacterRigState::Unrigged: StateString = TEXT("Unrigged"); break;
			case EMetaHumanCharacterRigState::RigPending: StateString = TEXT("RigPending"); break;
			case EMetaHumanCharacterRigState::Rigged: StateString = TEXT("Rigged"); break;
		}

		UE_LOG(LogTemp, Error, TEXT("Character is not rigged yet! Current state: %s"), StateString);
		UE_LOG(LogTemp, Error, TEXT("Please wait for AutoRig to complete before calling AssembleCharacter()"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("✓ Character is rigged, proceeding with assembly..."));

	// Assemble using native pipeline
	FMetaHumanAssemblyBuildParameters BuildParams =
		UMetaHumanAssemblyPipelineManager::CreateDefaultBuildParameters(
			Character,
			QualityLevel,
			OutputPath
		);

	if (!UMetaHumanAssemblyPipelineManager::BuildMetaHumanCharacter(Character, BuildParams))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to assemble character with native pipeline!"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("✓ Character assembled successfully"));
	UE_LOG(LogTemp, Log, TEXT("  Quality Level: %s"), *UEnum::GetValueAsString(QualityLevel));
	UE_LOG(LogTemp, Log, TEXT("  Output Path: %s"), *OutputPath);
	UE_LOG(LogTemp, Log, TEXT("=== Step 2 Complete - Character is ready! ==="));

	return true;
}

// ============================================================================
// Get Rigging Status
// ============================================================================

FString UMetaHumanParametricGenerator::GetRiggingStatusString(UMetaHumanCharacter* Character)
{
	if (!Character)
	{
		return TEXT("Invalid Character");
	}

	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = getEditorSubsystem();
	if (!EditorSubsystem)
	{
		return TEXT("Error: Cannot get subsystem");
	}

	EMetaHumanCharacterRigState RigState = EditorSubsystem->GetRiggingState(Character);

	switch (RigState)
	{
		case EMetaHumanCharacterRigState::Unrigged:
			return TEXT("Unrigged");
		case EMetaHumanCharacterRigState::RigPending:
			return TEXT("RigPending (AutoRig in progress...)");
		case EMetaHumanCharacterRigState::Rigged:
			return TEXT("Rigged (Ready for assembly!)");
		default:
			return TEXT("Unknown");
	}
}

// ============================================================================
// Step 1: Create Base Character Asset
// ============================================================================

UMetaHumanCharacter* UMetaHumanParametricGenerator::CreateBaseCharacter(
	const FString& PackagePath,
	const FString& CharacterName,
	EMetaHumanCharacterTemplateType TemplateType)
{
	// 1. Build complete package path
	FString PackageNameStr = FPackageName::ObjectPathToPackageName(PackagePath / CharacterName);
	UPackage* Package = CreatePackage(*PackageNameStr);

	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *PackageNameStr);
		return nullptr;
	}

	// 2. Create MetaHumanCharacter object
	UMetaHumanCharacter* Character = NewObject<UMetaHumanCharacter>(
		Package,
		UMetaHumanCharacter::StaticClass(),
		*CharacterName,
		RF_Public | RF_Standalone
	);

	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create MetaHumanCharacter object"));
		return nullptr;
	}

	// 3. Set template type
	Character->TemplateType = TemplateType;

	// 4. Initialize character (load necessary models and data)
	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = getEditorSubsystem();
	if (EditorSubsystem)
	{
		EditorSubsystem->InitializeMetaHumanCharacter(Character);

		// Register character for editing
		if (!EditorSubsystem->IsObjectAddedForEditing(Character)) {
			if (!EditorSubsystem->TryAddObjectToEdit(Character))
			{
				UE_LOG(LogTemp, Warning, TEXT("Failed to register character for editing, but continuing..."));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get MetaHumanCharacterEditorSubsystem"));
		return nullptr;
	}

	// 5. Mark package as dirty (needs saving)
	Package->MarkPackageDirty();

	return Character;
}

// ============================================================================
// Step 2: Configure Body Parameters (Core!)
// ============================================================================

bool UMetaHumanParametricGenerator::ConfigureBodyParameters(
	UMetaHumanCharacter* Character,
	const FMetaHumanBodyParametricConfig& BodyConfig)
{
	if (!Character)
	{
		return false;
	}

	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = getEditorSubsystem();
	if (!EditorSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get editor subsystem"));
		return false;
	}

	// 1. Set body type (fixed vs parametric)
	UE_LOG(LogTemp, Log, TEXT("  - Setting body type: %s"),
		*UEnum::GetValueAsString(BodyConfig.BodyType));

	EditorSubsystem->SetMetaHumanBodyType(
		Character,
		BodyConfig.BodyType,
		UMetaHumanCharacterEditorSubsystem::EBodyMeshUpdateMode::Full
	);

	// 2. Set global deformation strength
	UE_LOG(LogTemp, Log, TEXT("  - Setting global delta scale: %.2f"), BodyConfig.GlobalDeltaScale);
	EditorSubsystem->SetBodyGlobalDeltaScale(Character, BodyConfig.GlobalDeltaScale);

	// 3. If using parametric body, apply body constraints
	if (BodyConfig.bUseParametricBody && BodyConfig.BodyMeasurements.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("  - Applying parametric body constraints (%d measurements)..."),
			BodyConfig.BodyMeasurements.Num());

		// Convert measurements to constraints array
		TArray<FMetaHumanCharacterBodyConstraint> Constraints =
			ConvertMeasurementsToConstraints(BodyConfig.BodyMeasurements);

		// Apply constraints to character
		EditorSubsystem->SetBodyConstraints(Character, Constraints);

		// Print each constraint value
		for (const auto& Pair : BodyConfig.BodyMeasurements)
		{
			UE_LOG(LogTemp, Log, TEXT("    • %s: %.2f cm"), *Pair.Key, Pair.Value);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("  - Using fixed body type (no parametric constraints)"));
	}

	// 4. Get current body state and commit changes
	TSharedRef<FMetaHumanCharacterBodyIdentity::FState> BodyState =
		EditorSubsystem->CopyBodyState(Character);

	EditorSubsystem->CommitBodyState(
		Character,
		BodyState,
		UMetaHumanCharacterEditorSubsystem::EBodyMeshUpdateMode::Full
	);

	UE_LOG(LogTemp, Log, TEXT("  ✓ Body configuration complete"));
	return true;
}

// ============================================================================
// Step 3: Configure Appearance (Skin, Eyes, Eyelashes, etc.)
// ============================================================================

bool UMetaHumanParametricGenerator::ConfigureAppearance(
	UMetaHumanCharacter* Character,
	const FMetaHumanAppearanceConfig& AppearanceConfig)
{
	if (!Character)
	{
		return false;
	}

	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = getEditorSubsystem();
	if (!EditorSubsystem)
	{
		return false;
	}

	// 1. Configure skin settings
	UE_LOG(LogTemp, Log, TEXT("  - Configuring skin settings..."));
	{
		FMetaHumanCharacterSkinSettings SkinSettings;
		SkinSettings.Skin.U = AppearanceConfig.SkinToneU;
		SkinSettings.Skin.V = AppearanceConfig.SkinToneV;
		SkinSettings.Skin.Roughness = AppearanceConfig.SkinRoughness;

		// Apply and commit skin settings
		EditorSubsystem->ApplySkinSettings(Character, SkinSettings);
		EditorSubsystem->CommitSkinSettings(Character, SkinSettings);

		UE_LOG(LogTemp, Log, TEXT("    • Skin Tone: (U=%.2f, V=%.2f)"),
			AppearanceConfig.SkinToneU, AppearanceConfig.SkinToneV);
		UE_LOG(LogTemp, Log, TEXT("    • Roughness: %.2f"), AppearanceConfig.SkinRoughness);
	}

	// 2. Configure eyes settings
	UE_LOG(LogTemp, Log, TEXT("  - Configuring eyes settings..."));
	{
		FMetaHumanCharacterEyesSettings EyesSettings;

		// Use the same settings for left and right eyes (can be set separately)
		EyesSettings.EyeLeft.Iris.IrisPattern = AppearanceConfig.IrisPattern;
		EyesSettings.EyeLeft.Iris.PrimaryColorU = AppearanceConfig.IrisPrimaryColorU;
		EyesSettings.EyeLeft.Iris.PrimaryColorV = AppearanceConfig.IrisPrimaryColorV;

		EyesSettings.EyeRight.Iris.IrisPattern = AppearanceConfig.IrisPattern;
		EyesSettings.EyeRight.Iris.PrimaryColorU = AppearanceConfig.IrisPrimaryColorU;
		EyesSettings.EyeRight.Iris.PrimaryColorV = AppearanceConfig.IrisPrimaryColorV;

		// Apply and commit eyes settings
		EditorSubsystem->ApplyEyesSettings(Character, EyesSettings);
		EditorSubsystem->CommitEyesSettings(Character, EyesSettings);

		UE_LOG(LogTemp, Log, TEXT("    • Iris Pattern: %s"),
			*UEnum::GetValueAsString(AppearanceConfig.IrisPattern));
		UE_LOG(LogTemp, Log, TEXT("    • Iris Color: (U=%.2f, V=%.2f)"),
			AppearanceConfig.IrisPrimaryColorU, AppearanceConfig.IrisPrimaryColorV);
	}

	// 3. Configure head model settings (eyelashes, etc.)
	UE_LOG(LogTemp, Log, TEXT("  - Configuring head model (eyelashes)..."));
	{
		FMetaHumanCharacterHeadModelSettings HeadModelSettings;
		HeadModelSettings.Eyelashes.Type = AppearanceConfig.EyelashesType;
		HeadModelSettings.Eyelashes.bEnableGrooms = AppearanceConfig.bEnableEyelashGrooms;

		// Apply and commit head model settings
		EditorSubsystem->ApplyHeadModelSettings(Character, HeadModelSettings);
		EditorSubsystem->CommitHeadModelSettings(Character, HeadModelSettings);

		UE_LOG(LogTemp, Log, TEXT("    • Eyelashes Type: %s"),
			*UEnum::GetValueAsString(AppearanceConfig.EyelashesType));
		UE_LOG(LogTemp, Log, TEXT("    • Grooms Enabled: %s"),
			AppearanceConfig.bEnableEyelashGrooms ? TEXT("Yes") : TEXT("No"));
	}

	UE_LOG(LogTemp, Log, TEXT("  ✓ Appearance configuration complete"));
	return true;
}

// ============================================================================
// Helper Function: Convert Measurements to Constraints
// ============================================================================

TArray<FMetaHumanCharacterBodyConstraint> UMetaHumanParametricGenerator::ConvertMeasurementsToConstraints(
	const TMap<FString, float>& Measurements)
{
	TArray<FMetaHumanCharacterBodyConstraint> Constraints;

	// Iterate through all measurements and create constraints
	for (const auto& Pair : Measurements)
	{
		FMetaHumanCharacterBodyConstraint Constraint;
		Constraint.Name = FName(*Pair.Key);
		Constraint.bIsActive = true;  // Activate this constraint
		Constraint.TargetMeasurement = Pair.Value;  // Target value (centimeters)

		// Set reasonable min/max range (±50% of target value)
		Constraint.MinMeasurement = Pair.Value * 0.5f;
		Constraint.MaxMeasurement = Pair.Value * 1.5f;

		Constraints.Add(Constraint);
	}

	return Constraints;
}

// ============================================================================
// Added: Download Texture Source Data
// ============================================================================

bool UMetaHumanParametricGenerator::DownloadTextureSourceData(UMetaHumanCharacter* Character)
{
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid character for texture download"));
		return false;
	}

	// Ensure EditorSubsystem is obtained in the game thread
	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = getEditorSubsystem();

	if (!EditorSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get editor subsystem for texture download"));
		return false;
	}
	bool CanDownload =  Character->HasSynthesizedTextures() && \
						!EditorSubsystem->IsRequestingHighResolutionTextures(Character) && \
						EditorSubsystem->IsTextureSynthesisEnabled();
	if (!CanDownload)
	{
		UE_LOG(LogTemp, Warning, TEXT("Character has no synthesized textures, cannot download high-res textures"));
		return false;
	}
	
	if (Character->HasHighResolutionTextures())
	{
		UE_LOG(LogTemp, Log, TEXT("Character already has high-res textures, no need to download"));
		return true;
	}

	return DownloadTextureSourceData_Impl(Character, EditorSubsystem);
}

bool UMetaHumanParametricGenerator::DownloadTextureSourceData_Impl(UMetaHumanCharacter* Character, UMetaHumanCharacterEditorSubsystem* EditorSubsystem)
{
	if (!Character || !EditorSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid parameters for texture download implementation"));
		return false;
	}

	// Check if character has synthesized textures
	if (!Character->HasSynthesizedTextures())
	{
		UE_LOG(LogTemp, Warning, TEXT("Character does not have synthesized textures, cannot download high-res textures"));
		return false;
	}

	// Check if texture synthesis is enabled
	if (!EditorSubsystem->IsTextureSynthesisEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("Texture synthesis is not enabled"));
		return false;
	}

	// Step 2: Request texture download
	UE_LOG(LogTemp, Log, TEXT("Requesting 2k texture download..."));
	EditorSubsystem->RequestHighResolutionTextures(Character, ERequestTextureResolution::Res2k);

	// Wait for download to complete
	const float MaxWaitTime = 120.0f; // Texture download may take more time
	const float StartTime = FPlatformTime::Seconds();
	float LastProgressReport = 0.0f;
	bool bDownloadStarted = false;

	while (EditorSubsystem->IsRequestingHighResolutionTextures(Character))
	{
		const float ElapsedTime = FPlatformTime::Seconds() - StartTime;
		bDownloadStarted = true;

		// Report progress every 10 seconds
		if (ElapsedTime - LastProgressReport > 10.0f)
		{
			UE_LOG(LogTemp, Log, TEXT("Still downloading textures... (%.1f seconds elapsed)"), ElapsedTime);
			UE_LOG(LogTemp, Log, TEXT("  Make sure you're logged into MetaHuman cloud services in the editor"));
			LastProgressReport = ElapsedTime;
		}

		if (ElapsedTime > MaxWaitTime)
		{
			UE_LOG(LogTemp, Warning, TEXT("Texture download timeout after %.1f seconds"), ElapsedTime);
			UE_LOG(LogTemp, Warning, TEXT("  Possible causes:"));
			UE_LOG(LogTemp, Warning, TEXT("  - Not logged into MetaHuman cloud services"));
			UE_LOG(LogTemp, Warning, TEXT("  - Network connectivity issues"));
			UE_LOG(LogTemp, Warning, TEXT("  - Service temporarily unavailable"));
			break;
		}

		// In background thread, no need to manually handle editor tick
		FPlatformProcess::Sleep(1.0f); // Background thread can use longer sleep interval
	}

	const float TotalTime = FPlatformTime::Seconds() - StartTime;

	if (bDownloadStarted && !EditorSubsystem->IsRequestingHighResolutionTextures(Character))
	{
		UE_LOG(LogTemp, Log, TEXT("Texture download completed in %.1f seconds"), TotalTime);
		return true;
	}
	else if (!bDownloadStarted)
	{
		UE_LOG(LogTemp, Warning, TEXT("Texture download is not running - likely it has finished"));
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Texture download did not complete within timeout"));
		return false;
	}
}



// ============================================================================
// Rig Character
// ============================================================================

bool UMetaHumanParametricGenerator::RigCharacter(UMetaHumanCharacter* Character)
{
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid character for rigging"));
		return false;
	}

	// Ensure EditorSubsystem is obtained in the game thread
	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = getEditorSubsystem();

	if (!EditorSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get editor subsystem for rigging"));
		return false;
	}

	// Get current rigging state
	EMetaHumanCharacterRigState RigState = EditorSubsystem->GetRiggingState(Character);

	if (RigState == EMetaHumanCharacterRigState::Rigged)
	{
		UE_LOG(LogTemp, Log, TEXT("Character is already rigged, skipping autorig"));
		return true;
	}

	UE_LOG(LogTemp, Log, TEXT("Character is not rigged, performing autorig..."));

	// See Engine\Plugins\MetaHuman\MetaHumanCharacter\Source\MetaHumanCharacterEditor\Private\MetaHumanCharacterEditorToolkit.cpp line 910 AutoRigFace
	check(Character->IsCharacterValid());
	if (Character->HasFaceDNA())
	{
		Character->Modify();
		EditorSubsystem->RemoveFaceRig(Character);
		UE_LOG(LogTemp, Log, TEXT("Removed old face rig from character"));
	}

	// Setup completion tracking using a delegate
	bool bRiggingComplete = false;
	bool bRiggingSucceeded = false;
	FDelegateHandle RiggingStateChangedHandle;

	// Lambda to track rig completion
	auto OnRiggingStateChangedLambda = [&bRiggingComplete, &bRiggingSucceeded, Character](TNotNull<const UMetaHumanCharacter*> InCharacter, EMetaHumanCharacterRigState NewState)
	{
		if (InCharacter == Character)
		{
			const TCHAR* StateString = TEXT("Unknown");
			switch (NewState)
			{
				case EMetaHumanCharacterRigState::Unrigged: StateString = TEXT("Unrigged"); break;
				case EMetaHumanCharacterRigState::RigPending: StateString = TEXT("RigPending"); break;
				case EMetaHumanCharacterRigState::Rigged: StateString = TEXT("Rigged"); break;
			}
			UE_LOG(LogTemp, Log, TEXT("Rigging state changed: %s"), StateString);

			if (NewState == EMetaHumanCharacterRigState::Rigged)
			{
				bRiggingComplete = true;
				bRiggingSucceeded = true;
				UE_LOG(LogTemp, Log, TEXT("✓ AutoRig completed successfully"));
			}
			else if (NewState == EMetaHumanCharacterRigState::Unrigged)
			{
				// If it goes back to Unrigged after RigPending, that means it failed
				bRiggingComplete = true;
				bRiggingSucceeded = false;
				UE_LOG(LogTemp, Error, TEXT("✗ AutoRig failed (returned to Unrigged state)"));
			}
		}
	};

	// Register delegate BEFORE starting autorig
	RiggingStateChangedHandle = EditorSubsystem->OnRiggingStateChanged.AddLambda(OnRiggingStateChangedLambda);

	// Execute auto-rigging (asynchronous operation)
	UE_LOG(LogTemp, Log, TEXT("Starting autorig (async operation)..."));
	EditorSubsystem->AutoRigFace(Character, UE::MetaHuman::ERigType::JointsAndBlendshapes);

	// Wait for completion with timeout
	// NOTE: We are running in a background thread - delegate callbacks occur on GameThread
	// We just need to wait for the completion flags to be set by the delegate
	const float AutorigStartTime = FPlatformTime::Seconds();
	const float MaxWaitTime = 180.0f; // 3 minutes for network operations
	float LastProgressReport = 0.0f;

	while (!bRiggingComplete)
	{
		const float ElapsedTime = FPlatformTime::Seconds() - AutorigStartTime;

		// Report progress every 15 seconds
		if (ElapsedTime - LastProgressReport > 15.0f)
		{
			UE_LOG(LogTemp, Log, TEXT("Autorig in progress... (%.1f seconds elapsed)"), ElapsedTime);
			UE_LOG(LogTemp, Log, TEXT("  Waiting for cloud service response..."));
			UE_LOG(LogTemp, Log, TEXT("  (Running in background thread - editor remains responsive)"));
			LastProgressReport = ElapsedTime;
		}

		if (ElapsedTime > MaxWaitTime)
		{
			UE_LOG(LogTemp, Warning, TEXT("Autorig operation timed out after %.1f seconds"), ElapsedTime);
			UE_LOG(LogTemp, Warning, TEXT("  The operation may still complete in the background"));
			break;
		}

		// Sleep to avoid busy-waiting - delegate will update flags on GameThread
		FPlatformProcess::Sleep(0.001f);
	}

	// Unregister delegate
	EditorSubsystem->OnRiggingStateChanged.Remove(RiggingStateChangedHandle);

	const float TotalTime = FPlatformTime::Seconds() - AutorigStartTime;
	UE_LOG(LogTemp, Log, TEXT("Autorig operation took %.1f seconds"), TotalTime);

	if (bRiggingComplete && bRiggingSucceeded)
	{
		UE_LOG(LogTemp, Log, TEXT("✓ Character successfully rigged"));
		return true;
	}
	else if (!bRiggingComplete)
	{
		UE_LOG(LogTemp, Warning, TEXT("✗ Autorig did not complete within timeout"));
		UE_LOG(LogTemp, Warning, TEXT("  Check MetaHuman cloud services connection"));
		return false;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("✗ Autorig failed"));
		return false;
	}
}



// ============================================================================
// MetaHuman Cloud Services Authentication Functions 
// ============================================================================

bool UMetaHumanParametricGenerator::EnsureCloudServicesLogin()
{
	UE_LOG(LogTemp, Log, TEXT("Checking MetaHuman cloud services login status..."));

	bool bIsLoggedIn = false;
	bool bCheckComplete = false;

	// log in if not
	TestCloudAuthentication();

	// 异步检查登录状态
	UE::MetaHuman::ServiceAuthentication::CheckHasLoggedInUserAsync(
		UE::MetaHuman::ServiceAuthentication::FOnCheckHasLoggedInUserCompleteDelegate::CreateLambda(
			[&bIsLoggedIn, &bCheckComplete](bool bLoggedIn)
			{
				bIsLoggedIn = bLoggedIn;
				bCheckComplete = true;
			}
		)
	);

	// 等待检查完成（最多等待 5 秒）
	const float MaxWaitTime = 5.0f;
	const float StartTime = FPlatformTime::Seconds();
	while (!bCheckComplete && (FPlatformTime::Seconds() - StartTime) < MaxWaitTime)
	{
		FPlatformProcess::Sleep(0.1f);
	}

	if (!bCheckComplete)
	{
		UE_LOG(LogTemp, Error, TEXT("Timeout while checking cloud services login status"));
		return false;
	}

	if (bIsLoggedIn)
	{
		UE_LOG(LogTemp, Log, TEXT("✓ User is already logged in to MetaHuman cloud services"));
		return true;
	}
	else
	{

		UE_LOG(LogTemp, Log, TEXT("✗ User is NOT logged in to MetaHuman cloud services"));
		return false;
	}
}

void UMetaHumanParametricGenerator::CheckCloudServicesLoginAsync(TFunction<void(bool)> OnCheckComplete)
{
	UE::MetaHuman::ServiceAuthentication::CheckHasLoggedInUserAsync(
		UE::MetaHuman::ServiceAuthentication::FOnCheckHasLoggedInUserCompleteDelegate::CreateLambda(
			[OnCheckComplete](bool bLoggedIn)
			{
				if (OnCheckComplete)
				{
					OnCheckComplete(bLoggedIn);
				}
			}
		)
	);
}

void UMetaHumanParametricGenerator::LoginToCloudServicesAsync(
	TFunction<void()> OnLoginComplete,
	TFunction<void()> OnLoginFailed)
{
	UE::MetaHuman::ServiceAuthentication::LoginToAuthEnvironment(
		UE::MetaHuman::ServiceAuthentication::FOnLoginCompleteDelegate::CreateLambda(
			[OnLoginComplete]()
			{
				UE_LOG(LogTemp, Log, TEXT("✓ Successfully logged in to MetaHuman cloud services"));
				if (OnLoginComplete)
				{
					OnLoginComplete();
				}
			}
		),
		UE::MetaHuman::ServiceAuthentication::FOnLoginFailedDelegate::CreateLambda(
			[OnLoginFailed]()
			{
				UE_LOG(LogTemp, Error, TEXT("✗ Failed to login to MetaHuman cloud services"));
				if (OnLoginFailed)
				{
					OnLoginFailed();
				}
			}
		)
	);
}

void UMetaHumanParametricGenerator::TestCloudAuthentication()
{
	UE_LOG(LogTemp, Log, TEXT("=== Testing MetaHuman Cloud Authentication ==="));

	CheckCloudServicesLoginAsync([](bool bLoggedIn)
	{
		if (bLoggedIn)
		{
			UE_LOG(LogTemp, Log, TEXT("✓ User is logged in to MetaHuman cloud services"));
			UE_LOG(LogTemp, Log, TEXT("  Cloud operations (AutoRig, texture download) should work"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("✗ User is NOT logged in"));
			UE_LOG(LogTemp, Warning, TEXT("  Attempting automatic login..."));

			LoginToCloudServicesAsync(
				[]()
				{
					UE_LOG(LogTemp, Log, TEXT("✓ Login succeeded! Cloud services are now available."));
				},
				[]()
				{
					UE_LOG(LogTemp, Error, TEXT("✗ Login failed! Please login manually via:"));
					UE_LOG(LogTemp, Error, TEXT("  Window > MetaHuman > Cloud Services"));
				}
			);
		}
	});
}

