#include "MetaHumanConfigSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformFilemanager.h"

UMetaHumanConfigSerializer::UMetaHumanConfigSerializer()
{
}

bool UMetaHumanConfigSerializer::SaveBodyConfigToJson(const FMetaHumanBodyParametricConfig& BodyConfig, const FString& FilePath)
{
    TSharedPtr<FJsonObject> JsonObject = BodyConfigToJson(BodyConfig);
    if (!JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to convert body config to JSON"));
        return false;
    }

    return WriteJsonToFile(JsonObject, FilePath);
}

bool UMetaHumanConfigSerializer::LoadBodyConfigFromJson(FMetaHumanBodyParametricConfig& OutBodyConfig, const FString& FilePath)
{
    TSharedPtr<FJsonObject> JsonObject = ReadJsonFromFile(FilePath);
    if (!JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to read JSON from file: %s"), *FilePath);
        return false;
    }

    return JsonToBodyConfig(JsonObject, OutBodyConfig);
}

bool UMetaHumanConfigSerializer::SaveAppearanceConfigToJson(const FMetaHumanAppearanceConfig& AppearanceConfig, const FString& FilePath)
{
    TSharedPtr<FJsonObject> JsonObject = AppearanceConfigToJson(AppearanceConfig);
    if (!JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to convert appearance config to JSON"));
        return false;
    }

    return WriteJsonToFile(JsonObject, FilePath);
}

bool UMetaHumanConfigSerializer::LoadAppearanceConfigFromJson(FMetaHumanAppearanceConfig& OutAppearanceConfig, const FString& FilePath)
{
    TSharedPtr<FJsonObject> JsonObject = ReadJsonFromFile(FilePath);
    if (!JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to read JSON from file: %s"), *FilePath);
        return false;
    }

    return JsonToAppearanceConfig(JsonObject, OutAppearanceConfig);
}

bool UMetaHumanConfigSerializer::SaveFullSessionToJson(const FMetaHumanGenerationSession& Session, const FString& FilePath)
{
    TSharedPtr<FJsonObject> JsonObject = SessionToJson(Session);
    if (!JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to convert session to JSON"));
        return false;
    }

    return WriteJsonToFile(JsonObject, FilePath);
}

bool UMetaHumanConfigSerializer::LoadFullSessionFromJson(FMetaHumanGenerationSession& OutSession, const FString& FilePath)
{
    TSharedPtr<FJsonObject> JsonObject = ReadJsonFromFile(FilePath);
    if (!JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to read JSON from file: %s"), *FilePath);
        return false;
    }

    return JsonToSession(JsonObject, OutSession);
}

FString UMetaHumanConfigSerializer::SerializeBodyConfigToString(const FMetaHumanBodyParametricConfig& BodyConfig)
{
    TSharedPtr<FJsonObject> JsonObject = BodyConfigToJson(BodyConfig);
    if (!JsonObject.IsValid())
    {
        return FString();
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
    {
        return OutputString;
    }

    return FString();
}

FString UMetaHumanConfigSerializer::SerializeAppearanceConfigToString(const FMetaHumanAppearanceConfig& AppearanceConfig)
{
    TSharedPtr<FJsonObject> JsonObject = AppearanceConfigToJson(AppearanceConfig);
    if (!JsonObject.IsValid())
    {
        return FString();
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
    {
        return OutputString;
    }

    return FString();
}

bool UMetaHumanConfigSerializer::DeserializeBodyConfigFromString(const FString& JsonString, FMetaHumanBodyParametricConfig& OutBodyConfig)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return JsonToBodyConfig(JsonObject, OutBodyConfig);
}

bool UMetaHumanConfigSerializer::DeserializeAppearanceConfigFromString(const FString& JsonString, FMetaHumanAppearanceConfig& OutAppearanceConfig)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        return false;
    }

    return JsonToAppearanceConfig(JsonObject, OutAppearanceConfig);
}

bool UMetaHumanConfigSerializer::SaveGenerationSession(
    const FString& CharacterName,
    const FString& OutputPath,
    const FMetaHumanBodyParametricConfig& BodyConfig,
    const FMetaHumanAppearanceConfig& AppearanceConfig,
    const FString& Status)
{
    FMetaHumanGenerationSession Session = CreateSessionFromCurrentGeneration(
        CharacterName, OutputPath, BodyConfig, AppearanceConfig);
    Session.GenerationStatus = Status;

    FString SessionFilePath = GetSessionFilePath(CharacterName);

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    FString Directory = FPaths::GetPath(SessionFilePath);
    if (!PlatformFile.DirectoryExists(*Directory))
    {
        PlatformFile.CreateDirectoryTree(*Directory);
    }

    return SaveFullSessionToJson(Session, SessionFilePath);
}

bool UMetaHumanConfigSerializer::UpdateSessionStatus(const FString& CharacterName, const FString& NewStatus)
{
    FString SessionFilePath = GetSessionFilePath(CharacterName);

    FMetaHumanGenerationSession Session;
    if (!LoadFullSessionFromJson(Session, SessionFilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load session for character: %s"), *CharacterName);
        return false;
    }

    Session.GenerationStatus = NewStatus;
    return SaveFullSessionToJson(Session, SessionFilePath);
}

FMetaHumanGenerationSession UMetaHumanConfigSerializer::CreateSessionFromCurrentGeneration(
    const FString& CharacterName,
    const FString& OutputPath,
    const FMetaHumanBodyParametricConfig& BodyConfig,
    const FMetaHumanAppearanceConfig& AppearanceConfig)
{
    FMetaHumanGenerationSession Session;
    Session.SessionID = GenerateSessionID(CharacterName);
    Session.CharacterName = CharacterName;
    Session.OutputPath = OutputPath;
    Session.BodyConfig = BodyConfig;
    Session.AppearanceConfig = AppearanceConfig;
    Session.Timestamp = FDateTime::Now();

    return Session;
}

FString UMetaHumanConfigSerializer::GenerateSessionID(const FString& CharacterName)
{
    FDateTime Now = FDateTime::Now();
    return FString::Printf(TEXT("%s_%02d%02d_%02d%02d%02d"),
        *CharacterName,
        Now.GetMonth(), Now.GetDay(),
        Now.GetHour(), Now.GetMinute(), Now.GetSecond());
}

FString UMetaHumanConfigSerializer::GetDefaultConfigDirectory()
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MetaHumanGeneration"), TEXT("Configs"));
}

FString UMetaHumanConfigSerializer::GetSessionFilePath(const FString& CharacterName)
{
    FString ConfigDir = GetDefaultConfigDirectory();
    return FPaths::Combine(ConfigDir, FString::Printf(TEXT("%s_Session.json"), *CharacterName));
}

TSharedPtr<FJsonObject> UMetaHumanConfigSerializer::BodyConfigToJson(const FMetaHumanBodyParametricConfig& BodyConfig)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

    JsonObject->SetStringField(TEXT("BodyType"), *UEnum::GetValueAsString(BodyConfig.BodyType));
    JsonObject->SetNumberField(TEXT("GlobalDeltaScale"), BodyConfig.GlobalDeltaScale);
    JsonObject->SetBoolField(TEXT("bUseParametricBody"), BodyConfig.bUseParametricBody);
    JsonObject->SetStringField(TEXT("QualityLevel"), *UEnum::GetValueAsString(BodyConfig.QualityLevel));

    TSharedPtr<FJsonObject> MeasurementsObj = MakeShareable(new FJsonObject);
    for (const auto& Measurement : BodyConfig.BodyMeasurements)
    {
        MeasurementsObj->SetNumberField(Measurement.Key, Measurement.Value);
    }
    JsonObject->SetObjectField(TEXT("BodyMeasurements"), MeasurementsObj);

    return JsonObject;
}

TSharedPtr<FJsonObject> UMetaHumanConfigSerializer::AppearanceConfigToJson(const FMetaHumanAppearanceConfig& AppearanceConfig)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

    TSharedPtr<FJsonObject> WardrobeObj = WardrobeConfigToJson(AppearanceConfig.WardrobeConfig);
    JsonObject->SetObjectField(TEXT("WardrobeConfig"), WardrobeObj);

    return JsonObject;
}

TSharedPtr<FJsonObject> UMetaHumanConfigSerializer::WardrobeConfigToJson(const FMetaHumanWardrobeConfig& WardrobeConfig)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

    JsonObject->SetStringField(TEXT("HairPath"), WardrobeConfig.HairPath);

    TSharedPtr<FJsonObject> ColorConfigObj = WardrobeColorConfigToJson(WardrobeConfig.ColorConfig);
    JsonObject->SetObjectField(TEXT("ColorConfig"), ColorConfigObj);

    TArray<TSharedPtr<FJsonValue>> ClothingArray;
    for (const FString& ClothingPath : WardrobeConfig.ClothingPaths)
    {
        ClothingArray.Add(MakeShareable(new FJsonValueString(ClothingPath)));
    }
    JsonObject->SetArrayField(TEXT("ClothingPaths"), ClothingArray);

    return JsonObject;
}

TSharedPtr<FJsonObject> UMetaHumanConfigSerializer::WardrobeColorConfigToJson(const FMetaHumanWardrobeColorConfig& ColorConfig)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

    TSharedPtr<FJsonObject> ShirtColorObj = LinearColorToJson(ColorConfig.PrimaryColorShirt);
    JsonObject->SetObjectField(TEXT("PrimaryColorShirt"), ShirtColorObj);

    TSharedPtr<FJsonObject> ShortColorObj = LinearColorToJson(ColorConfig.PrimaryColorShort);
    JsonObject->SetObjectField(TEXT("PrimaryColorShort"), ShortColorObj);

    return JsonObject;
}

TSharedPtr<FJsonObject> UMetaHumanConfigSerializer::SessionToJson(const FMetaHumanGenerationSession& Session)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

    JsonObject->SetStringField(TEXT("SessionID"), Session.SessionID);
    JsonObject->SetStringField(TEXT("Timestamp"), *Session.Timestamp.ToString(TEXT("%Y-%m-%d %H:%M:%S")));
    JsonObject->SetStringField(TEXT("CharacterName"), Session.CharacterName);
    JsonObject->SetStringField(TEXT("OutputPath"), Session.OutputPath);
    JsonObject->SetStringField(TEXT("GenerationStatus"), Session.GenerationStatus);

    TSharedPtr<FJsonObject> BodyConfigObj = BodyConfigToJson(Session.BodyConfig);
    JsonObject->SetObjectField(TEXT("BodyConfig"), BodyConfigObj);

    TSharedPtr<FJsonObject> AppearanceConfigObj = AppearanceConfigToJson(Session.AppearanceConfig);
    JsonObject->SetObjectField(TEXT("AppearanceConfig"), AppearanceConfigObj);

    return JsonObject;
}

bool UMetaHumanConfigSerializer::JsonToBodyConfig(const TSharedPtr<FJsonObject>& JsonObject, FMetaHumanBodyParametricConfig& OutBodyConfig)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }

    FString BodyTypeString;
    if (JsonObject->TryGetStringField(TEXT("BodyType"), BodyTypeString))
    {
        UEnum* BodyTypeEnum = StaticEnum<EMetaHumanBodyType>();
        if (BodyTypeEnum)
        {
            int64 BodyTypeValue = BodyTypeEnum->GetValueByNameString(BodyTypeString);
            if (BodyTypeValue != INDEX_NONE)
            {
                OutBodyConfig.BodyType = static_cast<EMetaHumanBodyType>(BodyTypeValue);
            }
        }
    }

    JsonObject->TryGetNumberField(TEXT("GlobalDeltaScale"), OutBodyConfig.GlobalDeltaScale);
    JsonObject->TryGetBoolField(TEXT("bUseParametricBody"), OutBodyConfig.bUseParametricBody);

    FString QualityLevelString;
    if (JsonObject->TryGetStringField(TEXT("QualityLevel"), QualityLevelString))
    {
        UEnum* QualityLevelEnum = StaticEnum<EMetaHumanQualityLevel>();
        if (QualityLevelEnum)
        {
            int64 QualityLevelValue = QualityLevelEnum->GetValueByNameString(QualityLevelString);
            if (QualityLevelValue != INDEX_NONE)
            {
                OutBodyConfig.QualityLevel = static_cast<EMetaHumanQualityLevel>(QualityLevelValue);
            }
        }
    }

    const TSharedPtr<FJsonObject>* MeasurementsObj;
    if (JsonObject->TryGetObjectField(TEXT("BodyMeasurements"), MeasurementsObj) && MeasurementsObj->IsValid())
    {
        OutBodyConfig.BodyMeasurements.Empty();
        for (const auto& Measurement : (*MeasurementsObj)->Values)
        {
            float Value = 0.0f;
            if (Measurement.Value->TryGetNumber(Value))
            {
                OutBodyConfig.BodyMeasurements.Add(Measurement.Key, Value);
            }
        }
    }

    return true;
}

bool UMetaHumanConfigSerializer::JsonToAppearanceConfig(const TSharedPtr<FJsonObject>& JsonObject, FMetaHumanAppearanceConfig& OutAppearanceConfig)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }

    const TSharedPtr<FJsonObject>* WardrobeObj;
    if (JsonObject->TryGetObjectField(TEXT("WardrobeConfig"), WardrobeObj) && WardrobeObj->IsValid())
    {
        return JsonToWardrobeConfig(*WardrobeObj, OutAppearanceConfig.WardrobeConfig);
    }

    return true;
}

bool UMetaHumanConfigSerializer::JsonToWardrobeConfig(const TSharedPtr<FJsonObject>& JsonObject, FMetaHumanWardrobeConfig& OutWardrobeConfig)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }

    JsonObject->TryGetStringField(TEXT("HairPath"), OutWardrobeConfig.HairPath);

    const TSharedPtr<FJsonObject>* ColorConfigObj;
    if (JsonObject->TryGetObjectField(TEXT("ColorConfig"), ColorConfigObj) && ColorConfigObj->IsValid())
    {
        JsonToWardrobeColorConfig(*ColorConfigObj, OutWardrobeConfig.ColorConfig);
    }

    const TArray<TSharedPtr<FJsonValue>>* ClothingArray;
    if (JsonObject->TryGetArrayField(TEXT("ClothingPaths"), ClothingArray) && ClothingArray)
    {
        OutWardrobeConfig.ClothingPaths.Empty();
        for (const auto& ClothingValue : *ClothingArray)
        {
            FString ClothingPath;
            if (ClothingValue->TryGetString(ClothingPath))
            {
                OutWardrobeConfig.ClothingPaths.Add(ClothingPath);
            }
        }
    }

    return true;
}

bool UMetaHumanConfigSerializer::JsonToWardrobeColorConfig(const TSharedPtr<FJsonObject>& JsonObject, FMetaHumanWardrobeColorConfig& OutColorConfig)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }

    const TSharedPtr<FJsonObject>* ShirtColorObj;
    if (JsonObject->TryGetObjectField(TEXT("PrimaryColorShirt"), ShirtColorObj) && ShirtColorObj->IsValid())
    {
        OutColorConfig.PrimaryColorShirt = JsonToLinearColor(*ShirtColorObj);
    }

    const TSharedPtr<FJsonObject>* ShortColorObj;
    if (JsonObject->TryGetObjectField(TEXT("PrimaryColorShort"), ShortColorObj) && ShortColorObj->IsValid())
    {
        OutColorConfig.PrimaryColorShort = JsonToLinearColor(*ShortColorObj);
    }

    return true;
}

bool UMetaHumanConfigSerializer::JsonToSession(const TSharedPtr<FJsonObject>& JsonObject, FMetaHumanGenerationSession& OutSession)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }

    JsonObject->TryGetStringField(TEXT("SessionID"), OutSession.SessionID);
    JsonObject->TryGetStringField(TEXT("CharacterName"), OutSession.CharacterName);
    JsonObject->TryGetStringField(TEXT("OutputPath"), OutSession.OutputPath);
    JsonObject->TryGetStringField(TEXT("GenerationStatus"), OutSession.GenerationStatus);

    FString TimestampString;
    if (JsonObject->TryGetStringField(TEXT("Timestamp"), TimestampString))
    {
        FDateTime ParsedTime;
        if (FDateTime::Parse(TimestampString, ParsedTime))
        {
            OutSession.Timestamp = ParsedTime;
        }
    }

    const TSharedPtr<FJsonObject>* BodyConfigObj;
    if (JsonObject->TryGetObjectField(TEXT("BodyConfig"), BodyConfigObj) && BodyConfigObj->IsValid())
    {
        JsonToBodyConfig(*BodyConfigObj, OutSession.BodyConfig);
    }

    const TSharedPtr<FJsonObject>* AppearanceConfigObj;
    if (JsonObject->TryGetObjectField(TEXT("AppearanceConfig"), AppearanceConfigObj) && AppearanceConfigObj->IsValid())
    {
        JsonToAppearanceConfig(*AppearanceConfigObj, OutSession.AppearanceConfig);
    }

    return true;
}

bool UMetaHumanConfigSerializer::WriteJsonToFile(const TSharedPtr<FJsonObject>& JsonObject, const FString& FilePath)
{
    if (!JsonObject.IsValid())
    {
        return false;
    }

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);

    if (!FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to serialize JSON to string"));
        return false;
    }

    if (!FFileHelper::SaveStringToFile(OutputString, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save JSON to file: %s"), *FilePath);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("Successfully saved JSON config to: %s"), *FilePath);
    return true;
}

TSharedPtr<FJsonObject> UMetaHumanConfigSerializer::ReadJsonFromFile(const FString& FilePath)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load JSON from file: %s"), *FilePath);
        return nullptr;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON from file: %s"), *FilePath);
        return nullptr;
    }

    return JsonObject;
}

TSharedPtr<FJsonObject> UMetaHumanConfigSerializer::LinearColorToJson(const FLinearColor& Color)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    JsonObject->SetNumberField(TEXT("R"), Color.R);
    JsonObject->SetNumberField(TEXT("G"), Color.G);
    JsonObject->SetNumberField(TEXT("B"), Color.B);
    JsonObject->SetNumberField(TEXT("A"), Color.A);
    return JsonObject;
}

FLinearColor UMetaHumanConfigSerializer::JsonToLinearColor(const TSharedPtr<FJsonObject>& JsonObject)
{
    if (!JsonObject.IsValid())
    {
        return FLinearColor::White;
    }

    float R = 1.0f, G = 1.0f, B = 1.0f, A = 1.0f;
    JsonObject->TryGetNumberField(TEXT("R"), R);
    JsonObject->TryGetNumberField(TEXT("G"), G);
    JsonObject->TryGetNumberField(TEXT("B"), B);
    JsonObject->TryGetNumberField(TEXT("A"), A);

    return FLinearColor(R, G, B, A);
}