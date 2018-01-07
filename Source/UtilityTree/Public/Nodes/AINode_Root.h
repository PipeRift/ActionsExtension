// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Nodes/AINodeBase.h"
#include "AINode_Root.generated.h"

// Root node of an animation tree (sink)
USTRUCT(BlueprintInternalUseOnly)
struct UTILITYTREE_API FAINode_Root : public FAINode_Base
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FAILink Result;

public:	
	FAINode_Root();

	// FAINode_Base interface
	virtual void Initialize() override;
	virtual void Update() override;
	virtual void GatherDebugData(FAINodeDebugData& DebugData) override;
	// End of FAINode_Base interface
};
