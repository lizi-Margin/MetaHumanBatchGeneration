// Copyright Epic Games, Inc. All Rights Reserved.
// MetaHuman Asset I/O Utility - Implementation
//
// Utility class for saving and loading MetaHuman generated assets

#include "MetaHumanAssetIOUtility.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

// ============================================================================
// Public API Functions
// ============================================================================

bool UMetaHumanAssetIOUtility::SaveSkeletalMesh(
	USkeletalMesh* Mesh,
	const FString& OutputPath,
	const FString& AssetName)
{
	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MetaHumanAssetIO] Cannot save null skeletal mesh"));
		return false;
	}

	return SaveAssetToPackage<USkeletalMesh>(Mesh, OutputPath, AssetName);
}

bool UMetaHumanAssetIOUtility::SavePhysicsAsset(
	UPhysicsAsset* PhysicsAsset,
	const FString& OutputPath,
	const FString& AssetName)
{
	if (!PhysicsAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MetaHumanAssetIO] Cannot save null physics asset"));
		return false;
	}

	return SaveAssetToPackage<UPhysicsAsset>(PhysicsAsset, OutputPath, AssetName);
}

bool UMetaHumanAssetIOUtility::SaveTexture2D(
	UTexture2D* Texture,
	const FString& OutputPath,
	const FString& AssetName)
{
	if (!Texture)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MetaHumanAssetIO] Cannot save null texture"));
		return false;
	}

	// Sanitize the asset name for textures (they often have special characters)
	FString CleanAssetName = SanitizeAssetName(AssetName);

	return SaveAssetToPackage<UTexture2D>(Texture, OutputPath, CleanAssetName);
}

int32 UMetaHumanAssetIOUtility::SaveAllGeneratedAssets(
	const FMetaHumanCharacterGeneratedAssets& GeneratedAssets,
	const FString& OutputPath,
	const FString& BaseAssetName,
	TArray<FString>& OutSavedAssetPaths)
{
	int32 SavedCount = 0;
	OutSavedAssetPaths.Empty();

	UE_LOG(LogTemp, Log, TEXT("[MetaHumanAssetIO] Saving all generated assets for: %s"), *BaseAssetName);

	// Save Face Mesh
	if (GeneratedAssets.FaceMesh)
	{
		FString AssetName = BaseAssetName + TEXT("_Face");
		if (SaveSkeletalMesh(GeneratedAssets.FaceMesh, OutputPath, AssetName))
		{
			OutSavedAssetPaths.Add(OutputPath / AssetName);
			SavedCount++;
			UE_LOG(LogTemp, Log, TEXT("[MetaHumanAssetIO]   ✓ Saved Face Mesh"));
		}
	}

	// Save Body Mesh
	if (GeneratedAssets.BodyMesh)
	{
		FString AssetName = BaseAssetName + TEXT("_Body");
		if (SaveSkeletalMesh(GeneratedAssets.BodyMesh, OutputPath, AssetName))
		{
			OutSavedAssetPaths.Add(OutputPath / AssetName);
			SavedCount++;
			UE_LOG(LogTemp, Log, TEXT("[MetaHumanAssetIO]   ✓ Saved Body Mesh"));
		}
	}

	// Save Physics Asset
	if (GeneratedAssets.PhysicsAsset)
	{
		FString AssetName = BaseAssetName + TEXT("_Physics");
		if (SavePhysicsAsset(GeneratedAssets.PhysicsAsset, OutputPath, AssetName))
		{
			OutSavedAssetPaths.Add(OutputPath / AssetName);
			SavedCount++;
			UE_LOG(LogTemp, Log, TEXT("[MetaHumanAssetIO]   ✓ Saved Physics Asset"));
		}
	}

	// Save Face Textures
	int32 FaceTextureCount = 0;
	for (const auto& TexturePair : GeneratedAssets.SynthesizedFaceTextures)
	{
		if (TexturePair.Value)
		{
			FString TextureName = FString::Printf(TEXT("%s_Face_%s"),
				*BaseAssetName,
				*UEnum::GetValueAsString(TexturePair.Key));

			if (SaveTexture2D(TexturePair.Value, OutputPath, TextureName))
			{
				OutSavedAssetPaths.Add(OutputPath / SanitizeAssetName(TextureName));
				SavedCount++;
				FaceTextureCount++;
			}
		}
	}
	if (FaceTextureCount > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[MetaHumanAssetIO]   ✓ Saved %d Face Textures"), FaceTextureCount);
	}

	// Save Body Textures
	int32 BodyTextureCount = 0;
	for (const auto& TexturePair : GeneratedAssets.BodyTextures)
	{
		if (TexturePair.Value)
		{
			FString TextureName = FString::Printf(TEXT("%s_Body_%s"),
				*BaseAssetName,
				*UEnum::GetValueAsString(TexturePair.Key));

			if (SaveTexture2D(TexturePair.Value, OutputPath, TextureName))
			{
				OutSavedAssetPaths.Add(OutputPath / SanitizeAssetName(TextureName));
				SavedCount++;
				BodyTextureCount++;
			}
		}
	}
	if (BodyTextureCount > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[MetaHumanAssetIO]   ✓ Saved %d Body Textures"), BodyTextureCount);
	}

	UE_LOG(LogTemp, Log, TEXT("[MetaHumanAssetIO] Total assets saved: %d"), SavedCount);
	return SavedCount;
}

// ============================================================================
// Private Helper Functions
// ============================================================================

template<typename T>
bool UMetaHumanAssetIOUtility::SaveAssetToPackage(
	T* Asset,
	const FString& OutputPath,
	const FString& AssetName)
{
	if (!Asset)
	{
		return false;
	}

	// Create package for the asset
	FString PackageNameStr = FPackageName::ObjectPathToPackageName(OutputPath / AssetName);
	UPackage* Package = CreatePackage(*PackageNameStr);

	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("[MetaHumanAssetIO] Failed to create package: %s"), *PackageNameStr);
		return false;
	}

	// Duplicate the asset into the new package
	T* NewAsset = DuplicateObject<T>(Asset, Package, *AssetName);
	if (!NewAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("[MetaHumanAssetIO] Failed to duplicate asset: %s"), *AssetName);
		return false;
	}

	// Set flags
	NewAsset->SetFlags(RF_Public | RF_Standalone);
	Package->MarkPackageDirty();

	// Save package
	FString FilePath = FPackageName::LongPackageNameToFilename(
		PackageNameStr,
		FPackageName::GetAssetPackageExtension()
	);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;

	if (!UPackage::SavePackage(Package, NewAsset, *FilePath, SaveArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("[MetaHumanAssetIO] Failed to save package: %s"), *FilePath);
		return false;
	}

	// Register with asset registry
	RegisterAssetWithRegistry(NewAsset);

	return true;
}

FString UMetaHumanAssetIOUtility::SanitizeAssetName(const FString& AssetName)
{
	FString CleanName = AssetName;

	// Remove or replace invalid characters
	CleanName.ReplaceInline(TEXT("::"), TEXT("_"));
	CleanName.ReplaceInline(TEXT(":"), TEXT("_"));
	CleanName.ReplaceInline(TEXT(" "), TEXT("_"));
	CleanName.ReplaceInline(TEXT("-"), TEXT("_"));
	CleanName.ReplaceInline(TEXT("("), TEXT("_"));
	CleanName.ReplaceInline(TEXT(")"), TEXT("_"));
	CleanName.ReplaceInline(TEXT("["), TEXT("_"));
	CleanName.ReplaceInline(TEXT("]"), TEXT("_"));

	return CleanName;
}

void UMetaHumanAssetIOUtility::RegisterAssetWithRegistry(UObject* Asset)
{
	if (!Asset)
	{
		return;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().AssetCreated(Asset);
}
