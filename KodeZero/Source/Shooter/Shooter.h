// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Http.h"
#include "CoreMinimal.h"

class FShooterModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    void OnConditionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful, uint32 EditorPID, FString ProjectRoot);
};