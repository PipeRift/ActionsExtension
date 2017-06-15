// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "AIFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class AIEXTENSION_API UAIFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	

    /** Project a location to the navmesh and return its area type.
     *	@param Location from where we will find the nearest point on nav mesh
     *	@param Extent How far nav area can be
     *	@param Querier if not passed default navigation data will be used
     *	@return true if line from RayStart to RayEnd was obstructed. Also, true when no navigation data present */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "AI|Navigation", meta = (WorldContext = "WorldContext"))
    static TSubclassOf<UNavArea> ProjectPointToNavArea(UObject* WorldContext, const FVector& Location, const FVector Extent = FVector(200,200,200), TSubclassOf<UNavigationQueryFilter> FilterClass = NULL, AController* Querier = NULL);
};
