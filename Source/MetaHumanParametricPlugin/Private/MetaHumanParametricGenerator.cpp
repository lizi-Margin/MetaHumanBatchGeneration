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

// ============================================================================
// Main Generation Function
// ============================================================================

bool UMetaHumanParametricGenerator::GenerateParametricMetaHuman(
	const FString& CharacterName,
	const FString& OutputPath,
	const FMetaHumanBodyParametricConfig& BodyConfig,
	const FMetaHumanAppearanceConfig& AppearanceConfig,
	UMetaHumanCharacter*& OutCharacter)
{
	UE_LOG(LogTemp, Log, TEXT("=== MetaHuman Parametric Generation Started ==="));
	UE_LOG(LogTemp, Log, TEXT("Character Name: %s"), *CharacterName);
	UE_LOG(LogTemp, Log, TEXT("Output Path: %s"), *OutputPath);

	// Step 0: Check authentication FIRST (新增)
	UE_LOG(LogTemp, Log, TEXT("[Step 0/6] Verifying MetaHuman cloud services authentication..."));
	if (!EnsureCloudServicesLogin())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to authenticate with MetaHuman cloud services!"));
		UE_LOG(LogTemp, Error, TEXT("  Cloud operations (AutoRig, texture download) require authentication"));
		UE_LOG(LogTemp, Error, TEXT("  Please login manually via: Window > MetaHuman > Cloud Services"));
		UE_LOG(LogTemp, Error, TEXT("  Or ensure your Epic Games account has MetaHuman access"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("[Step 0/6] ✓ Authentication verified - cloud services available"));

	// Step 1: Create base Character asset
	UE_LOG(LogTemp, Log, TEXT("[Step 1/5] Creating base MetaHuman Character asset..."));
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
	UE_LOG(LogTemp, Log, TEXT("[Step 1/5] ✓ Base character created"));

	// Step 2: Configure body parameters
	UE_LOG(LogTemp, Log, TEXT("[Step 2/5] Configuring body parameters..."));
	if (!ConfigureBodyParameters(Character, BodyConfig))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to configure body parameters!"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("[Step 2/5] ✓ Body parameters configured"));

	// Step 3: Configure appearance
	UE_LOG(LogTemp, Log, TEXT("[Step 3/5] Configuring appearance..."));
	if (!ConfigureAppearance(Character, AppearanceConfig))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to configure appearance!"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("[Step 3/5] ✓ Appearance configured"));

	// Step 4: Download texture source data (new)
	UE_LOG(LogTemp, Log, TEXT("[Step 4/6] Downloading texture source data..."));
	if (!DownloadTextureSourceData(Character))
	{
		UE_LOG(LogTemp, Warning, TEXT("Warning: Failed to download texture source data, will use default textures"));
		// Don't return false, continue with default textures
	}
	UE_LOG(LogTemp, Log, TEXT("[Step 4/6] ✓ Texture source data download completed"));

	// Step 5: Generate assets
	UE_LOG(LogTemp, Log, TEXT("[Step 5/6] Generating character assets..."));
	FMetaHumanCharacterGeneratedAssets GeneratedAssets;
	if (!GenerateCharacterAssets(Character, GeneratedAssets))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to generate character assets!"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("[Step 5/6] ✓ Assets generated: Face Mesh, Body Mesh, Textures, Physics"));

	// Step 6: Save assets
	UE_LOG(LogTemp, Log, TEXT("[Step 6/6] Saving character assets..."));
	if (!SaveCharacterAssets(Character, OutputPath, GeneratedAssets))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save character assets!"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("[Step 5/5] ✓ Assets saved to: %s"), *OutputPath);

	OutCharacter = Character;
	UE_LOG(LogTemp, Log, TEXT("=== MetaHuman Generation Completed Successfully ==="));
	return true;
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
	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();
	if (EditorSubsystem)
	{
		EditorSubsystem->InitializeMetaHumanCharacter(Character);

		// Register character for editing
		if (!EditorSubsystem->TryAddObjectToEdit(Character))
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to register character for editing, but continuing..."));
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

	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();
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

	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();
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
// Step 4: Generate Character Assets
// ============================================================================

bool UMetaHumanParametricGenerator::GenerateCharacterAssets(
	UMetaHumanCharacter* Character,
	FMetaHumanCharacterGeneratedAssets& OutAssets)
{
	if (!Character)
	{
		return false;
	}

	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();
	if (!EditorSubsystem)
	{
		return false;
	}

	// Create temporary package to hold generated assets
	UPackage* TransientPackage = GetTransientPackage();

	// Call asset generation function
	// This will generate:
	//   - Face Mesh (facial skeletal mesh)
	//   - Body Mesh (body skeletal mesh)
	//   - Textures (skin textures, normal maps, etc.)
	//   - Physics Asset (physics asset)
	//   - RigLogic Assets (facial rig assets)
	if (!EditorSubsystem->TryGenerateCharacterAssets(Character, TransientPackage, OutAssets))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to generate character assets"));
		return false;
	}

	// Validate generated assets
	if (!OutAssets.FaceMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("Generated assets missing FaceMesh"));
		return false;
	}

	if (!OutAssets.BodyMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("Generated assets missing BodyMesh"));
		return false;
	}

	// Print generated asset information
	UE_LOG(LogTemp, Log, TEXT("  ✓ Generated Assets:"));
	UE_LOG(LogTemp, Log, TEXT("    • Face Mesh: %s"), *OutAssets.FaceMesh->GetName());
	UE_LOG(LogTemp, Log, TEXT("    • Body Mesh: %s"), *OutAssets.BodyMesh->GetName());
	UE_LOG(LogTemp, Log, TEXT("    • Face Textures: %d"), OutAssets.SynthesizedFaceTextures.Num());
	UE_LOG(LogTemp, Log, TEXT("    • Body Textures: %d"), OutAssets.BodyTextures.Num());

	if (OutAssets.PhysicsAsset)
	{
		UE_LOG(LogTemp, Log, TEXT("    • Physics Asset: [Valid]"));
	}

	UE_LOG(LogTemp, Log, TEXT("    • Body Measurements: %d"), OutAssets.BodyMeasurements.Num());
	UE_LOG(LogTemp, Log, TEXT("    • Total Metadata Entries: %d"), OutAssets.Metadata.Num());

	return true;
}

// ============================================================================
// Step 5: Save Assets
// ============================================================================

bool UMetaHumanParametricGenerator::SaveCharacterAssets(
	UMetaHumanCharacter* Character,
	const FString& OutputPath,
	const FMetaHumanCharacterGeneratedAssets& GeneratedAssets)
{
	if (!Character)
	{
		return false;
	}

	// 1. Save main character asset
	UPackage* CharacterPackage = Character->GetOutermost();
	FString CharacterFilePath = FPackageName::LongPackageNameToFilename(
		CharacterPackage->GetName(),
		FPackageName::GetAssetPackageExtension()
	);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;

	if (!UPackage::SavePackage(CharacterPackage, Character, *CharacterFilePath, SaveArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save character package: %s"), *CharacterFilePath);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("  ✓ Saved character: %s"), *CharacterFilePath);

	// 2. Register asset to AssetRegistry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().AssetCreated(Character);

	// 3. Save generated meshes and textures (optional - if needed as separate assets)
	// Note: Usually these assets are stored as part of the character and don't need separate saving
	// But if needed, similar SavePackage workflow can be used

	return true;
}

// ============================================================================
// Export to Blueprint
// ============================================================================

UBlueprint* UMetaHumanParametricGenerator::ExportCharacterToBlueprint(
	UMetaHumanCharacter* Character,
	const FString& BlueprintPath,
	const FString& BlueprintName)
{
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid character for blueprint export"));
		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("=== Exporting Character to Blueprint ==="));
	UE_LOG(LogTemp, Log, TEXT("Blueprint: %s/%s"), *BlueprintPath, *BlueprintName);

	// 1. First generate character assets
	FMetaHumanCharacterGeneratedAssets GeneratedAssets;
	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();

	if (!EditorSubsystem || !EditorSubsystem->TryGenerateCharacterAssets(Character, GetTransientPackage(), GeneratedAssets))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to generate assets for blueprint export"));
		return nullptr;
	}

	// 2. Create blueprint
	UBlueprint* Blueprint = CreateBlueprintFromCharacter(
		Character,
		GeneratedAssets,
		BlueprintPath,
		BlueprintName
	);

	if (Blueprint)
	{
		UE_LOG(LogTemp, Log, TEXT("✓ Blueprint created successfully: %s"), *Blueprint->GetPathName());
	}

	return Blueprint;
}

// ============================================================================
// Helper Function: Create Blueprint
// ============================================================================

UBlueprint* UMetaHumanParametricGenerator::CreateBlueprintFromCharacter(
	UMetaHumanCharacter* Character,
	const FMetaHumanCharacterGeneratedAssets& Assets,
	const FString& PackagePath,
	const FString& BlueprintName)
{
	// 1. Create blueprint package
	FString PackageNameStr = FPackageName::ObjectPathToPackageName(PackagePath / BlueprintName);
	UPackage* Package = CreatePackage(*PackageNameStr);

	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create blueprint package"));
		return nullptr;
	}

	// 2. Create Actor blueprint
	UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
		AActor::StaticClass(),
		Package,
		*BlueprintName,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		NAME_None
	);

	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create blueprint"));
		return nullptr;
	}

	// 3. Add skeletal mesh components
	USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
	if (!SCS)
	{
		UE_LOG(LogTemp, Error, TEXT("Blueprint has no SimpleConstructionScript"));
		return nullptr;
	}

	// Add face mesh component
	if (Assets.FaceMesh)
	{
		USCS_Node* FaceNode = SCS->CreateNode(USkeletalMeshComponent::StaticClass(), TEXT("FaceMesh"));
		USkeletalMeshComponent* FaceComp = Cast<USkeletalMeshComponent>(FaceNode->ComponentTemplate);
		if (FaceComp)
		{
			FaceComp->SetSkeletalMesh(Assets.FaceMesh);
			FaceComp->SetRelativeLocation(FVector(0, 0, 0));
			SCS->AddNode(FaceNode);
			UE_LOG(LogTemp, Log, TEXT("  + Added Face Mesh component"));
		}
	}

	// Add body mesh component
	if (Assets.BodyMesh)
	{
		USCS_Node* BodyNode = SCS->CreateNode(USkeletalMeshComponent::StaticClass(), TEXT("BodyMesh"));
		USkeletalMeshComponent* BodyComp = Cast<USkeletalMeshComponent>(BodyNode->ComponentTemplate);
		if (BodyComp)
		{
			BodyComp->SetSkeletalMesh(Assets.BodyMesh);
			BodyComp->SetRelativeLocation(FVector(0, 0, -90));  // Slightly offset body downward
			SCS->AddNode(BodyNode);
			UE_LOG(LogTemp, Log, TEXT("  + Added Body Mesh component"));
		}
	}

	// 4. Compile blueprint
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// 5. Save blueprint
	FString BlueprintFilePath = FPackageName::LongPackageNameToFilename(
		Package->GetName(),
		FPackageName::GetAssetPackageExtension()
	);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;

	if (!UPackage::SavePackage(Package, Blueprint, *BlueprintFilePath, SaveArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save blueprint package"));
		return nullptr;
	}

	// 6. Register to Asset Registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().AssetCreated(Blueprint);

	return Blueprint;
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
	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = nullptr;
	if (IsInGameThread())
	{
		EditorSubsystem = GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();
	}
	else
	{
		// If not in the game thread, we return false directly to let the main thread handle it
		// This is because MetaHuman operations must run on the game thread
		UE_LOG(LogTemp, Warning, TEXT("DownloadTextureSourceData called from background thread - returning false. MetaHuman operations must run on game thread."));
		UE_LOG(LogTemp, Warning, TEXT("  This is normal - texture download will be handled by the main generation process."));
		return false;
	}

	if (!EditorSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get editor subsystem for texture download"));
		return false;
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

	// Check if texture download is already in progress
	if (EditorSubsystem->IsRequestingHighResolutionTextures(Character))
	{
		UE_LOG(LogTemp, Log, TEXT("Texture download already in progress, waiting..."));

		// Wait for download to complete (maximum 60 seconds)
		const float MaxWaitTime = 60.0f;
		const float StartTime = FPlatformTime::Seconds();

		while (EditorSubsystem->IsRequestingHighResolutionTextures(Character))
		{
			const float ElapsedTime = FPlatformTime::Seconds() - StartTime;
			if (ElapsedTime > MaxWaitTime)
			{
				UE_LOG(LogTemp, Warning, TEXT("Texture download timeout after %.1f seconds"), ElapsedTime);
				return false;
			}

			// Handle editor tick to allow download to continue
			// Note: In background thread, we don't need to manually handle tick
			FPlatformProcess::Sleep(0.5f); // Can safely use longer sleep time in background thread
		}

		UE_LOG(LogTemp, Log, TEXT("Texture download completed"));
		return true;
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

	// Step 1: Check if auto-rigging (autorig) is needed
	UE_LOG(LogTemp, Log, TEXT("Checking if autorig is required before texture download..."));

	// Get current rigging state
	EMetaHumanCharacterRigState RigState = EditorSubsystem->GetRiggingState(Character);

	if (RigState != EMetaHumanCharacterRigState::Rigged)
	{
		UE_LOG(LogTemp, Log, TEXT("Character is not rigged, performing autorig with retry logic..."));

		// Retry autorig up to 5 times in case of network errors
		const int32 MaxRetries = 1;
		bool bAutorigSucceeded = false;

		for (int32 RetryCount = 0; RetryCount < MaxRetries && !bAutorigSucceeded; ++RetryCount)
		{
			if (RetryCount > 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("Autorig attempt %d/%d (retrying after failure)..."), RetryCount + 1, MaxRetries);
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("Autorig attempt %d/%d..."), RetryCount + 1, MaxRetries);
			}

			// Check if auto-rigging is already in progress
			if (EditorSubsystem->IsAutoRiggingFace(Character))
			{
				UE_LOG(LogTemp, Log, TEXT("Autorig already in progress, waiting..."));

				// Wait for auto-rigging to complete
				const float MaxAutorigWaitTime = 300.0f; // 5 minutes
				const float AutorigStartTime = FPlatformTime::Seconds();

				while (EditorSubsystem->IsAutoRiggingFace(Character))
				{
					const float AutorigElapsedTime = FPlatformTime::Seconds() - AutorigStartTime;
					if (AutorigElapsedTime > MaxAutorigWaitTime)
					{
						UE_LOG(LogTemp, Warning, TEXT("Autorig timeout after %.1f seconds (likely network timeout)"), AutorigElapsedTime);
						break;
					}

					// In background thread, no need to manually handle tick
					FPlatformProcess::Sleep(1.0f); // Longer sleep time because this is a background operation
				}
			}
			else
			{
				// Execute auto-rigging
				UE_LOG(LogTemp, Log, TEXT("Starting autorig..."));
				EditorSubsystem->AutoRigFace(Character, UE::MetaHuman::ERigType::JointsAndBlendshapes);

				// Wait for auto-rigging to complete
				const float MaxAutorigWaitTime = 300.0f; // 5 minutes
				const float AutorigStartTime = FPlatformTime::Seconds();
				float LastAutorigProgress = 0.0f;

				while (EditorSubsystem->IsAutoRiggingFace(Character))
				{
					const float AutorigElapsedTime = FPlatformTime::Seconds() - AutorigStartTime;

					// Report progress every 15 seconds
					if (AutorigElapsedTime - LastAutorigProgress > 15.0f)
					{
						UE_LOG(LogTemp, Log, TEXT("Autorig in progress... (%.1f seconds elapsed)"), AutorigElapsedTime);
						LastAutorigProgress = AutorigElapsedTime;
					}

					if (AutorigElapsedTime > MaxAutorigWaitTime)
					{
						UE_LOG(LogTemp, Warning, TEXT("Autorig timeout after %.1f seconds (likely network timeout: HTTP request timed out)"), AutorigElapsedTime);
						break;
					}

					// In background thread, no need to manually handle tick
					FPlatformProcess::Sleep(1.0f); // Longer sleep time because this is a background operation
				}

				const float AutorigTotalTime = FPlatformTime::Seconds() - AutorigStartTime;
				UE_LOG(LogTemp, Log, TEXT("Autorig operation took %.1f seconds"), AutorigTotalTime);
			}

			// Check rigging state after this attempt
			RigState = EditorSubsystem->GetRiggingState(Character);
			if (RigState == EMetaHumanCharacterRigState::Rigged)
			{
				bAutorigSucceeded = true;
				UE_LOG(LogTemp, Log, TEXT("✓ Character successfully rigged after %d attempt(s)"), RetryCount + 1);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("✗ Autorig attempt %d/%d failed (character not rigged)"),
					RetryCount + 1, MaxRetries);

				// Wait a bit before retrying (exponential backoff)
				if (RetryCount < MaxRetries - 1)
				{
					const float WaitTime = FMath::Pow(2.0f, RetryCount) * 2.0f; // 2s, 4s, 8s, 16s
					UE_LOG(LogTemp, Log, TEXT("Waiting %.1f seconds before retry..."), WaitTime);
					FPlatformProcess::Sleep(WaitTime);
				}
			}
		}

		// Final check after all retries
		if (!bAutorigSucceeded)
		{
			UE_LOG(LogTemp, Warning, TEXT("Warning: Character still not rigged after %d autorig attempts"), MaxRetries);
			UE_LOG(LogTemp, Warning, TEXT("  Common causes:"));
			UE_LOG(LogTemp, Warning, TEXT("  - Network timeout (HTTP request to autorig service timed out after 300s)"));
			UE_LOG(LogTemp, Warning, TEXT("  - MetaHuman cloud service unavailable"));
			UE_LOG(LogTemp, Warning, TEXT("  - Not logged into MetaHuman cloud services"));
			UE_LOG(LogTemp, Warning, TEXT("  Continuing with texture download anyway..."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Character is already rigged, skipping autorig"));
	}

	// Step 2: Request texture download
	UE_LOG(LogTemp, Log, TEXT("Requesting 2k texture download..."));
	UE_LOG(LogTemp, Warning, TEXT("Note: This requires MetaHuman cloud services login. If not logged in, download will fail but generation will continue with default textures."));

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
		UE_LOG(LogTemp, Warning, TEXT("Texture download failed to start - likely due to authentication issues"));
		UE_LOG(LogTemp, Warning, TEXT("  Please ensure you are logged into MetaHuman cloud services"));
		UE_LOG(LogTemp, Warning, TEXT("  You can log in via: Window > MetaHuman > Cloud Services"));
		return false;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Texture download did not complete within timeout"));
		return false;
	}
}

// ============================================================================
// MetaHuman Cloud Services Authentication Functions (新增)
// ============================================================================

bool UMetaHumanParametricGenerator::EnsureCloudServicesLogin()
{
	UE_LOG(LogTemp, Log, TEXT("Checking MetaHuman cloud services login status..."));

	bool bIsLoggedIn = false;
	bool bCheckComplete = false;

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

	// 用户未登录，尝试登录
	UE_LOG(LogTemp, Warning, TEXT("User is not logged in. Attempting automatic login..."));
	UE_LOG(LogTemp, Warning, TEXT("  Note: A browser window may open for Epic Games login"));

	bool bLoginSucceeded = false;
	bool bLoginComplete = false;

	UE::MetaHuman::ServiceAuthentication::LoginToAuthEnvironment(
		UE::MetaHuman::ServiceAuthentication::FOnLoginCompleteDelegate::CreateLambda(
			[&bLoginSucceeded, &bLoginComplete]()
			{
				bLoginSucceeded = true;
				bLoginComplete = true;
				UE_LOG(LogTemp, Log, TEXT("✓ Successfully logged in to MetaHuman cloud services"));
			}
		),
		UE::MetaHuman::ServiceAuthentication::FOnLoginFailedDelegate::CreateLambda(
			[&bLoginSucceeded, &bLoginComplete]()
			{
				bLoginSucceeded = false;
				bLoginComplete = true;
				UE_LOG(LogTemp, Error, TEXT("✗ Failed to login to MetaHuman cloud services"));
			}
		)
	);

	// 等待登录完成（最多等待 60 秒，因为可能需要打开浏览器）
	const float MaxLoginWaitTime = 60.0f;
	const float LoginStartTime = FPlatformTime::Seconds();
	float LastProgressReport = 0.0f;

	while (!bLoginComplete && (FPlatformTime::Seconds() - LoginStartTime) < MaxLoginWaitTime)
	{
		const float ElapsedTime = FPlatformTime::Seconds() - LoginStartTime;

		// 每 5 秒报告一次进度
		if (ElapsedTime - LastProgressReport > 5.0f)
		{
			UE_LOG(LogTemp, Log, TEXT("Still waiting for login... (%.1f seconds elapsed)"), ElapsedTime);
			UE_LOG(LogTemp, Log, TEXT("  If a browser window opened, please complete the login process"));
			LastProgressReport = ElapsedTime;
		}

		FPlatformProcess::Sleep(0.5f);
	}

	if (!bLoginComplete)
	{
		UE_LOG(LogTemp, Error, TEXT("Timeout while waiting for cloud services login (waited %.1f seconds)"), MaxLoginWaitTime);
		UE_LOG(LogTemp, Warning, TEXT("  Possible causes:"));
		UE_LOG(LogTemp, Warning, TEXT("  - Login browser window was not completed"));
		UE_LOG(LogTemp, Warning, TEXT("  - Network connectivity issues"));
		UE_LOG(LogTemp, Warning, TEXT("  - MetaHuman cloud services unavailable"));
		UE_LOG(LogTemp, Warning, TEXT("  - EOS (Epic Online Services) configuration missing"));
		return false;
	}

	return bLoginSucceeded;
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

