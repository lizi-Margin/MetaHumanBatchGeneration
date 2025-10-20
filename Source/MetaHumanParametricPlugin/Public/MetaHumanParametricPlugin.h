// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "MetaHumanParametricPlugin.h"
#include "MetaHumanParametricGenerator.h"

class SNotificationItem;

/**
 * MetaHuman Parametric Plugin Module
 */
class FMetaHumanParametricPluginModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	/** Register the toolbar menu extension */
	void RegisterMenuExtensions();

	/** Add toolbar menu entries - called by ToolMenus startup callback */
	static void AddToolbarExtension();

	/** Menu command callbacks - must be static for CreateStatic */
	static void OnGenerateSlenderFemale();
	static void OnGenerateMuscularMale();
	static void OnGenerateShortRounded();
	static void OnBatchGenerate();
	static void OnRunPluginTest();

	/** Two-Step Workflow callbacks */
	static void OnStep1PrepareAndRig();
	static void OnCheckRiggingStatus();
	static void OnStep2Assemble();
	static void OnExportWithAnimBP();

	/** Authentication menu command callbacks */
	static void OnCheckAuthentication();
	static void OnLoginToCloudServices();
	static void OnTestAuthentication();

	/** Batch Generation callbacks */
	static void OnStartBatchGeneration();
	static void OnStopBatchGeneration();
	static void OnCheckBatchStatus();

private:
	// Store the last generated character for the two-step workflow
	// Note: These are defined in the .cpp file
	static UMetaHumanCharacter* LastGeneratedCharacter;
	static FString LastOutputPath;
	static EMetaHumanQualityLevel LastQualityLevel;
};
