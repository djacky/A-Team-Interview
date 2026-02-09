// Fill out your copyright notice in the Description page of Project Settings.

#include "AsyncHTTPRequests.h"
#include "HttpModule.h"
#include "JsonObjectWrapper.h"
#include "Dom/JsonObject.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetStringLibrary.h"
#include "Shooter/SaveGame/ShooterSaveGame.h"
#include "Interfaces/IHttpResponse.h"
#include "Shooter/Misc/Interfaces/GeneralInterface.h"
#include "StructTypes/PlayerIdsStruct.h"


//#define LOCTEXT_NAMESPACE "KodeZero"

UAsyncHTTPRequests* UAsyncHTTPRequests::CreateHttpRequestProxyObjectFromString(const UObject* WorldContextObject,
	const FString& InEndpoint,
	const FString& InVerb,
	FHttpHeader InHeader,
	const FString& InBody,
    EURLType Backend) 
{
	UAsyncHTTPRequests* const Proxy = NewObject<UAsyncHTTPRequests>();
	Proxy->SetFlags(RF_StrongRefOnFrame);
	Proxy->CachedHeader = InHeader;
	Proxy->WCO = WorldContextObject;
	Proxy->Request = MakeRequest(WorldContextObject, InEndpoint, InVerb, Backend);
	Proxy->Request->SetContentAsString(InBody);
	return Proxy;
}

UAsyncHTTPRequests* UAsyncHTTPRequests::CreateHttpRequestProxyObjectFromBuffer(const UObject* WorldContextObject,
	const FString& InEndpoint,
	const FString& InVerb,
	FHttpHeader InHeader,
	const TArray<uint8>& InBody,
    EURLType Backend) 
{
	UAsyncHTTPRequests* const Proxy = NewObject<UAsyncHTTPRequests>();
	Proxy->SetFlags(RF_StrongRefOnFrame);
	Proxy->CachedHeader = InHeader;
	Proxy->WCO = WorldContextObject;
	Proxy->Request = MakeRequest(WorldContextObject, InEndpoint, InVerb, Backend);
	Proxy->Request->SetContent(InBody);
	return Proxy;
}

FHttpRequestPtr UAsyncHTTPRequests::MakeRequest(const UObject* WorldContextObject,
												const FString& InEndpoint,
                                                 const FString& InVerb,
                                                 EURLType URLType) 
{
    FString MainURL;
	const bool IsTestMode = IGeneralInterface::Execute_GetIsTestMode(
		UGameplayStatics::GetGameInstance(WorldContextObject)
	);
    switch (URLType)
    {
    case EURLType::EUT_Main:
        MainURL = IsTestMode ? TEST_REQUEST_URL : MAIN_REQUEST_URL;
        break;
    case EURLType::EUT_Wix:
        MainURL = WIX_REQUEST_URL;
        break;
    case EURLType::EUT_Test:
        MainURL = TEST_REQUEST_URL;
        break;
    }
    
	//FString Url = FText::Format(
	//	LOCTEXT("KodeZero", "{0}/{1}"), FText::FromString(MainURL),
	//	FText::FromString(InEndpoint)).ToString();
	
    FString Url = FString::Printf(TEXT("%s/%s"), *MainURL, *InEndpoint);
    UE_LOG(LogTemp, Warning, TEXT("Url: %s"), *Url);
	const TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(InVerb);
	return Request;
}

void UAsyncHTTPRequests::ProcessComplete(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse,
                                          bool bInSuccessful) const 
{
	FJsonObjectWrapper response = {};
	if (!bInSuccessful) 
    {
		if (InResponse.IsValid()) response.JsonString = InResponse->GetContentAsString();
		Failed.Broadcast(response);
		return;
	}
	//UE_LOG(LogTemp, Warning, TEXT("Response String: %s"), *InResponse->GetContentAsString());
	response.JsonObjectFromString(InResponse->GetContentAsString());
	FJsonObjectWrapper result = {};
	result.JsonObject = response.JsonObject->GetObjectField(TEXT("result"));
	
	if (response.JsonObject->GetBoolField(TEXT("success"))) 
    {
		Success.Broadcast(result);
		return;
	}
    else
    {
        Error.Broadcast(result);
        return;
    }
    /*
	FJsonObjectWrapper details = {};
	details.JsonObject = result.JsonObject->GetObjectField(TEXT("details"));
	auto code = details.JsonObject->GetStringField(TEXT("CODE"));
	if (UKismetStringLibrary::EqualEqual_StrStr(code, "CLIENT_UNAUTHORIZED")) {
		Unauthorized.Broadcast(details);
		return;
	}
    */
	
}

void UAsyncHTTPRequests::Activate() 
{
	Super::Activate();
    
	const FPlayerID Ids = IGeneralInterface::Execute_GetPlayerIds(UGameplayStatics::GetGameInstance(WCO));
    FString token = Ids.AccessJWT;
	if (token.IsEmpty()) {
		Unauthorized.Broadcast({});
		return;
	}
	CachedHeader.AddHeader(TPair<FString,FString>("token", token));
	CachedHeader.AssignHeadersToRequest(Request.ToSharedRef());
	Request->OnProcessRequestComplete().BindUObject(this, &ThisClass::ProcessComplete);
	Request->ProcessRequest();
}

//#undef LOCTEXT_NAMESPACE
