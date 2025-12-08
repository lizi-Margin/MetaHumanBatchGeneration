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

	/** Auto-start batch generation 20 seconds after editor launch */
	void AutoStartBatchGeneration();

	/** Initialize heartbeat system */
	void InitializeHeartbeat();

	/** Heartbeat tick - writes counter to file every 10 seconds */
	bool TickHeartbeat(float DeltaTime);

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
	static void OnStartBatchGeneration(bool bLoopMode = true);
	static void OnStopBatchGeneration();
	static void OnCheckBatchStatus();

private:
	// Store the last generated character for the two-step workflow
	// Note: These are defined in the .cpp file
	static UMetaHumanCharacter* LastGeneratedCharacter;
	static FString LastOutputPath;
	static EMetaHumanQualityLevel LastQualityLevel;

	// Heartbeat variables
	FTSTicker::FDelegateHandle HeartbeatTickerHandle;
	float HeartbeatCounter = 0.0f;
	uint32 HeartbeatValue = 0;
	const float HeartbeatInterval = 10.0f;
	FString HeartbeatFilePath;

};
