// Copyright Epic Games, Inc. All Rights Reserved.
// MetaHuman Assembly Pipeline Manager
//
// Wrapper around native FMetaHumanCharacterEditorBuild to properly assemble
// MetaHuman characters with full ABP animation support

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MetaHumanCharacter.h"
#include "MetaHumanCollectionPipeline.h"

#include "MetaHumanAssemblyPipelineManager.generated.h"

// Forward declarations
class UMetaHumanCollectionPipeline;
enum class EMetaHumanQualityLevel : uint8;
enum class EMetaHumanDefaultPipelineType : uint8;

/**
 * Build parameters for MetaHuman character assembly
 * Mirrors FMetaHumanCharacterEditorBuildParameters from the native code
 */
USTRUCT(BlueprintType)
struct FMetaHumanAssemblyBuildParameters
{
	GENERATED_BODY()

	/** Override name for the assembled character (if empty, uses character name) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
	FString NameOverride;

	/** Absolute path where assembled assets will be stored */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
	FString AbsoluteBuildPath;

	/** Path for common/shared assets (skeletal mesh, skeleton, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
	FString CommonFolderPath;

	/** Pipeline to use for assembly (optional - uses character's default if null) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build")
	TObjectPtr<UMetaHumanCollectionPipeline> PipelineOverride;

	FMetaHumanAssemblyBuildParameters()
		: NameOverride(TEXT(""))
		, AbsoluteBuildPath(TEXT(""))
		, CommonFolderPath(TEXT(""))
		, PipelineOverride(nullptr)
	{
	}
};

/**
 * MetaHuman Assembly Pipeline Manager
 *
 * Provides an OOP wrapper around the native MetaHuman assembly system.
 * Uses FMetaHumanCharacterEditorBuild::BuildMetaHumanCharacter to properly
 * assemble characters with full animation blueprint support.
 *
 * This is the proper way to assemble MetaHuman characters programmatically,
 * as it replicates what the Editor GUI does.
 */
UCLASS(BlueprintType)
class METAHUMANPARAMETRICPLUGIN_API UMetaHumanAssemblyPipelineManager : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Build/Assemble a MetaHuman character using the native assembly pipeline
	 * This is the proper way to create production-ready MetaHuman characters
	 *
	 * @param Character - The MetaHuman character to assemble
	 * @param BuildParams - Build parameters (paths, pipeline, etc.)
	 * @return true if assembly was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Assembly")
	static bool BuildMetaHumanCharacter(
		UMetaHumanCharacter* Character,
		const FMetaHumanAssemblyBuildParameters& BuildParams);

	/**
	 * Get the default pipeline for a given quality level
	 *
	 * @param QualityLevel - The quality level (Cinematic, High, Medium, Low)
	 * @param bUseUEFNPipeline - If true, returns UEFN pipeline instead of legacy
	 * @return The pipeline class for the given quality level
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Assembly")
	static UMetaHumanCollectionPipeline* GetDefaultPipelineForQuality(
		EMetaHumanQualityLevel QualityLevel,
		bool bUseUEFNPipeline = false);

	/**
	 * Check if a character can be built with the current settings
	 *
	 * @param Character - The character to check
	 * @param OutErrorMessage - Error message if character cannot be built
	 * @return true if character can be built
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Assembly")
	static bool CanBuildCharacter(
		UMetaHumanCharacter* Character,
		FText& OutErrorMessage);

	/**
	 * Get the default build path for a given quality level
	 * Reads from MetaHumanSDKSettings
	 *
	 * @param QualityLevel - The quality level
	 * @return The default import/build path for the quality level
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Assembly")
	static FString GetDefaultBuildPath(EMetaHumanQualityLevel QualityLevel);

	/**
	 * Create default build parameters with sensible defaults
	 *
	 * @param Character - The character to build
	 * @param QualityLevel - The quality level to use
	 * @param CustomBuildPath - Optional custom build path (if empty, uses default)
	 * @return Build parameters ready to use
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Assembly")
	static FMetaHumanAssemblyBuildParameters CreateDefaultBuildParameters(
		UMetaHumanCharacter* Character,
		EMetaHumanQualityLevel QualityLevel,
		const FString& CustomBuildPath = TEXT(""));

private:
	/**
	 * Initialize a pipeline for a character
	 * Ensures the character has the correct pipeline assigned
	 */
	static bool InitializePipelineForCharacter(
		UMetaHumanCharacter* Character,
		UMetaHumanCollectionPipeline* Pipeline);
};
