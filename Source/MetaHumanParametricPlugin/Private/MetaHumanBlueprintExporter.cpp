// Copyright Epic Games, Inc. All Rights Reserved.

#include "MetaHumanBlueprintExporter.h"
#include "MetaHumanCharacter.h"
#include "MetaHumanCharacterEditorSubsystem.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Factories/BlueprintFactory.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/Paths.h"
#include "GameFramework/Actor.h"
#include "Editor.h"

bool UMetaHumanBlueprintExporter::ExportUnifiedSkeletalMesh(
	UMetaHumanCharacter* Character,
	const FString& OutputPath,
	const FString& MeshName,
	USkeletalMesh*& OutSkeletalMesh)
{
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("ExportUnifiedSkeletalMesh: Invalid Character"));
		return false;
	}

	// Find the body skeletal mesh from the character
	USkeletalMesh* BodyMesh = FindBodySkeletalMesh(Character);
	if (!BodyMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("ExportUnifiedSkeletalMesh: Could not find body skeletal mesh in character"));
		return false;
	}

	// Create package path
	FString PackagePath = OutputPath;
	if (!PackagePath.EndsWith(TEXT("/")))
	{
		PackagePath += TEXT("/");
	}
	PackagePath += MeshName;

	// Create package
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("ExportUnifiedSkeletalMesh: Failed to create package at %s"), *PackagePath);
		return false;
	}

	// Duplicate the skeletal mesh to the new package
	USkeletalMesh* NewMesh = DuplicateObject<USkeletalMesh>(BodyMesh, Package, *MeshName);
	if (!NewMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("ExportUnifiedSkeletalMesh: Failed to duplicate skeletal mesh"));
		return false;
	}

	// Mark package as dirty and save
	Package->MarkPackageDirty();
	NewMesh->MarkPackageDirty();

	// Save the package
	if (!SavePackageToDisk(Package))
	{
		UE_LOG(LogTemp, Error, TEXT("ExportUnifiedSkeletalMesh: Failed to save package"));
		return false;
	}

	// Register with asset registry
	FAssetRegistryModule::AssetCreated(NewMesh);

	OutSkeletalMesh = NewMesh;
	UE_LOG(LogTemp, Log, TEXT("ExportUnifiedSkeletalMesh: Successfully exported skeletal mesh to %s"), *PackagePath);
	return true;
}

bool UMetaHumanBlueprintExporter::CreatePreviewBlueprint(
	USkeletalMesh* SkeletalMesh,
	const FString& AnimBlueprintPath,
	const FString& OutputPath,
	const FString& BlueprintName,
	UBlueprint*& OutBlueprint)
{
	if (!SkeletalMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePreviewBlueprint: Invalid SkeletalMesh"));
		return false;
	}

	// Load animation blueprint class
	UClass* AnimBPClass = LoadAnimBlueprintClass(AnimBlueprintPath);
	if (!AnimBPClass)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePreviewBlueprint: Failed to load AnimBlueprint at %s"), *AnimBlueprintPath);
		return false;
	}

	// Create package path
	FString PackagePath = OutputPath;
	if (!PackagePath.EndsWith(TEXT("/")))
	{
		PackagePath += TEXT("/");
	}
	PackagePath += BlueprintName;

	// Create Blueprint
	UBlueprint* NewBlueprint = CreateBlueprintAsset(PackagePath, BlueprintName, AActor::StaticClass());
	if (!NewBlueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePreviewBlueprint: Failed to create Blueprint"));
		return false;
	}

	// Configure skeletal mesh component
	if (!ConfigureSkeletalMeshComponent(NewBlueprint, SkeletalMesh, AnimBPClass))
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePreviewBlueprint: Failed to configure SkeletalMeshComponent"));
		return false;
	}

	// Compile Blueprint
	FKismetEditorUtilities::CompileBlueprint(NewBlueprint);

	// Save the package
	UPackage* Package = NewBlueprint->GetPackage();
	if (!SavePackageToDisk(Package))
	{
		UE_LOG(LogTemp, Error, TEXT("CreatePreviewBlueprint: Failed to save Blueprint package"));
		return false;
	}

	OutBlueprint = NewBlueprint;
	UE_LOG(LogTemp, Log, TEXT("CreatePreviewBlueprint: Successfully created Blueprint at %s"), *PackagePath);
	return true;
}

bool UMetaHumanBlueprintExporter::ExportCharacterWithPreviewBP(
	UMetaHumanCharacter* Character,
	const FString& AnimBlueprintPath,
	const FString& OutputPath,
	const FString& BaseName,
	USkeletalMesh*& OutSkeletalMesh,
	UBlueprint*& OutBlueprint)
{
	// Step 1: Export skeletal mesh
	FString MeshName = BaseName + TEXT("_SK");
	if (!ExportUnifiedSkeletalMesh(Character, OutputPath, MeshName, OutSkeletalMesh))
	{
		UE_LOG(LogTemp, Error, TEXT("ExportCharacterWithPreviewBP: Failed to export skeletal mesh"));
		return false;
	}

	// Step 2: Create preview Blueprint
	FString BlueprintName = BaseName + TEXT("_BP");
	if (!CreatePreviewBlueprint(OutSkeletalMesh, AnimBlueprintPath, OutputPath, BlueprintName, OutBlueprint))
	{
		UE_LOG(LogTemp, Error, TEXT("ExportCharacterWithPreviewBP: Failed to create preview Blueprint"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("ExportCharacterWithPreviewBP: Successfully exported character with preview BP"));
	return true;
}

// ============================================================================
// Private Helper Functions
// ============================================================================

USkeletalMesh* UMetaHumanBlueprintExporter::FindBodySkeletalMesh(UMetaHumanCharacter* Character)
{
	if (!Character)
	{
		return nullptr;
	}

	// MetaHuman characters are Blueprint assets with a component hierarchy
	// The structure is typically: Root -> Body (SkeletalMeshComponent) -> Face (SkeletalMeshComponent)
	// We need to find the Body skeletal mesh from the Blueprint's component hierarchy

	// Get the Blueprint from the character
	UBlueprint* CharacterBP = Cast<UBlueprint>(Character->GetClass()->ClassGeneratedBy);
	if (!CharacterBP)
	{
		UE_LOG(LogTemp, Error, TEXT("FindBodySkeletalMesh: Character is not a Blueprint class"));
		return nullptr;
	}

	// Get the Simple Construction Script
	USimpleConstructionScript* SCS = CharacterBP->SimpleConstructionScript;
	if (!SCS)
	{
		UE_LOG(LogTemp, Error, TEXT("FindBodySkeletalMesh: Blueprint has no SimpleConstructionScript"));
		return nullptr;
	}

	// Search through all nodes to find skeletal mesh components
	const TArray<USCS_Node*>& AllNodes = SCS->GetAllNodes();

	USkeletalMesh* BodyMesh = nullptr;
	USkeletalMesh* FaceMesh = nullptr;

	for (USCS_Node* Node : AllNodes)
	{
		if (!Node || !Node->ComponentTemplate)
		{
			continue;
		}

		// Check if this is a SkeletalMeshComponent
		USkeletalMeshComponent* SkelMeshComp = Cast<USkeletalMeshComponent>(Node->ComponentTemplate);
		if (SkelMeshComp && SkelMeshComp->GetSkeletalMeshAsset())
		{
			FString NodeName = Node->GetVariableName().ToString();
			USkeletalMesh* Mesh = SkelMeshComp->GetSkeletalMeshAsset();

			UE_LOG(LogTemp, Log, TEXT("FindBodySkeletalMesh: Found SkeletalMeshComponent '%s' with mesh '%s'"),
				*NodeName, *Mesh->GetName());

			// Identify Body vs Face based on node name
			if (NodeName.Contains(TEXT("Body"), ESearchCase::IgnoreCase))
			{
				BodyMesh = Mesh;
			}
			else if (NodeName.Contains(TEXT("Face"), ESearchCase::IgnoreCase))
			{
				FaceMesh = Mesh;
			}
			// If no specific name, prefer the first mesh we find
			else if (!BodyMesh)
			{
				BodyMesh = Mesh;
			}
		}
	}

	// Prefer Body mesh over Face mesh
	if (BodyMesh)
	{
		UE_LOG(LogTemp, Log, TEXT("FindBodySkeletalMesh: Using Body mesh: %s"), *BodyMesh->GetName());
		return BodyMesh;
	}
	else if (FaceMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindBodySkeletalMesh: No Body mesh found, using Face mesh: %s"), *FaceMesh->GetName());
		// return FaceMesh;
	}

	UE_LOG(LogTemp, Error, TEXT("FindBodySkeletalMesh: Could not find any skeletal mesh in character Blueprint"));
	return nullptr;
}

UBlueprint* UMetaHumanBlueprintExporter::CreateBlueprintAsset(
	const FString& PackagePath,
	const FString& BlueprintName,
	UClass* ParentClass)
{
	// Create package
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateBlueprintAsset: Failed to create package"));
		return nullptr;
	}

	// Create Blueprint using KismetEditorUtilities
	UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
		ParentClass,
		Package,
		*BlueprintName,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		NAME_None
	);

	if (!NewBlueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("CreateBlueprintAsset: FKismetEditorUtilities::CreateBlueprint failed"));
		return nullptr;
	}

	// Mark package dirty
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewBlueprint);

	return NewBlueprint;
}

bool UMetaHumanBlueprintExporter::ConfigureSkeletalMeshComponent(
	UBlueprint* Blueprint,
	USkeletalMesh* SkeletalMesh,
	UClass* AnimBlueprintClass)
{
	if (!Blueprint || !SkeletalMesh || !AnimBlueprintClass)
	{
		return false;
	}

	// Get the Blueprint's simple construction script
	USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
	if (!SCS)
	{
		UE_LOG(LogTemp, Error, TEXT("ConfigureSkeletalMeshComponent: Blueprint has no SimpleConstructionScript"));
		return false;
	}

	// Create a new SkeletalMeshComponent node
	USCS_Node* SkeletalMeshNode = SCS->CreateNode(USkeletalMeshComponent::StaticClass(), TEXT("SkeletalMeshComponent"));
	if (!SkeletalMeshNode)
	{
		UE_LOG(LogTemp, Error, TEXT("ConfigureSkeletalMeshComponent: Failed to create SkeletalMeshComponent node"));
		return false;
	}

	// Get the component template
	USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(SkeletalMeshNode->ComponentTemplate);
	if (!SkeletalMeshComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("ConfigureSkeletalMeshComponent: Failed to get SkeletalMeshComponent template"));
		return false;
	}

	// Set skeletal mesh
	SkeletalMeshComponent->SetSkeletalMesh(SkeletalMesh);

	// Set animation blueprint class
	SkeletalMeshComponent->SetAnimInstanceClass(AnimBlueprintClass);

	// Add the node to the SCS
	SCS->AddNode(SkeletalMeshNode);

	// Validate scene root nodes - this will automatically set it as root if there isn't one
	SCS->ValidateSceneRootNodes();

	// Mark Blueprint as modified
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	return true;
}

UClass* UMetaHumanBlueprintExporter::LoadAnimBlueprintClass(const FString& AnimBlueprintPath)
{
	if (AnimBlueprintPath.IsEmpty())
	{
		return nullptr;
	}

	// Load the animation blueprint asset
	UObject* LoadedObject = LoadObject<UObject>(nullptr, *AnimBlueprintPath);
	if (!LoadedObject)
	{
		UE_LOG(LogTemp, Error, TEXT("LoadAnimBlueprintClass: Failed to load asset at %s"), *AnimBlueprintPath);
		return nullptr;
	}

	// Try to cast to AnimBlueprint
	UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(LoadedObject);
	if (AnimBP && AnimBP->GeneratedClass)
	{
		return AnimBP->GeneratedClass;
	}

	// If direct load failed, try to get the _C class (compiled class)
	FString ClassPath = AnimBlueprintPath + TEXT("_C");
	UClass* AnimClass = LoadObject<UClass>(nullptr, *ClassPath);
	if (AnimClass && AnimClass->IsChildOf(UAnimInstance::StaticClass()))
	{
		return AnimClass;
	}

	UE_LOG(LogTemp, Error, TEXT("LoadAnimBlueprintClass: Loaded object is not an AnimBlueprint"));
	return nullptr;
}

bool UMetaHumanBlueprintExporter::SavePackageToDisk(UPackage* Package)
{
	if (!Package)
	{
		return false;
	}

	// Get package file path
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(),
		FPackageName::GetAssetPackageExtension()
	);

	// Save package
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;

	bool bSuccess = UPackage::SavePackage(
		Package,
		nullptr,
		*PackageFileName,
		SaveArgs
	);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("SavePackageToDisk: Successfully saved package to %s"), *PackageFileName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("SavePackageToDisk: Failed to save package to %s"), *PackageFileName);
	}

	return bSuccess;
}
