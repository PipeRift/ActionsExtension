// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "Faction.h"
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


    /***************************************/
    /* Factions                            */
    /***************************************/

    UFUNCTION(BlueprintPure, Category = "Factions", meta = (CompactNodeTitle = "=="))
    static FORCEINLINE bool Equals(const FFaction& A, const FFaction& B) {
        return A == B;
    }

    UFUNCTION(BlueprintPure, Category = "Factions", meta = (CompactNodeTitle = "!="))
    static FORCEINLINE bool NotEqual(const FFaction& A, const FFaction& B) {
        return A != B;
    }

	UFUNCTION(BlueprintPure, Category = "Factions")
	static FORCEINLINE FFaction GetFaction(AActor* Target)
	{
		return IFactionAgentInterface::Execute_GetFaction(Target);
	}

    UFUNCTION(BlueprintPure, Category = "Factions", meta = (DisplayName = "Get Attitude Towards", WorldContext = "A"))
	static FORCEINLINE TEnumAsByte<ETeamAttitude::Type> GetAttitudeBetween(AActor* A, AActor* B)
	{
		return GetAttitudeTowards(GetFaction(A), GetFaction(B));
	}

    UFUNCTION(BlueprintPure, Category = "Factions")
    static FORCEINLINE TEnumAsByte<ETeamAttitude::Type> GetAttitudeTowards(const FFaction& A, const FFaction& B)
	{
        return A.GetAttitudeTowards(B);
    }


    /***************************************/
    /* Enum To String                      */
    /***************************************/

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
