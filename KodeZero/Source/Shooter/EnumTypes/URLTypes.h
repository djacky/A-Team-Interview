#pragma once

#define MAIN_REQUEST_URL TEXT("https://apis.disruptive-labs.io")
#define WIX_REQUEST_URL TEXT("https://www.disruptive-labs.io/_functions")
//#define TEST_REQUEST_URL TEXT("http://localhost:80")
#define TEST_REQUEST_URL TEXT("https://settled-stallion-healthy.ngrok-free.app")

UENUM(BlueprintType)
enum class EURLType : uint8
{
	EUT_Main UMETA(DisplayName = "MainURL"),
	EUT_Wix UMETA(DisplayName = "WixURL"),
	EUT_Test UMETA(DisplayName = "TestURL"),
	EUT_Full UMETA(DisplayName = "FullURL"),

	EUT_MAX UMETA(DisplayName = "DefaultMAX")
};