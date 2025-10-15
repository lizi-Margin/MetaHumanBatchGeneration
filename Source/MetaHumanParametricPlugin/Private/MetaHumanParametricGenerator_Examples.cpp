// Copyright Epic Games, Inc. All Rights Reserved.
// MetaHuman Parametric Generator - Usage Example
//
// 这个文件展示如何使用 UMetaHumanParametricGenerator 创建不同类型的角色

#include "MetaHumanParametricGenerator.h"
#include "Engine/World.h"

/**
 * 示例 1: 创建一个女性角色 - 中等身高、苗条身材
 */
void Example1_CreateSlenderFemale()
{
	UE_LOG(LogTemp, Log, TEXT("========================================"));
	UE_LOG(LogTemp, Log, TEXT("Example 1: Creating Slender Female Character"));
	UE_LOG(LogTemp, Log, TEXT("========================================"));

	// 配置身体参数
	FMetaHumanBodyParametricConfig BodyConfig;
	BodyConfig.BodyType = EMetaHumanBodyType::f_med_nrw;  // 女性、中等身高、正常体重
	BodyConfig.bUseParametricBody = true;
	BodyConfig.GlobalDeltaScale = 1.0f;

	// 设置苗条身材的测量值
	BodyConfig.BodyMeasurements.Empty();
	BodyConfig.BodyMeasurements.Add(TEXT("Height"), 168.0f);       // 168cm 身高
	BodyConfig.BodyMeasurements.Add(TEXT("Chest"), 82.0f);         // 胸围 82cm
	BodyConfig.BodyMeasurements.Add(TEXT("Waist"), 62.0f);         // 腰围 62cm (苗条)
	BodyConfig.BodyMeasurements.Add(TEXT("Hips"), 88.0f);          // 臀围 88cm
	BodyConfig.BodyMeasurements.Add(TEXT("ShoulderWidth"), 38.0f); // 肩宽
	BodyConfig.BodyMeasurements.Add(TEXT("ArmLength"), 58.0f);     // 臂长
	BodyConfig.BodyMeasurements.Add(TEXT("LegLength"), 90.0f);     // 腿长 (较长的腿部比例)

	// 配置外观
	FMetaHumanAppearanceConfig AppearanceConfig;
	AppearanceConfig.SkinToneU = 0.4f;  // 略微偏白皙的肤色
	AppearanceConfig.SkinToneV = 0.5f;
	AppearanceConfig.SkinRoughness = 1.0f;
	AppearanceConfig.IrisPattern = EMetaHumanCharacterEyesIrisPattern::Iris002;  // 深色眼睛
	AppearanceConfig.IrisPrimaryColorU = 0.25f;  // 棕色眼睛
	AppearanceConfig.IrisPrimaryColorV = 0.35f;
	AppearanceConfig.EyelashesType = EMetaHumanCharacterEyelashesType::LongCurl;  // 长卷睫毛
	AppearanceConfig.bEnableEyelashGrooms = true;

	// 生成角色
	UMetaHumanCharacter* Character = nullptr;
	bool bSuccess = UMetaHumanParametricGenerator::GenerateParametricMetaHuman(
		TEXT("SlenderFemale_Character"),
		TEXT("/Game/MyMetaHumans/Females/"),
		BodyConfig,
		AppearanceConfig,
		Character
	);

	if (bSuccess && Character)
	{
		UE_LOG(LogTemp, Log, TEXT("✓ Character created successfully!"));

		// 导出为蓝图
		UBlueprint* Blueprint = UMetaHumanParametricGenerator::ExportCharacterToBlueprint(
			Character,
			TEXT("/Game/MyMetaHumans/Blueprints/"),
			TEXT("BP_SlenderFemale")
		);

		if (Blueprint)
		{
			UE_LOG(LogTemp, Log, TEXT("✓ Blueprint exported: %s"), *Blueprint->GetPathName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("✗ Failed to create character"));
	}
}

/**
 * 示例 2: 创建一个男性角色 - 高大、强壮身材
 */
void Example2_CreateMuscularMale()
{
	UE_LOG(LogTemp, Log, TEXT("========================================"));
	UE_LOG(LogTemp, Log, TEXT("Example 2: Creating Muscular Male Character"));
	UE_LOG(LogTemp, Log, TEXT("========================================"));

	// 配置身体参数
	FMetaHumanBodyParametricConfig BodyConfig;
	BodyConfig.BodyType = EMetaHumanBodyType::m_tal_ovw;  // 男性、高大、偏重体型
	BodyConfig.bUseParametricBody = true;
	BodyConfig.GlobalDeltaScale = 1.0f;

	// 设置强壮身材的测量值
	BodyConfig.BodyMeasurements.Empty();
	BodyConfig.BodyMeasurements.Add(TEXT("Height"), 185.0f);       // 185cm 高大身材
	BodyConfig.BodyMeasurements.Add(TEXT("Chest"), 110.0f);        // 宽阔的胸围
	BodyConfig.BodyMeasurements.Add(TEXT("Waist"), 85.0f);         // 腰围
	BodyConfig.BodyMeasurements.Add(TEXT("Hips"), 100.0f);         // 臀围
	BodyConfig.BodyMeasurements.Add(TEXT("ShoulderWidth"), 48.0f); // 宽肩
	BodyConfig.BodyMeasurements.Add(TEXT("ArmLength"), 65.0f);     // 长臂
	BodyConfig.BodyMeasurements.Add(TEXT("LegLength"), 95.0f);     // 腿长

	// 配置外观
	FMetaHumanAppearanceConfig AppearanceConfig;
	AppearanceConfig.SkinToneU = 0.6f;  // 略深的肤色
	AppearanceConfig.SkinToneV = 0.45f;
	AppearanceConfig.SkinRoughness = 1.15f;  // 稍微粗糙的皮肤
	AppearanceConfig.IrisPattern = EMetaHumanCharacterEyesIrisPattern::Iris005;
	AppearanceConfig.IrisPrimaryColorU = 0.35f;  // 蓝绿色眼睛
	AppearanceConfig.IrisPrimaryColorV = 0.65f;
	AppearanceConfig.EyelashesType = EMetaHumanCharacterEyelashesType::Thin;
	AppearanceConfig.bEnableEyelashGrooms = true;

	// 生成角色
	UMetaHumanCharacter* Character = nullptr;
	bool bSuccess = UMetaHumanParametricGenerator::GenerateParametricMetaHuman(
		TEXT("MuscularMale_Character"),
		TEXT("/Game/MyMetaHumans/Males/"),
		BodyConfig,
		AppearanceConfig,
		Character
	);

	if (bSuccess && Character)
	{
		UE_LOG(LogTemp, Log, TEXT("✓ Character created successfully!"));

		// 导出为蓝图
		UBlueprint* Blueprint = UMetaHumanParametricGenerator::ExportCharacterToBlueprint(
			Character,
			TEXT("/Game/MyMetaHumans/Blueprints/"),
			TEXT("BP_MuscularMale")
		);

		if (Blueprint)
		{
			UE_LOG(LogTemp, Log, TEXT("✓ Blueprint exported: %s"), *Blueprint->GetPathName());
		}
	}
}

/**
 * 示例 3: 创建一个矮小、圆润的角色
 */
void Example3_CreateShortRoundedCharacter()
{
	UE_LOG(LogTemp, Log, TEXT("========================================"));
	UE_LOG(LogTemp, Log, TEXT("Example 3: Creating Short Rounded Character"));
	UE_LOG(LogTemp, Log, TEXT("========================================"));

	// 配置身体参数
	FMetaHumanBodyParametricConfig BodyConfig;
	BodyConfig.BodyType = EMetaHumanBodyType::f_srt_ovw;  // 女性、矮小、超重体型
	BodyConfig.bUseParametricBody = true;
	BodyConfig.GlobalDeltaScale = 1.0f;

	// 设置圆润身材的测量值
	BodyConfig.BodyMeasurements.Empty();
	BodyConfig.BodyMeasurements.Add(TEXT("Height"), 155.0f);       // 155cm 矮小
	BodyConfig.BodyMeasurements.Add(TEXT("Chest"), 105.0f);        // 较大胸围
	BodyConfig.BodyMeasurements.Add(TEXT("Waist"), 92.0f);         // 较粗腰围
	BodyConfig.BodyMeasurements.Add(TEXT("Hips"), 110.0f);         // 较宽臀围
	BodyConfig.BodyMeasurements.Add(TEXT("ShoulderWidth"), 42.0f);
	BodyConfig.BodyMeasurements.Add(TEXT("ArmLength"), 55.0f);
	BodyConfig.BodyMeasurements.Add(TEXT("LegLength"), 75.0f);     // 较短的腿

	// 配置外观
	FMetaHumanAppearanceConfig AppearanceConfig;
	AppearanceConfig.SkinToneU = 0.55f;
	AppearanceConfig.SkinToneV = 0.55f;
	AppearanceConfig.SkinRoughness = 1.1f;
	AppearanceConfig.IrisPattern = EMetaHumanCharacterEyesIrisPattern::Iris003;
	AppearanceConfig.IrisPrimaryColorU = 0.45f;  // 浅棕色眼睛
	AppearanceConfig.IrisPrimaryColorV = 0.50f;
	AppearanceConfig.EyelashesType = EMetaHumanCharacterEyelashesType::ShortFine;
	AppearanceConfig.bEnableEyelashGrooms = true;

	// 生成角色
	UMetaHumanCharacter* Character = nullptr;
	bool bSuccess = UMetaHumanParametricGenerator::GenerateParametricMetaHuman(
		TEXT("ShortRounded_Character"),
		TEXT("/Game/MyMetaHumans/Various/"),
		BodyConfig,
		AppearanceConfig,
		Character
	);

	if (bSuccess && Character)
	{
		UE_LOG(LogTemp, Log, TEXT("✓ Character created successfully!"));

		// 导出为蓝图
		UBlueprint* Blueprint = UMetaHumanParametricGenerator::ExportCharacterToBlueprint(
			Character,
			TEXT("/Game/MyMetaHumans/Blueprints/"),
			TEXT("BP_ShortRounded")
		);

		if (Blueprint)
		{
			UE_LOG(LogTemp, Log, TEXT("✓ Blueprint exported: %s"), *Blueprint->GetPathName());
		}
	}
}

/**
 * 示例 4: 批量创建多个不同体型的角色
 */
void Example4_BatchCreateCharacters()
{
	UE_LOG(LogTemp, Log, TEXT("========================================"));
	UE_LOG(LogTemp, Log, TEXT("Example 4: Batch Creating Multiple Characters"));
	UE_LOG(LogTemp, Log, TEXT("========================================"));

	// 定义多种体型配置
	struct FCharacterPreset
	{
		FString Name;
		EMetaHumanBodyType BodyType;
		float Height;
		float ChestSize;
		float WaistSize;
		float SkinU;
		float SkinV;
	};

	TArray<FCharacterPreset> Presets = {
		{TEXT("Athletic_Female"),  EMetaHumanBodyType::f_med_nrw, 172.0f, 88.0f, 68.0f, 0.45f, 0.50f},
		{TEXT("Average_Male"),     EMetaHumanBodyType::m_med_nrw, 178.0f, 98.0f, 82.0f, 0.50f, 0.50f},
		{TEXT("Tall_Slim_Female"), EMetaHumanBodyType::f_tal_unw, 180.0f, 80.0f, 60.0f, 0.40f, 0.55f},
		{TEXT("Short_Stocky_Male"),EMetaHumanBodyType::m_srt_ovw, 165.0f, 105.0f, 95.0f, 0.55f, 0.45f},
		{TEXT("Petite_Female"),    EMetaHumanBodyType::f_srt_nrw, 158.0f, 82.0f, 64.0f, 0.48f, 0.52f}
	};

	// 批量生成
	int32 SuccessCount = 0;
	for (const FCharacterPreset& Preset : Presets)
	{
		UE_LOG(LogTemp, Log, TEXT("Creating: %s..."), *Preset.Name);

		// 配置身体
		FMetaHumanBodyParametricConfig BodyConfig;
		BodyConfig.BodyType = Preset.BodyType;
		BodyConfig.bUseParametricBody = true;
		BodyConfig.GlobalDeltaScale = 1.0f;
		BodyConfig.BodyMeasurements.Empty();
		BodyConfig.BodyMeasurements.Add(TEXT("Height"), Preset.Height);
		BodyConfig.BodyMeasurements.Add(TEXT("Chest"), Preset.ChestSize);
		BodyConfig.BodyMeasurements.Add(TEXT("Waist"), Preset.WaistSize);

		// 配置外观
		FMetaHumanAppearanceConfig AppearanceConfig;
		AppearanceConfig.SkinToneU = Preset.SkinU;
		AppearanceConfig.SkinToneV = Preset.SkinV;
		AppearanceConfig.IrisPattern = EMetaHumanCharacterEyesIrisPattern::Iris001;
		AppearanceConfig.IrisPrimaryColorU = 0.3f;
		AppearanceConfig.IrisPrimaryColorV = 0.5f;

		// 生成
		UMetaHumanCharacter* Character = nullptr;
		if (UMetaHumanParametricGenerator::GenerateParametricMetaHuman(
			Preset.Name,
			TEXT("/Game/MyMetaHumans/Batch/"),
			BodyConfig,
			AppearanceConfig,
			Character))
		{
			SuccessCount++;
			UE_LOG(LogTemp, Log, TEXT("  ✓ %s created"), *Preset.Name);

			// 导出蓝图
			FString BlueprintName = FString::Printf(TEXT("BP_%s"), *Preset.Name);
			UMetaHumanParametricGenerator::ExportCharacterToBlueprint(
				Character,
				TEXT("/Game/MyMetaHumans/Batch/Blueprints/"),
				BlueprintName
			);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("  ✗ %s failed"), *Preset.Name);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Batch creation complete: %d/%d successful"), SuccessCount, Presets.Num());
}

/**
 * 从蓝图或 Editor Utility Widget 调用的主函数
 */
UFUNCTION(BlueprintCallable, Category = "MetaHuman|Examples")
void RunAllExamples()
{
	Example1_CreateSlenderFemale();
	Example2_CreateMuscularMale();
	Example3_CreateShortRoundedCharacter();
	Example4_BatchCreateCharacters();
}
