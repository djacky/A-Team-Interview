// Fill out your copyright notice in the Description page of Project Settings.


#include "Misc/GeneralObject.h"

bool UGeneralObject::ReadJsonFromFile(const FString& FilePath, TSharedPtr<FJsonObject>& OutJsonObject)
{
    // Read the file into a string
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load JSON file: %s"), *FilePath);
        return false;
    }

    // Create a JSON reader
    TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);

    // Deserialize the JSON
    if (!FJsonSerializer::Deserialize(JsonReader, OutJsonObject) || !OutJsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to deserialize JSON from file: %s"), *FilePath);
        return false;
    }

    return true;
}