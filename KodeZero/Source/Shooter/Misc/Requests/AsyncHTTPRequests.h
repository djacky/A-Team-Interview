// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HttpFwd.h"
#include "HttpHeader.h"
#include "JsonObjectWrapper.h"
#include "Shooter/EnumTypes/URLTypes.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncHTTPRequests.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAPIHitComplete, FJsonObjectWrapper, Response);

//#define KodeZeroAPI_PORT "3000"

UCLASS()
class SHOOTER_API UAsyncHTTPRequests : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintAssignable)
	FOnAPIHitComplete Success;
	UPROPERTY(BlueprintAssignable)
	FOnAPIHitComplete Unauthorized;
	UPROPERTY(BlueprintAssignable)
	FOnAPIHitComplete Error;
	UPROPERTY(BlueprintAssignable)
	FOnAPIHitComplete Failed;
	
	FHttpRequestPtr Request;
	UPROPERTY()
	const UObject* WCO;

	void ProcessComplete(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bInSuccessful) const;
	UPROPERTY()
	FHttpHeader CachedHeader;

public:
	static FHttpRequestPtr MakeRequest(const UObject* WorldContextObject, const FString& InEndpoint, const FString& InVerb, EURLType URLType);
	UFUNCTION(BlueprintCallable,
		Meta = (BlueprintInternalUseOnly = "true", DisplayName = "Make Request Async (String)",WorldContext="WorldContextObject"), Category = "Http")
	static UAsyncHTTPRequests* CreateHttpRequestProxyObjectFromString( const UObject* WorldContextObject,
		const FString& InEndpoint,
		const FString& InVerb,
		FHttpHeader InHeader,
		const FString& InBody,
    	EURLType Backend);

	UFUNCTION(BlueprintCallable,
		Meta = (BlueprintInternalUseOnly = "true", DisplayName = "Make Request Async (Buffer)",WorldContext="WorldContextObject"), Category = "Http")
	static UAsyncHTTPRequests* CreateHttpRequestProxyObjectFromBuffer(const UObject* WorldContextObject,
		const FString& InEndpoint,
		const FString& InVerb,
		FHttpHeader InHeader,
		const TArray<uint8>& InBody,
    	EURLType Backend);
protected:
	virtual void Activate() override;
	
};
