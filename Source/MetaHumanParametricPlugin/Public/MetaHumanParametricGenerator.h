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
#include "Item/MetaHumanDefaultGroomPipeline.h"

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

	// 质量级别 (Cinematic, High, Medium, Low)
	// 控制生成的角色资产的质量级别和管线配置
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Build Settings")
	EMetaHumanQualityLevel QualityLevel = EMetaHumanQualityLevel::Cinematic;

	FMetaHumanBodyParametricConfig()
	{
		// // 默认身体测量值（单位：厘米）
		// BodyMeasurements.Add(TEXT("Height"), 170.0f);          // 身高
		// BodyMeasurements.Add(TEXT("Chest"), 90.0f);            // 胸围
		// BodyMeasurements.Add(TEXT("Waist"), 75.0f);            // 腰围
		// BodyMeasurements.Add(TEXT("Hips"), 95.0f);             // 臀围
		// BodyMeasurements.Add(TEXT("ShoulderWidth"), 40.0f);    // 肩宽
		// BodyMeasurements.Add(TEXT("ArmLength"), 60.0f);        // 臂长
		// BodyMeasurements.Add(TEXT("LegLength"), 85.0f);        // 腿长
	}
};

USTRUCT(BlueprintType)
struct FMetaHumanWardrobeColorConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe Colors")
	FLinearColor PrimaryColorShirt = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wardrobe Colors")
	FLinearColor PrimaryColorShort = FLinearColor::Blue;

	FMetaHumanWardrobeColorConfig()
	{
		// Default colors
		PrimaryColorShirt = FLinearColor(0.8f, 0.8f, 0.8f, 1.0f); // Light gray
		PrimaryColorShort = FLinearColor(0.2f, 0.4f, 0.8f, 1.0f); // Blue
	}
};

USTRUCT()
struct FMetaHumanWardrobeConfig
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UMetaHumanDefaultGroomPipelineMaterialParameters> HairParameters;

	UPROPERTY()
	FMetaHumanWardrobeColorConfig ColorConfig;

	FMetaHumanWardrobeConfig()
	{
		HairParameters = NewObject<UMetaHumanDefaultGroomPipelineMaterialParameters>();
		ColorConfig = FMetaHumanWardrobeColorConfig();
	}
};

USTRUCT()
struct FMetaHumanAppearanceConfig
{
	GENERATED_BODY()

	UPROPERTY()
	FMetaHumanCharacterSkinSettings SkinSettings;

	UPROPERTY()
	FMetaHumanCharacterEyesSettings EyesSettings;

	UPROPERTY()
	FMetaHumanCharacterHeadModelSettings HeadModelSettings;

	UPROPERTY()
	FMetaHumanWardrobeConfig WardrobeConfig;
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
	// ============================================================================
	// Two-Step Generation Workflow (Recommended - Non-blocking)
	// ============================================================================

	/**
	 * Step 1: 准备并开始 Rigging 角色
	 * 此函数会：
	 * 1. 创建 MetaHuman Character 资产
	 * 2. 配置身体参数和外观
	 * 3. 下载纹理源数据
	 * 4. 启动 AutoRig（异步云服务）
	 * 5. 立即返回（不等待 AutoRig 完成）
	 *
	 * AutoRig 会在后台运行，完成后你可以调用 AssembleCharacter() 完成角色生成
	 *
	 * @param CharacterName - 角色名称
	 * @param OutputPath - 输出路径 (例如: "/Game/MyCharacters/")
	 * @param BodyConfig - 身体参数配置
	 * @param AppearanceConfig - 外观配置
	 * @param OutCharacter - 输出：创建的角色资产（未完成 rigging）
	 * @return 是否成功创建并启动 AutoRig
	 */
	static bool PrepareAndRigCharacter(
		const FString& CharacterName,
		const FString& OutputPath,
		const FMetaHumanBodyParametricConfig& BodyConfig,
		const FMetaHumanAppearanceConfig& AppearanceConfig,
		UMetaHumanCharacter*& OutCharacter);

	/**
	 * Step 2: 组装角色（在 AutoRig 完成后调用）
	 * 此函数会：
	 * 1. 检查角色是否已完成 rigging
	 * 2. 使用原生管线组装角色（生成网格、纹理、ABP 等）
	 * 3. 保存所有资产
	 *
	 * 注意：此函数必须在 AutoRig 完成后才能成功调用
	 *
	 * @param Character - 已完成 rigging 的角色资产
	 * @param OutputPath - 输出路径
	 * @param QualityLevel - 质量级别
	 * @return 是否成功组装角色
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Generation")
	static bool AssembleCharacter(
		UMetaHumanCharacter* Character,
		const FString& OutputPath,
		EMetaHumanQualityLevel QualityLevel);

	/**
	 * 检查角色的 Rigging 状态
	 *
	 * @param Character - 要检查的角色
	 * @return Rigging 状态：Unrigged, RigPending, Rigged
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Generation")
	static FString GetRiggingStatusString(UMetaHumanCharacter* Character);


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

	/**
	 * 添加衣橱物品到角色（通用方法）
	 *
	 * @param Character - 要添加物品的角色
	 * @param SlotName - 槽位名称 (例如: "Hair", "Outfits", "Beard", "Eyebrows", etc.)
	 * @param WardrobeItemPath - 衣橱物品资源路径 (例如: "/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_L_Straight.WI_Hair_L_Straight")
	 * @return 是否成功添加
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Wardrobe")
	static bool AddWardrobeItem(
		UMetaHumanCharacter* Character,
		const FName& SlotName,
		const FString& WardrobeItemPath);

	/**
	 * 添加头发到角色
	 *
	 * @param Character - 要添加头发的角色
	 * @param HairAssetPath - 头发资源路径 (例如: "/MetaHumanCharacter/Optional/Grooms/Bindings/Hair/WI_Hair_L_Straight")
	 * @return 是否成功添加
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Wardrobe")
	static bool AddHair(UMetaHumanCharacter* Character, const FString& HairAssetPath);

	/**
	 * 添加服装到角色
	 *
	 * @param Character - 要添加服装的角色
	 * @param ClothingAssetPath - 服装资源路径 (例如: "/MetaHumanCharacter/Optional/Clothing/WI_DefaultGarment")
	 * @return 是否成功添加
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Wardrobe")
	static bool AddClothing(UMetaHumanCharacter* Character, const FString& ClothingAssetPath);

	/**
	 * 移除槽位中的物品
	 *
	 * @param Character - 角色
	 * @param SlotName - 槽位名称 (例如: "Hair", "Outfits")
	 * @return 是否成功移除
	 */
	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Wardrobe")
	static bool RemoveWardrobeItem(UMetaHumanCharacter* Character, const FName& SlotName);

	/**
	 * 从指定目录随机选择一个衣橱物品
	 *
	 * @param SlotName - 槽位名称
	 * @param ContentPath - 内容路径 (例如: "/MetaHumanCharacter/Optional/Grooms/Bindings/Hair")
	 * @return 随机选择的物品路径（如果找到）
	 */
	static FString GetRandomWardrobeItemFromPath(const FName& SlotName, const FString& ContentPath);

	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Wardrobe")
	static bool ApplyHairParameters(
		UMetaHumanCharacter* Character,
		UMetaHumanDefaultGroomPipelineMaterialParameters* HairParams);

	UFUNCTION(BlueprintCallable, Category = "MetaHuman|Wardrobe")
	static bool ApplyWardrobeColorParameters(
		UMetaHumanCharacter* Character,
		const FMetaHumanWardrobeColorConfig& ColorConfig);

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

	/**
	 * 对角色进行 rigging
	 * NOTE: Uses synchronous waiting - MUST be called from background thread, NOT UI thread!
	 */
	static bool RigCharacter(UMetaHumanCharacter* Character);

private:
	/**
	 * 下载纹理源数据的实际实现函数
	 * 线程安全的实现，处理后台线程调用
	 */
	static bool DownloadTextureSourceData_Impl(UMetaHumanCharacter* Character, UMetaHumanCharacterEditorSubsystem* EditorSubsystem);

	/**
	 * 辅助：将身体测量值转换为约束数组
	 */
	static TArray<FMetaHumanCharacterBodyConstraint> ConvertMeasurementsToConstraints(
		const TMap<FString, float>& Measurements);

};
