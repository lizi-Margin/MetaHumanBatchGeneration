// Copyright Epic Games, Inc. All Rights Reserved.

#include "MetaHumanParametricPlugin.h"

#define LOCTEXT_NAMESPACE "FMetaHumanParametricPluginModule"

void FMetaHumanParametricPluginModule::StartupModule()
{
	// 插件启动时的初始化代码
	UE_LOG(LogTemp, Log, TEXT("MetaHumanParametricPlugin module has been loaded"));
}

void FMetaHumanParametricPluginModule::ShutdownModule()
{
	// 插件关闭时的清理代码
	UE_LOG(LogTemp, Log, TEXT("MetaHumanParametricPlugin module has been unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMetaHumanParametricPluginModule, MetaHumanParametricPlugin)
