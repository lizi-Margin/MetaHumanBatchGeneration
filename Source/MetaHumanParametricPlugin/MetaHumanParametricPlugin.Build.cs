// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MetaHumanParametricPlugin : ModuleRules
{
	public MetaHumanParametricPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UnrealEd",

				// MetaHuman 核心模块
				"MetaHumanCharacter",           // UMetaHumanCharacter
				"MetaHumanCharacterEditor",     // UMetaHumanCharacterEditorSubsystem
				"MetaHumanCoreTechLib",         // FMetaHumanCharacterBodyIdentity
				"MetaHumanSDKRuntime",          // EMetaHumanBodyType
				"MetaHumanDefaultPipeline",     // FMetaHumanDefaultAssemblyOutput

				// 其他依赖
				"RigLogic",                     // DNA 系统
				"Slate",
				"SlateCore"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// UE 核心模块
				"Projects",
				"InputCore",
				"ToolMenus",
				"EditorStyle",
				"EditorWidgets",
				"UnrealEd",
				"LevelEditor",

				// 资产相关
				"AssetRegistry",
				"AssetTools",
				"ContentBrowser",

				// 蓝图相关
				"Kismet",
				"KismetCompiler",
				"BlueprintGraph",

				// 骨骼网格和动画
				"SkeletalMeshUtilitiesCommon",
				"AnimGraph",
				"AnimGraphRuntime",

				// 其他
				"PropertyEditor",
				"DetailCustomizations"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);

		// 编辑器模块设置
		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}

		// 平台特定设置
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Win64 特定的设置
		}
	}
}
