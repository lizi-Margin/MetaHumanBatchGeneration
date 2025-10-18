// Copyright Epic Games, Inc. All Rights Reserved.
// MetaHuman Asset I/O Utility
//
// Utility class for saving and loading MetaHuman generated assets

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "MetaHumanCharacter.h"

#include "MetaHumanAssetIOUtility.generated.h"

/**
 * Utility class for saving and loading MetaHuman generated assets
 * Provides clean separation of asset I/O operations from generation logic
 */
UCLASS(BlueprintType)
class METAHUMANPARAMETRICPLUGIN_API UMetaHumanAssetIOUtility : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Save a skeletal mesh as a separate asset
	 *
	 * @param Mesh - The skeletal mesh to save
	 * @param OutputPath - Directory path where the asset will be saved (e.g., "/Game/MyCharacters/")
	 * @param AssetName - Name of the asset file
	 * @return true if save was successful, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|AssetIO")
	static bool SaveSkeletalMesh(
		USkeletalMesh* Mesh,
		const FString& OutputPath,
		const FString& AssetName);

	/**
	 * Save a physics asset as a separate asset
	 *
	 * @param PhysicsAsset - The physics asset to save
	 * @param OutputPath - Directory path where the asset will be saved
	 * @param AssetName - Name of the asset file
	 * @return true if save was successful, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|AssetIO")
	static bool SavePhysicsAsset(
		UPhysicsAsset* PhysicsAsset,
		const FString& OutputPath,
		const FString& AssetName);

	/**
	 * Save a texture as a separate asset
	 *
	 * @param Texture - The texture to save
	 * @param OutputPath - Directory path where the asset will be saved
	 * @param AssetName - Name of the asset file (will be sanitized automatically)
	 * @return true if save was successful, false otherwise
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|AssetIO")
	static bool SaveTexture2D(
		UTexture2D* Texture,
		const FString& OutputPath,
		const FString& AssetName);

	/**
	 * Save all generated assets from a MetaHumanCharacterGeneratedAssets struct
	 * This is a convenience function that saves all meshes, physics assets, and textures
	 *
	 * Note: Not BlueprintCallable because FMetaHumanCharacterGeneratedAssets is not a BlueprintType
	 *
	 * @param GeneratedAssets - The struct containing all generated assets
	 * @param OutputPath - Directory path where assets will be saved
	 * @param BaseAssetName - Base name for all assets (suffixes will be added automatically)
	 * @param OutSavedAssetPaths - Optional output array of all saved asset paths
	 * @return Number of assets successfully saved
	 */
	static int32 SaveAllGeneratedAssets(
		const FMetaHumanCharacterGeneratedAssets& GeneratedAssets,
		const FString& OutputPath,
		const FString& BaseAssetName,
		TArray<FString>& OutSavedAssetPaths);

private:
	/**
	 * Generic template function for saving UObject-derived assets
	 * Handles package creation, duplication, and registration
	 */
	template<typename T>
	static bool SaveAssetToPackage(
		T* Asset,
		const FString& OutputPath,
		const FString& AssetName);

	/**
	 * Sanitize asset name by removing invalid characters
	 */
	static FString SanitizeAssetName(const FString& AssetName);

	/**
	 * Register an asset with the Asset Registry
	 */
	static void RegisterAssetWithRegistry(UObject* Asset);
};
