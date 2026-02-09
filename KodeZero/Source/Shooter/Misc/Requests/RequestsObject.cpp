// Fill out your copyright notice in the Description page of Project Settings.


#include "RequestsObject.h"
#include "Json.h"
#include "Globals.h"
#include "Kismet/GameplayStatics.h"
#include "Shooter/Misc/Interfaces/GeneralInterface.h"
#include "Misc/DefaultValueHelper.h"



FString URequestsObject::ReadFileAsString(const FString& FilePath, bool bStartMainDirectory)
{
    FString FullFilePath = bStartMainDirectory ? FPaths::ProjectDir() + FilePath : FPaths::ProjectContentDir() + FilePath;
    FString FileContent;
    if (FFileHelper::LoadFileToString(FileContent, *FullFilePath))
    {
		//UE_LOG(LogTemp, Warning, TEXT("FileContent = %s"), *FileContent);
        // File successfully read, and its content is now stored in the "FileContent" FString.
        return FileContent;
    }

    // If the file could not be read or does not exist, return an empty string.
    return FString();
}

void URequestsObject::MakeRequest(const FKeyValue &Params, const FString &EndPoint, ERequestType RequestType, EURLType URL_Type)
{
    FString URL;
    FString BackendToUse;
    switch (URL_Type)
    {
    case EURLType::EUT_Main:
        BackendToUse = IGeneralInterface::Execute_GetBackendURL(UGameplayStatics::GetGameInstance(this));
        URL = BackendToUse + FString(TEXT("/")) + EndPoint;
        break;

    case EURLType::EUT_Wix:
        URL = WIX_REQUEST_URL + FString(TEXT("/")) +  EndPoint;
        break;

    case EURLType::EUT_Test:
        URL = TEST_REQUEST_URL + FString(TEXT("/")) +  EndPoint;
        break;

    case EURLType::EUT_Full:
        URL = EndPoint;
        break;
    }
    UE_LOG(LogTemp, Warning, TEXT("URL: %s"), *URL);
    const FString MainURL = URL;
    CheckKeyValues = Params;
    CheckEndPoint = EndPoint;
    CheckRequestType = RequestType;
    CheckURLType = URL_Type;

	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();

    switch (RequestType)
    {
    case ERequestType::ERT_GET:
        LOG_SHOOTER_NORMAL(FString::Printf(TEXT("GET request to: %s"), *MainURL))
        Request->SetVerb("GET");
        Request->SetURL(SetGetQueries(Params, MainURL));
        break;

    case ERequestType::ERT_POST:
        LOG_SHOOTER_NORMAL(FString::Printf(TEXT("POST request to: %s"), *MainURL))
        FString RequestBody = CreateJsonString(Params);
        LOG_SHOOTER_NORMAL(FString::Printf(TEXT("RequestBody: %s"), *RequestBody))

        if (RequestBody.IsEmpty()) 
        {
            LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Nothing to send in the post request for %s...cancelling post"), *MainURL))
            return;
        }
        Request->SetVerb("POST");
        Request->SetContentAsString(RequestBody);
        Request->SetHeader("Content-Type", "application/json");
        Request->SetURL(MainURL);
        break;
    }

    if (Params.HeadersMap.Num() > 0)
    {
        for (auto Header : Params.HeadersMap)
        {
            Request->AppendToHeader(Header.Key, Header.Value);
        }
    }
    
    Request->OnProcessRequestComplete().BindUObject(this, &URequestsObject::OnResponseReceived);
    Request->ProcessRequest();
}

void URequestsObject::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
    if (bConnectedSuccessfully && Response.IsValid())
    {
        TSharedPtr<FJsonObject> ResponseObj;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
        FJsonSerializer::Deserialize(Reader, ResponseObj);

        FString RequestURL = Response->GetURL();
        LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Successful Request from: %s | Response = %s"), *Response->GetURL(), *Response->GetContentAsString()))
        OnRequestResponseDelegate.Broadcast(Response->GetContentAsString(), Response->GetURL());
        if (OnContentReceivedDelegate.IsBound()) OnContentReceivedDelegate.Broadcast(Response->GetContentAsString(), Response->GetURL(), Response->GetContent());
        RequestAttempts = 0;
    }
    else
    {
        LOG_SHOOTER_NORMAL(FString::Printf(TEXT("Failed request from: %s...trying again"), *Request->GetURL()))
        if (RequestAttempts <= 5)
        {
            MakeRequest(CheckKeyValues, CheckEndPoint, CheckRequestType, CheckURLType);
            RequestAttempts += 1;
        }
        else
        {
            RequestAttempts = 0;
            UE_LOG(LogTemp, Warning, TEXT("Request unsuccessful fron URL: %s"), *Request->GetURL());
        }
    }
}

FString URequestsObject::CreateJsonString(FKeyValue Params)
{
    TSharedRef<FJsonObject> RequestObj = MakeShared<FJsonObject>();

    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);

    if (Params.JsonArray.Num() > 0)
    {
        FJsonSerializer::Serialize(Params.JsonArray, Writer);
    }
    else
    {
        for (const auto& KeyValue : Params.KeyValueMap)
        {
            const FString& Key = KeyValue.Key;
            const FValueType& Value = KeyValue.Value;

            // Access value members based on type
            switch (Value.ValueType)
            {
                case EValueType::EVT_String:
                    RequestObj->SetStringField(Key, Value.StringValue);
                    break;
                case EValueType::EVT_Integer:
                    RequestObj->SetNumberField(Key, Value.IntegerValue);
                    break;
                case EValueType::EVT_Float:
                    RequestObj->SetNumberField(Key, Value.FloatValue);
                    break;
                case EValueType::EVT_Bool:
                    RequestObj->SetBoolField(Key, Value.BoolValue);
                    break;
                case EValueType::EVT_Object:
                    if (Value.JsonObjectValue.IsValid())
                    {
                        RequestObj->SetObjectField(Key, Value.JsonObjectValue.ToSharedRef());
                    }
                    break;
                case EValueType::EVT_StringArray:
                    {
                        TArray<TSharedPtr<FJsonValue>> JsonArray;
                        for (const FString& Item : Value.StringArrayValue)
                        {
                            JsonArray.Add(MakeShared<FJsonValueString>(Item));
                        }
                        RequestObj->SetArrayField(Key, JsonArray);
                        break;
                    }
                case EValueType::EVT_Array:  // New case
                    RequestObj->SetArrayField(Key, Value.ArrayValue);
                    break;
                default:
                    break;
            }
        }
        FJsonSerializer::Serialize(RequestObj, Writer);
    }

    return RequestBody;
}

FString URequestsObject::SetGetQueries(const FKeyValue &Params, FString MainURL)
{
    if (Params.KeyValueMap.Num() == 0) return MainURL;
    MainURL += TEXT("?");
    int ParamsIter = 0;
    for (const auto& KeyValue : Params.KeyValueMap)
    {
        const FString& Key = KeyValue.Key;
        const FValueType& Value = KeyValue.Value;

        switch (Value.ValueType)
        {
            case EValueType::EVT_String:
                MainURL += Key + TEXT("=") + Value.StringValue;
                ++ParamsIter;
                if (Params.KeyValueMap.Num() > 1 && ParamsIter < Params.KeyValueMap.Num())
                {
                    MainURL += TEXT("&");
                }
                break;
        }
    }
    return MainURL;
}

