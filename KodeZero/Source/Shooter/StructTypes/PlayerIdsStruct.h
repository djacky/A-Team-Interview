#pragma once

#include "CoreMinimal.h"
#include "PlayerIdsStruct.generated.h"

USTRUCT(BlueprintType)
struct FPlayerID
{
	GENERATED_BODY() // needed for structs

	// Epic Games player ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PlayerID = TEXT("");

	// Epic Games player name
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PlayerName = TEXT("");

	// Gamelift Player Session ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString PlayerSessionID = TEXT("");

	// Local GameLift ID
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString LocalGameLiftID = TEXT("");

	// Epic Games JSON Web Token
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString AccessJWT = TEXT("");

	// Kode Zero JSON Web Token
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString GameAccessJWT = TEXT("");
};