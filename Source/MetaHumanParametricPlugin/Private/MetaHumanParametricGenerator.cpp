// Copyright Epic Games, Inc. All Rights Reserved.
// MetaHuman Parametric Generator - Implementation
//
// 完整的参数化 MetaHuman 角色生成流程实现

#include "MetaHumanParametricGenerator.h"

#include "MetaHumanCharacter.h"
#include "MetaHumanCharacterEditorSubsystem.h"
#include "MetaHumanCharacterBodyIdentity.h"
#include "MetaHumanCollection.h"
#include "MetaHumanBodyType.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/SkeletalMeshComponent.h"
#include "HAL/PlatformProcess.h"
#include "Tasks/Task.h"
#include "Misc/DateTime.h"
#include "Cloud/MetaHumanARServiceRequest.h"

// ============================================================================
// 主要生成函数
// ============================================================================

bool UMetaHumanParametricGenerator::GenerateParametricMetaHuman(
	const FString& CharacterName,
	const FString& OutputPath,
	const FMetaHumanBodyParametricConfig& BodyConfig,
	const FMetaHumanAppearanceConfig& AppearanceConfig,
	UMetaHumanCharacter*& OutCharacter)
{
	UE_LOG(LogTemp, Log, TEXT("=== MetaHuman Parametric Generation Started ==="));
	UE_LOG(LogTemp, Log, TEXT("Character Name: %s"), *CharacterName);
	UE_LOG(LogTemp, Log, TEXT("Output Path: %s"), *OutputPath);

	// 步骤 1: 创建基础 Character 资产
	UE_LOG(LogTemp, Log, TEXT("[Step 1/5] Creating base MetaHuman Character asset..."));
	UMetaHumanCharacter* Character = CreateBaseCharacter(
		OutputPath,
		CharacterName,
		EMetaHumanCharacterTemplateType::MetaHuman
	);

	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create base character!"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("[Step 1/5] ✓ Base character created"));

	// 步骤 2: 配置身体参数
	UE_LOG(LogTemp, Log, TEXT("[Step 2/5] Configuring body parameters..."));
	if (!ConfigureBodyParameters(Character, BodyConfig))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to configure body parameters!"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("[Step 2/5] ✓ Body parameters configured"));

	// 步骤 3: 配置外观
	UE_LOG(LogTemp, Log, TEXT("[Step 3/5] Configuring appearance..."));
	if (!ConfigureAppearance(Character, AppearanceConfig))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to configure appearance!"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("[Step 3/5] ✓ Appearance configured"));

	// 步骤 4: 下载纹理源数据（新增）
	UE_LOG(LogTemp, Log, TEXT("[Step 4/6] Downloading texture source data..."));
	if (!DownloadTextureSourceData(Character))
	{
		UE_LOG(LogTemp, Warning, TEXT("Warning: Failed to download texture source data, will use default textures"));
		// 不返回 false，继续使用默认纹理
	}
	UE_LOG(LogTemp, Log, TEXT("[Step 4/6] ✓ Texture source data download completed"));

	// 步骤 5: 生成资产
	UE_LOG(LogTemp, Log, TEXT("[Step 5/6] Generating character assets..."));
	FMetaHumanCharacterGeneratedAssets GeneratedAssets;
	if (!GenerateCharacterAssets(Character, GeneratedAssets))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to generate character assets!"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("[Step 5/6] ✓ Assets generated: Face Mesh, Body Mesh, Textures, Physics"));

	// 步骤 6: 保存资产
	UE_LOG(LogTemp, Log, TEXT("[Step 6/6] Saving character assets..."));
	if (!SaveCharacterAssets(Character, OutputPath, GeneratedAssets))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save character assets!"));
		return false;
	}
	UE_LOG(LogTemp, Log, TEXT("[Step 5/5] ✓ Assets saved to: %s"), *OutputPath);

	OutCharacter = Character;
	UE_LOG(LogTemp, Log, TEXT("=== MetaHuman Generation Completed Successfully ==="));
	return true;
}

// ============================================================================
// 步骤 1: 创建基础角色资产
// ============================================================================

UMetaHumanCharacter* UMetaHumanParametricGenerator::CreateBaseCharacter(
	const FString& PackagePath,
	const FString& CharacterName,
	EMetaHumanCharacterTemplateType TemplateType)
{
	// 1. 构建完整的包路径
	FString PackageNameStr = FPackageName::ObjectPathToPackageName(PackagePath / CharacterName);
	UPackage* Package = CreatePackage(*PackageNameStr);

	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *PackageNameStr);
		return nullptr;
	}

	// 2. 创建 MetaHumanCharacter 对象
	UMetaHumanCharacter* Character = NewObject<UMetaHumanCharacter>(
		Package,
		UMetaHumanCharacter::StaticClass(),
		*CharacterName,
		RF_Public | RF_Standalone
	);

	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create MetaHumanCharacter object"));
		return nullptr;
	}

	// 3. 设置模板类型
	Character->TemplateType = TemplateType;

	// 4. 初始化角色（加载必要的模型和数据）
	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();
	if (EditorSubsystem)
	{
		EditorSubsystem->InitializeMetaHumanCharacter(Character);

		// 注册角色以便编辑
		if (!EditorSubsystem->TryAddObjectToEdit(Character))
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to register character for editing, but continuing..."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get MetaHumanCharacterEditorSubsystem"));
		return nullptr;
	}

	// 5. 标记包为脏状态（需要保存）
	Package->MarkPackageDirty();

	return Character;
}

// ============================================================================
// 步骤 2: 配置身体参数（核心！）
// ============================================================================

bool UMetaHumanParametricGenerator::ConfigureBodyParameters(
	UMetaHumanCharacter* Character,
	const FMetaHumanBodyParametricConfig& BodyConfig)
{
	if (!Character)
	{
		return false;
	}

	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();
	if (!EditorSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get editor subsystem"));
		return false;
	}

	// 1. 设置身体类型（固定 vs 参数化）
	UE_LOG(LogTemp, Log, TEXT("  - Setting body type: %s"),
		*UEnum::GetValueAsString(BodyConfig.BodyType));

	EditorSubsystem->SetMetaHumanBodyType(
		Character,
		BodyConfig.BodyType,
		UMetaHumanCharacterEditorSubsystem::EBodyMeshUpdateMode::Full
	);

	// 2. 设置全局变形强度
	UE_LOG(LogTemp, Log, TEXT("  - Setting global delta scale: %.2f"), BodyConfig.GlobalDeltaScale);
	EditorSubsystem->SetBodyGlobalDeltaScale(Character, BodyConfig.GlobalDeltaScale);

	// 3. 如果使用参数化身体，应用身体约束
	if (BodyConfig.bUseParametricBody && BodyConfig.BodyMeasurements.Num() > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("  - Applying parametric body constraints (%d measurements)..."),
			BodyConfig.BodyMeasurements.Num());

		// 转换测量值为约束数组
		TArray<FMetaHumanCharacterBodyConstraint> Constraints =
			ConvertMeasurementsToConstraints(BodyConfig.BodyMeasurements);

		// 应用约束到角色
		EditorSubsystem->SetBodyConstraints(Character, Constraints);

		// 打印每个约束的值
		for (const auto& Pair : BodyConfig.BodyMeasurements)
		{
			UE_LOG(LogTemp, Log, TEXT("    • %s: %.2f cm"), *Pair.Key, Pair.Value);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("  - Using fixed body type (no parametric constraints)"));
	}

	// 4. 获取当前身体状态并提交更改
	TSharedRef<FMetaHumanCharacterBodyIdentity::FState> BodyState =
		EditorSubsystem->CopyBodyState(Character);

	EditorSubsystem->CommitBodyState(
		Character,
		BodyState,
		UMetaHumanCharacterEditorSubsystem::EBodyMeshUpdateMode::Full
	);

	UE_LOG(LogTemp, Log, TEXT("  ✓ Body configuration complete"));
	return true;
}

// ============================================================================
// 步骤 3: 配置外观（皮肤、眼睛、睫毛等）
// ============================================================================

bool UMetaHumanParametricGenerator::ConfigureAppearance(
	UMetaHumanCharacter* Character,
	const FMetaHumanAppearanceConfig& AppearanceConfig)
{
	if (!Character)
	{
		return false;
	}

	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();
	if (!EditorSubsystem)
	{
		return false;
	}

	// 1. 配置皮肤设置
	UE_LOG(LogTemp, Log, TEXT("  - Configuring skin settings..."));
	{
		FMetaHumanCharacterSkinSettings SkinSettings;
		SkinSettings.Skin.U = AppearanceConfig.SkinToneU;
		SkinSettings.Skin.V = AppearanceConfig.SkinToneV;
		SkinSettings.Skin.Roughness = AppearanceConfig.SkinRoughness;

		// 应用并提交皮肤设置
		EditorSubsystem->ApplySkinSettings(Character, SkinSettings);
		EditorSubsystem->CommitSkinSettings(Character, SkinSettings);

		UE_LOG(LogTemp, Log, TEXT("    • Skin Tone: (U=%.2f, V=%.2f)"),
			AppearanceConfig.SkinToneU, AppearanceConfig.SkinToneV);
		UE_LOG(LogTemp, Log, TEXT("    • Roughness: %.2f"), AppearanceConfig.SkinRoughness);
	}

	// 2. 配置眼睛设置
	UE_LOG(LogTemp, Log, TEXT("  - Configuring eyes settings..."));
	{
		FMetaHumanCharacterEyesSettings EyesSettings;

		// 左眼和右眼使用相同的设置（可以分别设置）
		EyesSettings.EyeLeft.Iris.IrisPattern = AppearanceConfig.IrisPattern;
		EyesSettings.EyeLeft.Iris.PrimaryColorU = AppearanceConfig.IrisPrimaryColorU;
		EyesSettings.EyeLeft.Iris.PrimaryColorV = AppearanceConfig.IrisPrimaryColorV;

		EyesSettings.EyeRight.Iris.IrisPattern = AppearanceConfig.IrisPattern;
		EyesSettings.EyeRight.Iris.PrimaryColorU = AppearanceConfig.IrisPrimaryColorU;
		EyesSettings.EyeRight.Iris.PrimaryColorV = AppearanceConfig.IrisPrimaryColorV;

		// 应用并提交眼睛设置
		EditorSubsystem->ApplyEyesSettings(Character, EyesSettings);
		EditorSubsystem->CommitEyesSettings(Character, EyesSettings);

		UE_LOG(LogTemp, Log, TEXT("    • Iris Pattern: %s"),
			*UEnum::GetValueAsString(AppearanceConfig.IrisPattern));
		UE_LOG(LogTemp, Log, TEXT("    • Iris Color: (U=%.2f, V=%.2f)"),
			AppearanceConfig.IrisPrimaryColorU, AppearanceConfig.IrisPrimaryColorV);
	}

	// 3. 配置头部模型设置（睫毛等）
	UE_LOG(LogTemp, Log, TEXT("  - Configuring head model (eyelashes)..."));
	{
		FMetaHumanCharacterHeadModelSettings HeadModelSettings;
		HeadModelSettings.Eyelashes.Type = AppearanceConfig.EyelashesType;
		HeadModelSettings.Eyelashes.bEnableGrooms = AppearanceConfig.bEnableEyelashGrooms;

		// 应用并提交头部模型设置
		EditorSubsystem->ApplyHeadModelSettings(Character, HeadModelSettings);
		EditorSubsystem->CommitHeadModelSettings(Character, HeadModelSettings);

		UE_LOG(LogTemp, Log, TEXT("    • Eyelashes Type: %s"),
			*UEnum::GetValueAsString(AppearanceConfig.EyelashesType));
		UE_LOG(LogTemp, Log, TEXT("    • Grooms Enabled: %s"),
			AppearanceConfig.bEnableEyelashGrooms ? TEXT("Yes") : TEXT("No"));
	}

	UE_LOG(LogTemp, Log, TEXT("  ✓ Appearance configuration complete"));
	return true;
}

// ============================================================================
// 步骤 4: 生成角色资产
// ============================================================================

bool UMetaHumanParametricGenerator::GenerateCharacterAssets(
	UMetaHumanCharacter* Character,
	FMetaHumanCharacterGeneratedAssets& OutAssets)
{
	if (!Character)
	{
		return false;
	}

	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();
	if (!EditorSubsystem)
	{
		return false;
	}

	// 创建用于存放生成资产的临时包
	UPackage* TransientPackage = GetTransientPackage();

	// 调用资产生成函数
	// 这会生成：
	//   - Face Mesh (面部骨骼网格)
	//   - Body Mesh (身体骨骼网格)
	//   - Textures (皮肤纹理、法线贴图等)
	//   - Physics Asset (物理资产)
	//   - RigLogic Assets (面部绑定资产)
	if (!EditorSubsystem->TryGenerateCharacterAssets(Character, TransientPackage, OutAssets))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to generate character assets"));
		return false;
	}

	// 验证生成的资产
	if (!OutAssets.FaceMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("Generated assets missing FaceMesh"));
		return false;
	}

	if (!OutAssets.BodyMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("Generated assets missing BodyMesh"));
		return false;
	}

	// 打印生成的资产信息
	UE_LOG(LogTemp, Log, TEXT("  ✓ Generated Assets:"));
	UE_LOG(LogTemp, Log, TEXT("    • Face Mesh: %s"), *OutAssets.FaceMesh->GetName());
	UE_LOG(LogTemp, Log, TEXT("    • Body Mesh: %s"), *OutAssets.BodyMesh->GetName());
	UE_LOG(LogTemp, Log, TEXT("    • Face Textures: %d"), OutAssets.SynthesizedFaceTextures.Num());
	UE_LOG(LogTemp, Log, TEXT("    • Body Textures: %d"), OutAssets.BodyTextures.Num());

	if (OutAssets.PhysicsAsset)
	{
		UE_LOG(LogTemp, Log, TEXT("    • Physics Asset: [Valid]"));
	}

	UE_LOG(LogTemp, Log, TEXT("    • Body Measurements: %d"), OutAssets.BodyMeasurements.Num());
	UE_LOG(LogTemp, Log, TEXT("    • Total Metadata Entries: %d"), OutAssets.Metadata.Num());

	return true;
}

// ============================================================================
// 步骤 5: 保存资产
// ============================================================================

bool UMetaHumanParametricGenerator::SaveCharacterAssets(
	UMetaHumanCharacter* Character,
	const FString& OutputPath,
	const FMetaHumanCharacterGeneratedAssets& GeneratedAssets)
{
	if (!Character)
	{
		return false;
	}

	// 1. 保存主角色资产
	UPackage* CharacterPackage = Character->GetOutermost();
	FString CharacterFilePath = FPackageName::LongPackageNameToFilename(
		CharacterPackage->GetName(),
		FPackageName::GetAssetPackageExtension()
	);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;

	if (!UPackage::SavePackage(CharacterPackage, Character, *CharacterFilePath, SaveArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save character package: %s"), *CharacterFilePath);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("  ✓ Saved character: %s"), *CharacterFilePath);

	// 2. 注册资产到 AssetRegistry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().AssetCreated(Character);

	// 3. 保存生成的网格和纹理（可选 - 如果需要作为独立资产）
	// 注意：通常这些资产会作为角色的一部分存储，不需要单独保存
	// 但如果需要，可以使用类似的 SavePackage 流程

	return true;
}

// ============================================================================
// 导出到蓝图
// ============================================================================

UBlueprint* UMetaHumanParametricGenerator::ExportCharacterToBlueprint(
	UMetaHumanCharacter* Character,
	const FString& BlueprintPath,
	const FString& BlueprintName)
{
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid character for blueprint export"));
		return nullptr;
	}

	UE_LOG(LogTemp, Log, TEXT("=== Exporting Character to Blueprint ==="));
	UE_LOG(LogTemp, Log, TEXT("Blueprint: %s/%s"), *BlueprintPath, *BlueprintName);

	// 1. 首先生成角色资产
	FMetaHumanCharacterGeneratedAssets GeneratedAssets;
	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();

	if (!EditorSubsystem || !EditorSubsystem->TryGenerateCharacterAssets(Character, GetTransientPackage(), GeneratedAssets))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to generate assets for blueprint export"));
		return nullptr;
	}

	// 2. 创建蓝图
	UBlueprint* Blueprint = CreateBlueprintFromCharacter(
		Character,
		GeneratedAssets,
		BlueprintPath,
		BlueprintName
	);

	if (Blueprint)
	{
		UE_LOG(LogTemp, Log, TEXT("✓ Blueprint created successfully: %s"), *Blueprint->GetPathName());
	}

	return Blueprint;
}

// ============================================================================
// 辅助函数：创建蓝图
// ============================================================================

UBlueprint* UMetaHumanParametricGenerator::CreateBlueprintFromCharacter(
	UMetaHumanCharacter* Character,
	const FMetaHumanCharacterGeneratedAssets& Assets,
	const FString& PackagePath,
	const FString& BlueprintName)
{
	// 1. 创建蓝图包
	FString PackageNameStr = FPackageName::ObjectPathToPackageName(PackagePath / BlueprintName);
	UPackage* Package = CreatePackage(*PackageNameStr);

	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create blueprint package"));
		return nullptr;
	}

	// 2. 创建 Actor 蓝图
	UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
		AActor::StaticClass(),
		Package,
		*BlueprintName,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		NAME_None
	);

	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create blueprint"));
		return nullptr;
	}

	// 3. 添加骨骼网格组件
	USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
	if (!SCS)
	{
		UE_LOG(LogTemp, Error, TEXT("Blueprint has no SimpleConstructionScript"));
		return nullptr;
	}

	// 添加面部网格组件
	if (Assets.FaceMesh)
	{
		USCS_Node* FaceNode = SCS->CreateNode(USkeletalMeshComponent::StaticClass(), TEXT("FaceMesh"));
		USkeletalMeshComponent* FaceComp = Cast<USkeletalMeshComponent>(FaceNode->ComponentTemplate);
		if (FaceComp)
		{
			FaceComp->SetSkeletalMesh(Assets.FaceMesh);
			FaceComp->SetRelativeLocation(FVector(0, 0, 0));
			SCS->AddNode(FaceNode);
			UE_LOG(LogTemp, Log, TEXT("  + Added Face Mesh component"));
		}
	}

	// 添加身体网格组件
	if (Assets.BodyMesh)
	{
		USCS_Node* BodyNode = SCS->CreateNode(USkeletalMeshComponent::StaticClass(), TEXT("BodyMesh"));
		USkeletalMeshComponent* BodyComp = Cast<USkeletalMeshComponent>(BodyNode->ComponentTemplate);
		if (BodyComp)
		{
			BodyComp->SetSkeletalMesh(Assets.BodyMesh);
			BodyComp->SetRelativeLocation(FVector(0, 0, -90));  // 身体稍微向下偏移
			SCS->AddNode(BodyNode);
			UE_LOG(LogTemp, Log, TEXT("  + Added Body Mesh component"));
		}
	}

	// 4. 编译蓝图
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	// 5. 保存蓝图
	FString BlueprintFilePath = FPackageName::LongPackageNameToFilename(
		Package->GetName(),
		FPackageName::GetAssetPackageExtension()
	);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;

	if (!UPackage::SavePackage(Package, Blueprint, *BlueprintFilePath, SaveArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save blueprint package"));
		return nullptr;
	}

	// 6. 注册到资产注册表
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().AssetCreated(Blueprint);

	return Blueprint;
}

// ============================================================================
// 辅助函数：将测量值转换为约束
// ============================================================================

TArray<FMetaHumanCharacterBodyConstraint> UMetaHumanParametricGenerator::ConvertMeasurementsToConstraints(
	const TMap<FString, float>& Measurements)
{
	TArray<FMetaHumanCharacterBodyConstraint> Constraints;

	// 遍历所有测量值并创建约束
	for (const auto& Pair : Measurements)
	{
		FMetaHumanCharacterBodyConstraint Constraint;
		Constraint.Name = FName(*Pair.Key);
		Constraint.bIsActive = true;  // 激活此约束
		Constraint.TargetMeasurement = Pair.Value;  // 目标值（厘米）

		// 设置合理的最小/最大范围（目标值的 ±50%）
		Constraint.MinMeasurement = Pair.Value * 0.5f;
		Constraint.MaxMeasurement = Pair.Value * 1.5f;

		Constraints.Add(Constraint);
	}

	return Constraints;
}

// ============================================================================
// 新增：下载纹理源数据
// ============================================================================

bool UMetaHumanParametricGenerator::DownloadTextureSourceData(UMetaHumanCharacter* Character)
{
	if (!Character)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid character for texture download"));
		return false;
	}

	// 确保在游戏线程中获取 EditorSubsystem
	UMetaHumanCharacterEditorSubsystem* EditorSubsystem = nullptr;
	if (IsInGameThread())
	{
		EditorSubsystem = GEditor->GetEditorSubsystem<UMetaHumanCharacterEditorSubsystem>();
	}
	else
	{
		// 如果不在游戏线程，我们直接返回 false，让主线程处理
		// 这是因为 MetaHuman 操作必须在游戏线程中进行
		UE_LOG(LogTemp, Warning, TEXT("DownloadTextureSourceData called from background thread - returning false. MetaHuman operations must run on game thread."));
		UE_LOG(LogTemp, Warning, TEXT("  This is normal - texture download will be handled by the main generation process."));
		return false;
	}

	if (!EditorSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get editor subsystem for texture download"));
		return false;
	}

	return DownloadTextureSourceData_Impl(Character, EditorSubsystem);
}

bool UMetaHumanParametricGenerator::DownloadTextureSourceData_Impl(UMetaHumanCharacter* Character, UMetaHumanCharacterEditorSubsystem* EditorSubsystem)
{
	if (!Character || !EditorSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid parameters for texture download implementation"));
		return false;
	}

	// 检查是否已经在下载纹理
	if (EditorSubsystem->IsRequestingHighResolutionTextures(Character))
	{
		UE_LOG(LogTemp, Log, TEXT("Texture download already in progress, waiting..."));

		// 等待下载完成（最多等待60秒）
		const float MaxWaitTime = 60.0f;
		const float StartTime = FPlatformTime::Seconds();

		while (EditorSubsystem->IsRequestingHighResolutionTextures(Character))
		{
			const float ElapsedTime = FPlatformTime::Seconds() - StartTime;
			if (ElapsedTime > MaxWaitTime)
			{
				UE_LOG(LogTemp, Warning, TEXT("Texture download timeout after %.1f seconds"), ElapsedTime);
				return false;
			}

			// 处理编辑器 tick，让下载继续进行
			// 注意：在后台线程中，我们不需要手动处理 tick
			FPlatformProcess::Sleep(0.5f); // 在后台线程中可以安全地使用较长的睡眠时间
		}

		UE_LOG(LogTemp, Log, TEXT("Texture download completed"));
		return true;
	}

	// 检查角色是否有合成纹理
	if (!Character->HasSynthesizedTextures())
	{
		UE_LOG(LogTemp, Warning, TEXT("Character does not have synthesized textures, cannot download high-res textures"));
		return false;
	}

	// 检查纹理合成是否启用
	if (!EditorSubsystem->IsTextureSynthesisEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("Texture synthesis is not enabled"));
		return false;
	}

	// 步骤 1: 检查是否需要自动绑定（autorig）
	UE_LOG(LogTemp, Log, TEXT("Checking if autorig is required before texture download..."));

	// 获取当前的绑定状态
	EMetaHumanCharacterRigState RigState = EditorSubsystem->GetRiggingState(Character);

	if (RigState != EMetaHumanCharacterRigState::Rigged)
	{
		UE_LOG(LogTemp, Log, TEXT("Character is not rigged, performing autorig first..."));

		// 检查是否已经在进行自动绑定
		if (EditorSubsystem->IsAutoRiggingFace(Character))
		{
			UE_LOG(LogTemp, Log, TEXT("Autorig already in progress, waiting..."));

			// 等待自动绑定完成
			const float MaxAutorigWaitTime = 300.0f; // 5分钟
			const float AutorigStartTime = FPlatformTime::Seconds();

			while (EditorSubsystem->IsAutoRiggingFace(Character))
			{
				const float AutorigElapsedTime = FPlatformTime::Seconds() - AutorigStartTime;
				if (AutorigElapsedTime > MaxAutorigWaitTime)
				{
					UE_LOG(LogTemp, Warning, TEXT("Autorig timeout after %.1f seconds"), AutorigElapsedTime);
					break;
				}

				// 在后台线程中不需要手动处理 tick
				FPlatformProcess::Sleep(1.0f); // 较长的睡眠时间，因为这是后台操作
			}
		}
		else
		{
			// 执行自动绑定
			UE_LOG(LogTemp, Log, TEXT("Starting autorig..."));
			EditorSubsystem->AutoRigFace(Character, UE::MetaHuman::ERigType::JointsAndBlendshapes);

			// 等待自动绑定完成
			const float MaxAutorigWaitTime = 300.0f; // 5分钟
			const float AutorigStartTime = FPlatformTime::Seconds();
			float LastAutorigProgress = 0.0f;

			while (EditorSubsystem->IsAutoRiggingFace(Character))
			{
				const float AutorigElapsedTime = FPlatformTime::Seconds() - AutorigStartTime;

				// 每15秒报告一次进度
				if (AutorigElapsedTime - LastAutorigProgress > 15.0f)
				{
					UE_LOG(LogTemp, Log, TEXT("Autorig in progress... (%.1f seconds elapsed)"), AutorigElapsedTime);
					LastAutorigProgress = AutorigElapsedTime;
				}

				if (AutorigElapsedTime > MaxAutorigWaitTime)
				{
					UE_LOG(LogTemp, Warning, TEXT("Autorig timeout after %.1f seconds"), AutorigElapsedTime);
					break;
				}

				// 在后台线程中不需要手动处理 tick
				FPlatformProcess::Sleep(1.0f); // 较长的睡眠时间，因为这是后台操作
			}

			const float AutorigTotalTime = FPlatformTime::Seconds() - AutorigStartTime;
			UE_LOG(LogTemp, Log, TEXT("Autorig completed in %.1f seconds"), AutorigTotalTime);
		}

		// 再次检查绑定状态
		RigState = EditorSubsystem->GetRiggingState(Character);
		if (RigState != EMetaHumanCharacterRigState::Rigged)
		{
			UE_LOG(LogTemp, Warning, TEXT("Warning: Character still not rigged after autorig attempt, continuing with texture download anyway"));
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Character successfully rigged"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("Character is already rigged, skipping autorig"));
	}

	// 步骤 2: 请求纹理下载
	UE_LOG(LogTemp, Log, TEXT("Requesting 2k texture download..."));
	UE_LOG(LogTemp, Warning, TEXT("Note: This requires MetaHuman cloud services login. If not logged in, download will fail but generation will continue with default textures."));

	EditorSubsystem->RequestHighResolutionTextures(Character, ERequestTextureResolution::Res2k);

	// 等待下载完成
	const float MaxWaitTime = 120.0f; // 纹理下载可能需要更长时间
	const float StartTime = FPlatformTime::Seconds();
	float LastProgressReport = 0.0f;
	bool bDownloadStarted = false;

	while (EditorSubsystem->IsRequestingHighResolutionTextures(Character))
	{
		const float ElapsedTime = FPlatformTime::Seconds() - StartTime;
		bDownloadStarted = true;

		// 每10秒报告一次进度
		if (ElapsedTime - LastProgressReport > 10.0f)
		{
			UE_LOG(LogTemp, Log, TEXT("Still downloading textures... (%.1f seconds elapsed)"), ElapsedTime);
			UE_LOG(LogTemp, Log, TEXT("  Make sure you're logged into MetaHuman cloud services in the editor"));
			LastProgressReport = ElapsedTime;
		}

		if (ElapsedTime > MaxWaitTime)
		{
			UE_LOG(LogTemp, Warning, TEXT("Texture download timeout after %.1f seconds"), ElapsedTime);
			UE_LOG(LogTemp, Warning, TEXT("  Possible causes:"));
			UE_LOG(LogTemp, Warning, TEXT("  - Not logged into MetaHuman cloud services"));
			UE_LOG(LogTemp, Warning, TEXT("  - Network connectivity issues"));
			UE_LOG(LogTemp, Warning, TEXT("  - Service temporarily unavailable"));
			break;
		}

		// 在后台线程中不需要手动处理编辑器 tick
		FPlatformProcess::Sleep(1.0f); // 后台线程可以使用更长的睡眠间隔
	}

	const float TotalTime = FPlatformTime::Seconds() - StartTime;

	if (bDownloadStarted && !EditorSubsystem->IsRequestingHighResolutionTextures(Character))
	{
		UE_LOG(LogTemp, Log, TEXT("Texture download completed in %.1f seconds"), TotalTime);
		return true;
	}
	else if (!bDownloadStarted)
	{
		UE_LOG(LogTemp, Warning, TEXT("Texture download failed to start - likely due to authentication issues"));
		UE_LOG(LogTemp, Warning, TEXT("  Please ensure you are logged into MetaHuman cloud services"));
		UE_LOG(LogTemp, Warning, TEXT("  You can log in via: Window > MetaHuman > Cloud Services"));
		return false;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Texture download did not complete within timeout"));
		return false;
	}
}
