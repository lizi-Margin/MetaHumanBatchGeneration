// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

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
};
