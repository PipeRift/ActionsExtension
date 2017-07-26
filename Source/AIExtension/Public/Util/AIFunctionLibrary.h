// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "AISquad.h"
#include "BTD_CompareState.h"

#include "AIFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class AIEXTENSION_API UAIFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

    /** Project a location to the navmesh and return its area type.
     *	@param Location from where we will find the nearest point on nav mesh
     *	@param Extent How far nav area can be
     *	@param Querier if not passed default navigation data will be used
     *	@return true if line from RayStart to RayEnd was obstructed. Also, true when no navigation data present */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Navigation", meta = (WorldContext = "WorldContext"))
    static TSubclassOf<UNavArea> ProjectPointToNavArea(UObject* WorldContext, const FVector& Location, const FVector Extent = FVector(200,200,200), TSubclassOf<UNavigationQueryFilter> FilterClass = NULL, AController* Querier = NULL);



    /** Convert a combat state into a String */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Combat", meta = (DisplayName = "ToString"))
    static FORCEINLINE FString CombatStateToString(ECombatState Value) {
        return EnumToString(FindObject<UEnum>(ANY_PACKAGE, TEXT("ECombatState"), true), Value);
    }

    /** Convert a compare state into a String */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Combat", meta = (DisplayName = "ToString"))
    static FORCEINLINE FString CompareStateModeToString(ECompareStateMode Value) {
        return EnumToString(FindObject<UEnum>(ANY_PACKAGE, TEXT("ECompareStateMode"), true), Value);
    }

    /** Convert an enum into a String */
    template<typename E>
    static FORCEINLINE FString EnumToString(const UEnum* const EnumPtr, const E Value)
    {
        if (!EnumPtr)
            return FString("Invalid");

#if WITH_EDITOR
        return EnumPtr->GetDisplayNameTextByValue((int64)Value).ToString();
#else
        return EnumPtr->GetNameByValue((int64)Value).ToString();
#endif
    }
};
