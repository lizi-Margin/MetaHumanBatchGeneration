#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonReader.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "MetaHumanParametricGenerator.h"
#include "MetaHumanConfigSerializer.generated.h"

USTRUCT()
struct FMetaHumanGenerationSession
{
    GENERATED_BODY()

    UPROPERTY()
    FString SessionID;

    UPROPERTY()
    FDateTime Timestamp;

    UPROPERTY()
    FString CharacterName;

    UPROPERTY()
    FString OutputPath;

    UPROPERTY()
    FMetaHumanBodyParametricConfig BodyConfig;

    UPROPERTY()
    FMetaHumanAppearanceConfig AppearanceConfig;

    UPROPERTY()
    FString GenerationStatus;

    FMetaHumanGenerationSession()
    {
        Timestamp = FDateTime::Now();
        GenerationStatus = TEXT("Pending");
    }
};

UCLASS(BlueprintType)
class METAHUMANPARAMETRICPLUGIN_API UMetaHumanConfigSerializer : public UObject
{
    GENERATED_BODY()

public:
    UMetaHumanConfigSerializer();

    static bool SaveBodyConfigToJson(const FMetaHumanBodyParametricConfig& BodyConfig, const FString& FilePath);
    static bool LoadBodyConfigFromJson(FMetaHumanBodyParametricConfig& OutBodyConfig, const FString& FilePath);

    static bool SaveAppearanceConfigToJson(const FMetaHumanAppearanceConfig& AppearanceConfig, const FString& FilePath);
    static bool LoadAppearanceConfigFromJson(FMetaHumanAppearanceConfig& OutAppearanceConfig, const FString& FilePath);

    static bool SaveFullSessionToJson(const FMetaHumanGenerationSession& Session, const FString& FilePath);
    static bool LoadFullSessionFromJson(FMetaHumanGenerationSession& OutSession, const FString& FilePath);

    static FString SerializeBodyConfigToString(const FMetaHumanBodyParametricConfig& BodyConfig);
    static FString SerializeAppearanceConfigToString(const FMetaHumanAppearanceConfig& AppearanceConfig);

    static bool DeserializeBodyConfigFromString(const FString& JsonString, FMetaHumanBodyParametricConfig& OutBodyConfig);
    static bool DeserializeAppearanceConfigFromString(const FString& JsonString, FMetaHumanAppearanceConfig& OutAppearanceConfig);

    static bool SaveGenerationSession(
        const FString& CharacterName,
        const FString& OutputPath,
        const FMetaHumanBodyParametricConfig& BodyConfig,
        const FMetaHumanAppearanceConfig& AppearanceConfig,
        const FString& Status = TEXT("Started"));

    static bool UpdateSessionStatus(const FString& CharacterName, const FString& NewStatus);

    static FMetaHumanGenerationSession CreateSessionFromCurrentGeneration(
        const FString& CharacterName,
        const FString& OutputPath,
        const FMetaHumanBodyParametricConfig& BodyConfig,
        const FMetaHumanAppearanceConfig& AppearanceConfig);

    static FString GenerateSessionID(const FString& CharacterName);
    static FString GetDefaultConfigDirectory();
    static FString GetSessionFilePath(const FString& CharacterName);

private:
    static TSharedPtr<FJsonObject> BodyConfigToJson(const FMetaHumanBodyParametricConfig& BodyConfig);
    static TSharedPtr<FJsonObject> AppearanceConfigToJson(const FMetaHumanAppearanceConfig& AppearanceConfig);
    static TSharedPtr<FJsonObject> WardrobeConfigToJson(const FMetaHumanWardrobeConfig& WardrobeConfig);
    static TSharedPtr<FJsonObject> WardrobeColorConfigToJson(const FMetaHumanWardrobeColorConfig& ColorConfig);
    static TSharedPtr<FJsonObject> SessionToJson(const FMetaHumanGenerationSession& Session);

    static bool JsonToBodyConfig(const TSharedPtr<FJsonObject>& JsonObject, FMetaHumanBodyParametricConfig& OutBodyConfig);
    static bool JsonToAppearanceConfig(const TSharedPtr<FJsonObject>& JsonObject, FMetaHumanAppearanceConfig& OutAppearanceConfig);
    static bool JsonToWardrobeConfig(const TSharedPtr<FJsonObject>& JsonObject, FMetaHumanWardrobeConfig& OutWardrobeConfig);
    static bool JsonToWardrobeColorConfig(const TSharedPtr<FJsonObject>& JsonObject, FMetaHumanWardrobeColorConfig& OutColorConfig);
    static bool JsonToSession(const TSharedPtr<FJsonObject>& JsonObject, FMetaHumanGenerationSession& OutSession);

    static bool WriteJsonToFile(const TSharedPtr<FJsonObject>& JsonObject, const FString& FilePath);
    static TSharedPtr<FJsonObject> ReadJsonFromFile(const FString& FilePath);

    static TSharedPtr<FJsonObject> LinearColorToJson(const FLinearColor& Color);
    static FLinearColor JsonToLinearColor(const TSharedPtr<FJsonObject>& JsonObject);
};