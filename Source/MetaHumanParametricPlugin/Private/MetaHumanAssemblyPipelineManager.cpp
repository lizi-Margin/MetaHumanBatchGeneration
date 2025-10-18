// Copyright Epic Games, Inc. All Rights Reserved.
// MetaHuman Assembly Pipeline Manager - Implementation
//
// Wrapper around native FMetaHumanCharacterEditorBuild to properly assemble
// MetaHuman characters with full ABP animation support

#include "MetaHumanAssemblyPipelineManager.h"
#include "MetaHumanCharacterEditorSubsystem.h"
#include "MetaHumanCharacterPaletteProjectSettings.h"
#include "MetaHumanSDKSettings.h"
#include "MetaHumanCollectionPipeline.h"
#include "MetaHumanCollectionEditorPipeline.h"
#include "Subsystem/MetaHumanCharacterBuild.h"

// ============================================================================
// Main Assembly Function
// ============================================================================

bool UMetaHumanAssemblyPipelineManager::BuildMetaHumanCharacter(
	UMetaHumanCharacter* Character,
	const FMetaHumanAssemblyBuildParameters& BuildParams)
{
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("[AssemblyPipeline] Invalid character for assembly"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[AssemblyPipeline] === Starting MetaHuman Assembly ==="));
	UE_LOG(LogTemp, Log, TEXT("[AssemblyPipeline] Character: %s"), *Character->GetName());
	UE_LOG(LogTemp, Log, TEXT("[AssemblyPipeline] Build Path: %s"), *BuildParams.AbsoluteBuildPath);
	UE_LOG(LogTemp, Log, TEXT("[AssemblyPipeline] Common Path: %s"), *BuildParams.CommonFolderPath);

	// Check if character can be built
	FText ErrorMessage;
	if (!CanBuildCharacter(Character, ErrorMessage))
	{
		UE_LOG(LogTemp, Error, TEXT("[AssemblyPipeline] Cannot build character: %s"), *ErrorMessage.ToString());
		return false;
	}

	// Setup pipeline if provided
	UMetaHumanCollectionPipeline* Pipeline = BuildParams.PipelineOverride;
	if (Pipeline)
	{
		if (!InitializePipelineForCharacter(Character, Pipeline))
		{
			UE_LOG(LogTemp, Error, TEXT("[AssemblyPipeline] Failed to initialize pipeline for character"));
			return false;
		}
	}

	// Convert our parameters to native FMetaHumanCharacterEditorBuildParameters
	FMetaHumanCharacterEditorBuildParameters NativeBuildParams;
	NativeBuildParams.NameOverride = BuildParams.NameOverride;
	NativeBuildParams.AbsoluteBuildPath = BuildParams.AbsoluteBuildPath;
	NativeBuildParams.CommonFolderPath = BuildParams.CommonFolderPath;
	NativeBuildParams.PipelineOverride = BuildParams.PipelineOverride;

	UE_LOG(LogTemp, Log, TEXT("[AssemblyPipeline] Calling native FMetaHumanCharacterEditorBuild::BuildMetaHumanCharacter..."));

	// Call the native assembly function
	// This is the same function the Editor GUI uses, so it will properly set up
	// all assets including Animation Blueprints
	FMetaHumanCharacterEditorBuild::BuildMetaHumanCharacter(Character, NativeBuildParams);

	// The native build function handles all notifications and error reporting
	// through the MessageLog system, so we just log success here
	UE_LOG(LogTemp, Log, TEXT("[AssemblyPipeline] === Assembly Complete ==="));
	UE_LOG(LogTemp, Log, TEXT("[AssemblyPipeline] Check the MetaHuman Message Log for detailed results"));

	return true;
}

// ============================================================================
// Pipeline Management Functions
// ============================================================================

UMetaHumanCollectionPipeline* UMetaHumanAssemblyPipelineManager::GetDefaultPipelineForQuality(
	EMetaHumanQualityLevel QualityLevel,
	bool bUseUEFNPipeline)
{
	const UMetaHumanCharacterPaletteProjectSettings* Settings = GetDefault<UMetaHumanCharacterPaletteProjectSettings>();
	if (!Settings)
	{
		UE_LOG(LogTemp, Error, TEXT("[AssemblyPipeline] Failed to get MetaHumanCharacterPaletteProjectSettings"));
		return nullptr;
	}

	TSoftClassPtr<UMetaHumanCollectionPipeline> PipelineClassPtr;

	if (bUseUEFNPipeline)
	{
		// UEFN pipelines
		PipelineClassPtr = Settings->DefaultCharacterUEFNPipelines[QualityLevel];
	}
	else
	{
		// Legacy pipelines (Cinematic/Optimized)
		PipelineClassPtr = Settings->DefaultCharacterLegacyPipelines[QualityLevel];
	}

	if (!PipelineClassPtr.IsValid())
	{
		UClass* PipelineClass = PipelineClassPtr.LoadSynchronous();
		if (!PipelineClass)
		{
			UE_LOG(LogTemp, Error, TEXT("[AssemblyPipeline] Failed to load pipeline class for quality level: %s"),
				*UEnum::GetValueAsString(QualityLevel));
			return nullptr;
		}
	}

	// Create a new instance of the pipeline
	UMetaHumanCollectionPipeline* Pipeline = NewObject<UMetaHumanCollectionPipeline>(
		GetTransientPackage(),
		PipelineClassPtr.LoadSynchronous());

	if (!Pipeline)
	{
		UE_LOG(LogTemp, Error, TEXT("[AssemblyPipeline] Failed to create pipeline instance"));
		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("[AssemblyPipeline] Created pipeline: %s"), *Pipeline->GetClass()->GetName());
	return Pipeline;
}

bool UMetaHumanAssemblyPipelineManager::CanBuildCharacter(
	UMetaHumanCharacter* Character,
	FText& OutErrorMessage)
{
	if (!Character)
	{
		OutErrorMessage = FText::FromString(TEXT("Invalid character (null)"));
		return false;
	}

	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();
	if (!EditorSubsystem)
	{
		OutErrorMessage = FText::FromString(TEXT("Failed to get MetaHumanCharacterEditorSubsystem"));
		return false;
	}

	// Use the native CanBuildMetaHuman function
	bool bCanBuild = EditorSubsystem->CanBuildMetaHuman(Character, OutErrorMessage);

	if (!bCanBuild)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AssemblyPipeline] CanBuildCharacter failed: %s"), *OutErrorMessage.ToString());
	}

	return bCanBuild;
}

FString UMetaHumanAssemblyPipelineManager::GetDefaultBuildPath(EMetaHumanQualityLevel QualityLevel)
{
	const UMetaHumanSDKSettings* Settings = GetDefault<UMetaHumanSDKSettings>();
	if (!Settings)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AssemblyPipeline] Failed to get MetaHumanSDKSettings, using /Game/MetaHumans"));
		return TEXT("/Game/MetaHumans");
	}

	FString BuildPath;
	switch (QualityLevel)
	{
	case EMetaHumanQualityLevel::Cinematic:
		BuildPath = Settings->CinematicImportPath.Path;
		break;
	case EMetaHumanQualityLevel::High:
	case EMetaHumanQualityLevel::Medium:
	case EMetaHumanQualityLevel::Low:
		BuildPath = Settings->OptimizedImportPath.Path;
		break;
	default:
		BuildPath = TEXT("/Game/MetaHumans");
		break;
	}

	if (BuildPath.IsEmpty())
	{
		BuildPath = TEXT("/Game/MetaHumans");
	}

	UE_LOG(LogTemp, Log, TEXT("[AssemblyPipeline] Default build path for %s: %s"),
		*UEnum::GetValueAsString(QualityLevel), *BuildPath);

	return BuildPath;
}

FMetaHumanAssemblyBuildParameters UMetaHumanAssemblyPipelineManager::CreateDefaultBuildParameters(
	UMetaHumanCharacter* Character,
	EMetaHumanQualityLevel QualityLevel,
	const FString& CustomBuildPath)
{
	FMetaHumanAssemblyBuildParameters BuildParams;

	if (Character)
	{
		BuildParams.NameOverride = Character->GetName();
	}

	// Set build path
	if (!CustomBuildPath.IsEmpty())
	{
		BuildParams.AbsoluteBuildPath = CustomBuildPath;
	}
	else
	{
		BuildParams.AbsoluteBuildPath = GetDefaultBuildPath(QualityLevel);
	}

	// Set common folder path based on quality
	// Common folder contains shared assets like skeleton, base materials, etc.
	if (QualityLevel == EMetaHumanQualityLevel::Cinematic)
	{
		BuildParams.CommonFolderPath = BuildParams.AbsoluteBuildPath + TEXT("/Common");
	}
	else
	{
		// For optimized quality levels, use the same common path structure
		BuildParams.CommonFolderPath = BuildParams.AbsoluteBuildPath + TEXT("/Common");
	}

	// Get default pipeline for the quality level
	BuildParams.PipelineOverride = GetDefaultPipelineForQuality(QualityLevel, false);

	UE_LOG(LogTemp, Log, TEXT("[AssemblyPipeline] Created default build parameters:"));
	UE_LOG(LogTemp, Log, TEXT("[AssemblyPipeline]   Name: %s"), *BuildParams.NameOverride);
	UE_LOG(LogTemp, Log, TEXT("[AssemblyPipeline]   Build Path: %s"), *BuildParams.AbsoluteBuildPath);
	UE_LOG(LogTemp, Log, TEXT("[AssemblyPipeline]   Common Path: %s"), *BuildParams.CommonFolderPath);
	UE_LOG(LogTemp, Log, TEXT("[AssemblyPipeline]   Pipeline: %s"),
		BuildParams.PipelineOverride ? *BuildParams.PipelineOverride->GetClass()->GetName() : TEXT("None"));

	return BuildParams;
}

// ============================================================================
// Private Helper Functions
// ============================================================================

bool UMetaHumanAssemblyPipelineManager::InitializePipelineForCharacter(
	UMetaHumanCharacter* Character,
	UMetaHumanCollectionPipeline* Pipeline)
{
	if (!Character || !Pipeline)
	{
		return false;
	}

	// Make sure the pipeline has an editor pipeline
	if (!Pipeline->GetEditorPipeline())
	{
		UE_LOG(LogTemp, Error, TEXT("[AssemblyPipeline] Pipeline has no EditorPipeline"));
		return false;
	}

	// Store the pipeline in the character's PipelinesPerClass map
	// This is what the native code expects
	Character->PipelinesPerClass.Add(Pipeline->GetClass(), Pipeline);

	UE_LOG(LogTemp, Log, TEXT("[AssemblyPipeline] Initialized pipeline for character: %s"),
		*Pipeline->GetClass()->GetName());

	return true;
}
