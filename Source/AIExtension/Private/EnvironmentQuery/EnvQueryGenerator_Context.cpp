// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "EnvQueryGenerator_Context.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"

#define LOCTEXT_NAMESPACE "EnvQueryGenerator"

UEnvQueryGenerator_Context::UEnvQueryGenerator_Context(const FObjectInitializer& ObjectInitializer) :
    Super(ObjectInitializer)
{
    ItemType = UEnvQueryItemType_Actor::StaticClass();

    Source = nullptr;
}

void UEnvQueryGenerator_Context::GenerateItems(FEnvQueryInstance& QueryInstance) const
{
    if (Source == nullptr)
    {
        return;
    }

    UObject* QueryOwner = QueryInstance.Owner.Get();
    if (QueryOwner == nullptr)
    {
        return;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(QueryOwner);
    if (World == nullptr)
    {
        return;
    }

    TArray<AActor*> ContextActors;
    QueryInstance.PrepareContext(Source, ContextActors);

    QueryInstance.AddItemData<UEnvQueryItemType_Actor>(ContextActors);
}

FText UEnvQueryGenerator_Context::GetDescriptionTitle() const
{
    FFormatNamedArguments Args;
    Args.Add(TEXT("Title"), Super::GetDescriptionTitle());

    if (Source != nullptr)
    {
        Args.Add(TEXT("SourceContext"), UEnvQueryTypes::DescribeContext(Source));

        return FText::Format(LOCTEXT("ContextTitle", "{Title}: generate from {SourceContext}"), Args);
    }
    else
    {
        return FText::Format(LOCTEXT("ContextTitle", "{Title}: Not generating"), Args);
    }
};

FText UEnvQueryGenerator_Context::GetDescriptionDetails() const
{
    return LOCTEXT("ContextDescription", "");
}

#undef LOCTEXT_NAMESPACE

