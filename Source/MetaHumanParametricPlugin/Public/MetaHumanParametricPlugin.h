// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * MetaHuman Parametric Plugin Module
 *
 * 提供程序化创建和定制 MetaHuman 角色的 C++ API
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
};
