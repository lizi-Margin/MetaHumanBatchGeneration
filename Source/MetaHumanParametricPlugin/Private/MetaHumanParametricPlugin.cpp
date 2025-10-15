// Copyright Epic Games, Inc. All Rights Reserved.

#include "MetaHumanParametricPlugin.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Async/Async.h"
#include "HAL/PlatformProcess.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "FMetaHumanParametricPluginModule"

// Forward declarations for example functions
extern void Example1_CreateSlenderFemale();
extern void Example2_CreateMuscularMale();
extern void Example3_CreateShortRoundedCharacter();
extern void Example4_BatchCreateCharacters();
extern void Example_PluginTest();

void FMetaHumanParametricPluginModule::StartupModule()
{
	// 插件启动时的初始化代码
	UE_LOG(LogTemp, Log, TEXT("MetaHumanParametricPlugin module has been loaded"));

	// Register menu extensions
	RegisterMenuExtensions();
}

void FMetaHumanParametricPluginModule::ShutdownModule()
{
	// 清理菜单扩展
	UToolMenus::UnregisterOwner(this);

	// 插件关闭时的清理代码
	UE_LOG(LogTemp, Log, TEXT("MetaHumanParametricPlugin module has been unloaded"));
}

void FMetaHumanParametricPluginModule::RegisterMenuExtensions()
{
	// 等待 ToolMenus 模块加载
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateStatic(&FMetaHumanParametricPluginModule::AddToolbarExtension)
	);
}

void FMetaHumanParametricPluginModule::AddToolbarExtension()
{
	UE_LOG(LogTemp, Warning, TEXT("AddToolbarExtension called"));
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.User");
	if (!Menu)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to extend LevelEditor.LevelEditorToolBar.User menu"));
		return;
	}

	FToolMenuSection& Section = Menu->FindOrAddSection("MetaHumanGenerator");
	Section.Label = LOCTEXT("MetaHumanGenerator", "MetaHuman Generator");

	Section.AddSubMenu(
		"MetaHumanExamples",
		LOCTEXT("MetaHumanExamplesLabel", "MetaHuman Examples"),
		LOCTEXT("MetaHumanExamplesTooltip", "Generate example MetaHuman characters"),
		FNewToolMenuDelegate::CreateLambda([](UToolMenu* SubMenu)
		{
			FToolMenuSection& SubSection = SubMenu->AddSection("Examples", LOCTEXT("Examples", "Character Examples"));

			// TEST: Plugin basics test
			FToolMenuEntry& EntryTest = SubSection.AddMenuEntry(
				"PluginTest",
				LOCTEXT("PluginTestLabel", "0. Run Plugin Test (Safe)"),
				LOCTEXT("PluginTestTooltip", "Test plugin functionality without MetaHuman dependencies"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnRunPluginTest))
			);

			SubSection.AddSeparator("TestSeparator");

			FToolMenuEntry& Entry1 = SubSection.AddMenuEntry(
				"SlenderFemale",
				LOCTEXT("SlenderFemaleLabel", "1. Slender Female"),
				LOCTEXT("SlenderFemaleTooltip", "Create a slender female character (168cm, 62cm waist)"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnGenerateSlenderFemale))
			);

			FToolMenuEntry& Entry2 = SubSection.AddMenuEntry(
				"MuscularMale",
				LOCTEXT("MuscularMaleLabel", "2. Muscular Male"),
				LOCTEXT("MuscularMaleTooltip", "Create a muscular male character (185cm, 110cm chest)"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnGenerateMuscularMale))
			);

			FToolMenuEntry& Entry3 = SubSection.AddMenuEntry(
				"ShortRounded",
				LOCTEXT("ShortRoundedLabel", "3. Short Rounded"),
				LOCTEXT("ShortRoundedTooltip", "Create a short rounded character (155cm)"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnGenerateShortRounded))
			);

			FToolMenuEntry& Entry4 = SubSection.AddMenuEntry(
				"BatchGenerate",
				LOCTEXT("BatchGenerateLabel", "4. Batch Generate (5 Characters)"),
				LOCTEXT("BatchGenerateTooltip", "Generate 5 different character types in one go"),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&FMetaHumanParametricPluginModule::OnBatchGenerate))
			);
		}),
		false,
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports")
	);
}

void FMetaHumanParametricPluginModule::OnGenerateSlenderFemale()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting Example 1: Slender Female..."));

	// 显示通知
	FNotificationInfo Info(LOCTEXT("GeneratingSlenderFemale", "Generating Slender Female Character... (This will take a few minutes)"));
	Info.ExpireDuration = 30.0f; // 更长的显示时间，因为生成需要时间
	Info.bUseThrobber = true;
	Info.bUseSuccessFailIcons = true;
	TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);

    Example1_CreateSlenderFemale();
}

void FMetaHumanParametricPluginModule::OnGenerateMuscularMale()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting Example 2: Muscular Male..."));

	FNotificationInfo Info(LOCTEXT("GeneratingMuscularMale", "Generating Muscular Male Character... (This will take a few minutes)"));
	Info.ExpireDuration = 30.0f;
	Info.bUseThrobber = true;
	Info.bUseSuccessFailIcons = true;
	TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);

    Example2_CreateMuscularMale();
}

void FMetaHumanParametricPluginModule::OnGenerateShortRounded()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting Example 3: Short Rounded..."));

	FNotificationInfo Info(LOCTEXT("GeneratingShortRounded", "Generating Short Rounded Character... (This will take a few minutes)"));
	Info.ExpireDuration = 30.0f;
	Info.bUseThrobber = true;
	Info.bUseSuccessFailIcons = true;
	TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);

	Example3_CreateShortRoundedCharacter();
}

void FMetaHumanParametricPluginModule::OnBatchGenerate()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting Example 4: Batch Generate..."));

	FNotificationInfo Info(LOCTEXT("BatchGenerating", "Batch Generating 5 Characters... (This will take several minutes!)"));
	Info.ExpireDuration = 60.0f;
	Info.bUseThrobber = true;
	Info.bUseSuccessFailIcons = true;
	TSharedPtr<SNotificationItem> NotificationPtr = FSlateNotificationManager::Get().AddNotification(Info);

 	Example4_BatchCreateCharacters();
}

void FMetaHumanParametricPluginModule::OnRunPluginTest()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting Plugin Test..."));

	FNotificationInfo Info(LOCTEXT("RunningPluginTest", "Running Plugin Test (Safe - No MetaHuman Dependencies)"));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);

	Example_PluginTest();

	FNotificationInfo CompletedInfo(LOCTEXT("PluginTestComplete", "Plugin Test Complete! Check Output Log for results."));
	CompletedInfo.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(CompletedInfo);
}