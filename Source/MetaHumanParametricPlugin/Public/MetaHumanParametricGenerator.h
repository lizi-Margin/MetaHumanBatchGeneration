// Copyright Epic Games, Inc. All Rights Reserved.
// Example: Programmatic MetaHuman Character Generation with Parametric Body Control
//
// This file demonstrates how to:
// 1. Create a MetaHuman Character programmatically
// 2. Set body parameters (height, weight, proportions) using the parametric body system
// 3. Configure skin, eyes, makeup, and other appearance settings
// 4. Generate character assets and export to Blueprint
//
// NOTE: This is示例代码，不保证能直接编译。需要链接正确的模块依赖。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

// MetaHuman Character System Headers
#include "MetaHumanCharacter.h"
#include "MetaHumanCharacterEditorSubsystem.h"
#include "MetaHumanCharacterBodyIdentity.h"
#include "MetaHumanBodyType.h"
#include "MetaHumanCollection.h"

// Unreal Engine Headers
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Engine/Blueprint.h"
#include "Animation/Skeleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Factories/BlueprintFactory.h"
#include "UObject/SavePackage.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

#include "MetaHumanParametricGenerator.generated.h"

/**
 * 参数化身体配置结构
 * 使用身体测量约束系统来精确控制角色身材
 */
USTRUCT(BlueprintType)
struct FMetaHumanBodyParametricConfig
{
	GENERATED_BODY()

	// 身体类型（性别 + 身高 + 体型组合）
	// 格式: f/m（女/男） + srt/med/tal（矮/中/高） + nrw/ovw/unw（瘦/普通/胖）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body Type")
	EMetaHumanBodyType BodyType = EMetaHumanBodyType::f_med_nrw;  // 默认：女性 中等身高 正常体重

	// 全局变形强度 (0.0 - 1.0)
	// 控制整体身体变形的强度，1.0 为完全应用模型
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body Parameters", meta = (UIMin = "0.0", UIMax = "1.0"))
	float GlobalDeltaScale = 1.0f;

	// 是否使用参数化身体（true）还是固定身体类型（false）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body Type")
	bool bUseParametricBody = true;

	// 身体约束配置（用于精确控制身体各部位尺寸）
	// 这些约束会被转换为 FMetaHumanCharacterBodyConstraint 数组
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Body Constraints")
	TMap<FString, float> BodyMeasurements;  // 例如: {"Height": 175.0, "Chest": 95.0, "Waist": 70.0}

	FMetaHumanBodyParametricConfig()
	{
		// 默认身体测量值（单位：厘米）
		BodyMeasurements.Add(TEXT("Height"), 170.0f);          // 身高
		BodyMeasurements.Add(TEXT("Chest"), 90.0f);            // 胸围
		BodyMeasurements.Add(TEXT("Waist"), 75.0f);            // 腰围
		BodyMeasurements.Add(TEXT("Hips"), 95.0f);             // 臀围
		BodyMeasurements.Add(TEXT("ShoulderWidth"), 40.0f);    // 肩宽
		BodyMeasurements.Add(TEXT("ArmLength"), 60.0f);        // 臂长
		BodyMeasurements.Add(TEXT("LegLength"), 85.0f);        // 腿长
	}
};

/**
 * 皮肤和外观配置
 */
USTRUCT(BlueprintType)
struct FMetaHumanAppearanceConfig
{
	GENERATED_BODY()

	// 皮肤UV坐标 (控制肤色)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin", meta = (UIMin = "0.0", UIMax = "1.0"))
	float SkinToneU = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin", meta = (UIMin = "0.0", UIMax = "1.0"))
	float SkinToneV = 0.5f;

	// 皮肤粗糙度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skin", meta = (UIMin = "0.0", UIMax = "2.0"))
	float SkinRoughness = 1.06f;

	// 眼睛颜色配置
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyes")
	EMetaHumanCharacterEyesIrisPattern IrisPattern = EMetaHumanCharacterEyesIrisPattern::Iris001;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyes", meta = (UIMin = "0.0", UIMax = "1.0"))
	float IrisPrimaryColorU = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyes", meta = (UIMin = "0.0", UIMax = "1.0"))
	float IrisPrimaryColorV = 0.6f;

	// 睫毛类型
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyelashes")
	EMetaHumanCharacterEyelashesType EyelashesType = EMetaHumanCharacterEyelashesType::Thin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Eyelashes")
	bool bEnableEyelashGrooms = true;
};

/**
 * MetaHuman 参数化生成器
 *
 * 这个类展示了如何完全程序化地创建、配置和导出 MetaHuman 角色
 */
UCLASS(BlueprintType)
class UMetaHumanParametricGenerator : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 主要生成函数：创建一个完整的参数化 MetaHuman 角色
	 *
	 * @param CharacterName - 角色名称
	 * @param OutputPath - 输出路径 (例如: "/Game/MyCharacters/")
	 * @param BodyConfig - 身体参数配置
	 * @param AppearanceConfig - 外观配置
	 * @param OutCharacter - 输出：创建的角色资产
	 * @return 是否成功创建
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Generation")
	static bool GenerateParametricMetaHuman(
		const FString& CharacterName,
		const FString& OutputPath,
		const FMetaHumanBodyParametricConfig& BodyConfig,
		const FMetaHumanAppearanceConfig& AppearanceConfig,
		UMetaHumanCharacter*& OutCharacter);

	/**
	 * 从创建的角色生成蓝图
	 *
	 * @param Character - 源角色资产
	 * @param BlueprintPath - 蓝图保存路径
	 * @param BlueprintName - 蓝图名称
	 * @return 创建的蓝图资产
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Export")
	static UBlueprint* ExportCharacterToBlueprint(
		UMetaHumanCharacter* Character,
		const FString& BlueprintPath,
		const FString& BlueprintName);

	// ============================================================================
	// MetaHuman Cloud Services Authentication (新增)
	// ============================================================================

	/**
	 * 检查并确保用户已登录到 MetaHuman 云服务
	 * 如果未登录，会尝试自动登录（使用持久化凭据）或打开浏览器登录门户
	 *
	 * @return true if user is logged in, false if login failed
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Authentication")
	static bool EnsureCloudServicesLogin();

	/**
	 * 异步检查用户是否已登录
	 *
	 * @param OnCheckComplete Callback with boolean result (true = logged in)
	 */
	static void CheckCloudServicesLoginAsync(TFunction<void(bool)> OnCheckComplete);

	/**
	 * 异步执行云服务登录
	 * 先尝试持久化凭据登录，失败则打开浏览器账户门户登录
	 *
	 * @param OnLoginComplete Callback when login succeeds
	 * @param OnLoginFailed Callback when login fails
	 */
	static void LoginToCloudServicesAsync(TFunction<void()> OnLoginComplete, TFunction<void()> OnLoginFailed);

	/**
	 * 测试云服务认证功能（调试用）
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Debug")
	static void TestCloudAuthentication();

private: 
	static UMetaHumanCharacterEditorSubsystem* getEditorSubsystem();

private:
	// ========== 内部辅助函数 ==========

	/**
	 * 步骤 1: 创建基础 MetaHuman Character 资产
	 */
	static UMetaHumanCharacter* CreateBaseCharacter(
		const FString& PackagePath,
		const FString& CharacterName,
		EMetaHumanCharacterTemplateType TemplateType);

	/**
	 * 步骤 2: 配置身体参数（参数化系统）
	 */
	static bool ConfigureBodyParameters(
		UMetaHumanCharacter* Character,
		const FMetaHumanBodyParametricConfig& BodyConfig);

	/**
	 * 步骤 3: 配置外观（皮肤、眼睛、睫毛等）
	 */
	static bool ConfigureAppearance(
		UMetaHumanCharacter* Character,
		const FMetaHumanAppearanceConfig& AppearanceConfig);

	/**
	 * 步骤 4: 下载纹理源数据（新增）
	 * 在生成资产前下载所需的纹理源数据，避免 "Texture generated for assembly without source data" 错误
	 */
	static bool DownloadTextureSourceData(UMetaHumanCharacter* Character);

private:
	/**
	 * 下载纹理源数据的实际实现函数
	 * 线程安全的实现，处理后台线程调用
	 */
	static bool DownloadTextureSourceData_Impl(UMetaHumanCharacter* Character, UMetaHumanCharacterEditorSubsystem* EditorSubsystem);

	/**
	 * 步骤 5: 生成角色资产（网格、材质、纹理等）
	 */
	static bool GenerateCharacterAssets(
		UMetaHumanCharacter* Character,
		FMetaHumanCharacterGeneratedAssets& OutAssets);

	/**
	 * 步骤 6: 保存资产到磁盘
	 */
	static bool SaveCharacterAssets(
		UMetaHumanCharacter* Character,
		const FString& OutputPath,
		const FMetaHumanCharacterGeneratedAssets& GeneratedAssets);

	/**
	 * 辅助：将身体测量值转换为约束数组
	 */
	static TArray<FMetaHumanCharacterBodyConstraint> ConvertMeasurementsToConstraints(
		const TMap<FString, float>& Measurements);

	/**
	 * 辅助：创建蓝图并设置组件
	 */
	static UBlueprint* CreateBlueprintFromCharacter(
		UMetaHumanCharacter* Character,
		const FMetaHumanCharacterGeneratedAssets& Assets,
		const FString& PackagePath,
		const FString& BlueprintName);
};
