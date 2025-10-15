// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SNotificationItem;

/** Character generation types */
enum class ECharacterType
{
	SlenderFemale,
	MuscularMale,
	ShortRounded,
	Batch
};

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
	static void OnRunPluginTest();

private:
	/** Start asynchronous character generation */
	static void StartAsyncCharacterGeneration(TSharedPtr<SNotificationItem> Notification, ECharacterType Type);

	/** Execute character generation on game thread */
	static void ExecuteCharacterGeneration(ECharacterType Type);

	/** Static function for processing pending async tasks */
	static void ProcessPendingTasks();
};
