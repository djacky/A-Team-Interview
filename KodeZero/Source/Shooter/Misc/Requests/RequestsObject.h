// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Http.h"
#include "Shooter/EnumTypes/URLTypes.h"
#include "RequestsObject.generated.h"


UENUM(BlueprintType)
enum class ERequestType : uint8
{
	ERT_GET UMETA(DisplayName = "GET"),
	ERT_POST UMETA(DisplayName = "POST"),

	ERT_MAX UMETA(DisplayName = "DefaultMAX")
};

UENUM(BlueprintType)
enum class EValueType : uint8
{
	EVT_String UMETA(DisplayName = "String"),
	EVT_Float UMETA(DisplayName = "Float"),
	EVT_Integer UMETA(DisplayName = "Integer"),
    EVT_Bool UMETA(DisplayName = "Bool"),
	EVT_Object UMETA(DisplayName = "Object"),
	EVT_StringArray UMETA(DisplayName = "String Array"),
	EVT_Array UMETA(DisplayName = "Array"),

	EVT_MAX UMETA(DisplayName = "DefaultMAX")
};

USTRUCT(BlueprintType)
struct FValueType
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EValueType ValueType = EValueType::EVT_String;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString StringValue = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 IntegerValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FloatValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool BoolValue = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> StringArrayValue;

	TSharedPtr<FJsonObject> JsonObjectValue;

    TArray<TSharedPtr<FJsonValue>> ArrayValue;
};

USTRUCT(BlueprintType)
struct FKeyValue
{
	GENERATED_BODY() // needed for structs

    void SetStringField(const FString &InKey, const FString &InString)
    {
        FValueType ValueInfo;
        ValueInfo.StringValue = InString;
        ValueInfo.ValueType = EValueType::EVT_String;
        KeyValueMap.Add(InKey, ValueInfo);
    }

    void SetIntegerField(const FString &InKey, int32 InInteger)
    {
        FValueType ValueInfo;
        ValueInfo.IntegerValue = InInteger;
        ValueInfo.ValueType = EValueType::EVT_Integer;
        KeyValueMap.Add(InKey, ValueInfo);
    }

    void SetFloatField(const FString &InKey, float InFloat)
    {
        FValueType ValueInfo;
        ValueInfo.FloatValue = InFloat;
        ValueInfo.ValueType = EValueType::EVT_Float;
        KeyValueMap.Add(InKey, ValueInfo);
    }

    void SetBoolField(const FString &InKey, bool InBool)
    {
        FValueType ValueInfo;
        ValueInfo.BoolValue = InBool;
        ValueInfo.ValueType = EValueType::EVT_Bool;
        KeyValueMap.Add(InKey, ValueInfo);
    }

	void SetStringArrayField(const FString& InKey, const TArray<FString>& InArray)
	{
		FValueType ValueInfo;
		ValueInfo.StringArrayValue = InArray;
		ValueInfo.ValueType = EValueType::EVT_StringArray;
		KeyValueMap.Add(InKey, ValueInfo);
	}

	void SetObjectField(const FString& InKey, TSharedPtr<FJsonObject> InObject)
	{
		FValueType ValueInfo;
		ValueInfo.ValueType = EValueType::EVT_Object;
		ValueInfo.JsonObjectValue = InObject;
		KeyValueMap.Add(InKey, ValueInfo);
	}

    void SetArrayField(const FString& InKey, const TArray<TSharedPtr<FJsonValue>>& InArray)
    {
        FValueType ValueInfo;
        ValueInfo.ValueType = EValueType::EVT_Array;
        ValueInfo.ArrayValue = InArray;
        KeyValueMap.Add(InKey, ValueInfo);
    }

	void AddHeader(const FString &InKey, const FString &InValue)
	{
		HeadersMap.Add(InKey, InValue);
	}

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, FValueType> KeyValueMap;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FString, FString> HeadersMap;

	TArray<TSharedPtr<FJsonValue>> JsonArray;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGetRequestReceived, FString, GetResponseStr, FString, GetURL);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPostRequestReceived, FString, PostResponseStr, FString, GetURL);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnContentReceived, FString, ContentResponseStr, FString, ContentURL, TArray<uint8>, Content);

UCLASS()
class SHOOTER_API URequestsObject : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void MakeRequest(const FKeyValue &Params, const FString &EndPoint, ERequestType RequestType, EURLType URL_Type);
	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

	UFUNCTION(BlueprintCallable)
	FString ReadFileAsString(const FString& FilePath, bool bStartMainDirectory = false);

	UPROPERTY(BlueprintAssignable)
	FOnPostRequestReceived OnRequestResponseDelegate;

	UPROPERTY(BlueprintAssignable)
	FOnContentReceived OnContentReceivedDelegate;

	UFUNCTION(BlueprintCallable)
	FString CreateJsonString(FKeyValue Params);

	FString SetGetQueries(const FKeyValue &Params, FString MainURL);

	UPROPERTY()
	FString CheckEndPoint;
	UPROPERTY()
	FKeyValue CheckKeyValues;
	UPROPERTY()
	ERequestType CheckRequestType;
	UPROPERTY()
	EURLType CheckURLType;

	int8 RequestAttempts = 0;

    //TFunction<void()> RetryFunction;
};
