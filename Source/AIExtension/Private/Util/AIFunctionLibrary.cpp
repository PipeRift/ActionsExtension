// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "AIFunctionLibrary.h"
#include "AI/Navigation/RecastNavMesh.h"




TSubclassOf<UNavArea> UAIFunctionLibrary::ProjectPointToNavArea(UObject* WorldContext, const FVector& Location, const FVector Extent, TSubclassOf<UNavigationQueryFilter> FilterClass /*= NULL*/, AController* Querier /*= NULL*/)
{
    UWorld* World = NULL;

    if (WorldContext != NULL)
    {
        World = GEngine->GetWorldFromContextObject(WorldContext);
    }
    if (World == NULL && Querier != NULL)
    {
        World = GEngine->GetWorldFromContextObject(Querier);
    }

    if (World != NULL && World->GetNavigationSystem() != NULL)
    {
        const UNavigationSystem* NavSys = World->GetNavigationSystem();

        // figure out which navigation data to use
        const ANavigationData* NavData = NULL;
        INavAgentInterface* MyNavAgent = Cast<INavAgentInterface>(Querier);
        if (MyNavAgent)
        {
            const FNavAgentProperties& AgentProps = MyNavAgent->GetNavAgentPropertiesRef();
            NavData = NavSys->GetNavDataForProps(AgentProps);
        }
        if (NavData == NULL)
        {
            NavData = NavSys->GetMainNavData();
        }

        const ARecastNavMesh* NavMesh = Cast<ARecastNavMesh>(NavData);
        if (!NavMesh)
        {
            UE_LOG(LogAIExtension, Error, TEXT("UAIFunctionLibrary: Cast to RecastNavMesh failed"))
            return NULL;
        }

        const NavNodeRef NodeRef = NavMesh->FindNearestPoly(Location, Extent, UNavigationQueryFilter::GetQueryFilter(*NavData, Querier, FilterClass));
        
        //Get our Area
        return const_cast<UClass*>(NavMesh->GetAreaClass(NavMesh->GetPolyAreaID(NodeRef)));
    }

    return NULL;
}
