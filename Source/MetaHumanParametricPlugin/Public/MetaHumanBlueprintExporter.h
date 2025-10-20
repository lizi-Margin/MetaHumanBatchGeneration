// Copyright Epic Games, Inc. All Rights Reserved.
// MetaHuman Blueprint Exporter
//
// Utility class for exporting MetaHuman skeletal mesh and creating
// preview Blueprints with custom animation blueprints

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimBlueprint.h"
#include "Engine/Blueprint.h"

#include "MetaHumanBlueprintExporter.generated.h"

// Forward declarations
class UMetaHumanCharacter;
class USkeletalMeshComponent;

/**
 * MetaHuman Blueprint Exporter
 *
 * Provides functionality to:
 * 1. Export unified skeletal mesh from MetaHuman character
 * 2. Create preview Blueprint with custom animation blueprint
 */
UCLASS(BlueprintType)
class METAHUMANPARAMETRICPLUGIN_API UMetaHumanBlueprintExporter : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Export a unified skeletal mesh from MetaHuman character
	 * Combines face and body meshes into a single skeletal mesh asset
	 *
	 * @param Character - The MetaHuman character to export from
	 * @param OutputPath - Directory path where the mesh will be saved (e.g., "/Game/ExportedCharacters/")
	 * @param MeshName - Name of the exported skeletal mesh asset
	 * @param OutSkeletalMesh - Output: The created/exported skeletal mesh
	 * @return true if export was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Export")
	static bool ExportUnifiedSkeletalMesh(
		UMetaHumanCharacter* Character,
		const FString& OutputPath,
		const FString& MeshName,
		USkeletalMesh*& OutSkeletalMesh);

	/**
	 * Create a preview Blueprint with skeletal mesh and animation blueprint
	 * Creates an Actor Blueprint with a SkeletalMeshComponent configured with
	 * the specified mesh and animation blueprint
	 *
	 * @param SkeletalMesh - The skeletal mesh to use
	 * @param AnimBlueprintPath - Full path to animation blueprint (e.g., "/Game/Path/To/AnimBP.AnimBP")
	 * @param OutputPath - Directory path where the Blueprint will be saved
	 * @param BlueprintName - Name of the Blueprint asset
	 * @param OutBlueprint - Output: The created Blueprint
	 * @return true if creation was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Export")
	static bool CreatePreviewBlueprint(
		USkeletalMesh* SkeletalMesh,
		const FString& AnimBlueprintPath,
		const FString& OutputPath,
		const FString& BlueprintName,
		UBlueprint*& OutBlueprint);

	/**
	 * Complete workflow: Export mesh and create preview Blueprint
	 * This is a convenience function that combines ExportUnifiedSkeletalMesh and CreatePreviewBlueprint
	 *
	 * @param Character - The MetaHuman character to export from
	 * @param AnimBlueprintPath - Full path to animation blueprint
	 * @param OutputPath - Directory path where assets will be saved
	 * @param BaseName - Base name for both the mesh and Blueprint (suffixes will be added)
	 * @param OutSkeletalMesh - Output: The exported skeletal mesh
	 * @param OutBlueprint - Output: The created Blueprint
	 * @return true if the complete workflow was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Export")
	static bool ExportCharacterWithPreviewBP(
		UMetaHumanCharacter* Character,
		const FString& AnimBlueprintPath,
		const FString& OutputPath,
		const FString& BaseName,
		USkeletalMesh*& OutSkeletalMesh,
		UBlueprint*& OutBlueprint);

private:
	/**
	 * Find the body skeletal mesh from MetaHuman character
	 * MetaHuman characters have separate face and body meshes; this finds the body mesh
	 */
	static USkeletalMesh* FindBodySkeletalMesh(UMetaHumanCharacter* Character);

	/**
	 * Create a new Blueprint asset at the specified path
	 * Sets up basic Blueprint structure for an Actor
	 */
	static UBlueprint* CreateBlueprintAsset(
		const FString& PackagePath,
		const FString& BlueprintName,
		UClass* ParentClass);

	/**
	 * Configure the skeletal mesh component in a Blueprint
	 * Sets the mesh and animation blueprint on the component
	 */
	static bool ConfigureSkeletalMeshComponent(
		UBlueprint* Blueprint,
		USkeletalMesh* SkeletalMesh,
		UClass* AnimBlueprintClass);

	/**
	 * Load animation blueprint class from asset path
	 * Handles loading and validation of animation blueprint
	 */
	static UClass* LoadAnimBlueprintClass(const FString& AnimBlueprintPath);

	/**
	 * Save a package to disk
	 * Helper function for saving asset packages
	 */
	static bool SavePackageToDisk(UPackage* Package);
};
