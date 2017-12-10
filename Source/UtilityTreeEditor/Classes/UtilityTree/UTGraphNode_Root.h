// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UTGraphNode_Base.h"
//#include "UtilityTree/UTNode_Root.h"
#include "UTGraphNode_Root.generated.h"

class FBlueprintActionDatabaseRegistrar;

UCLASS(MinimalAPI)
class UUTGraphNode_Root : public UUTGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	//UPROPERTY(EditAnywhere, Category=Settings)
	//FUTNode_Root Node;

	//~ Begin UEdGraphNode Interface.
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool CanUserDeleteNode() const override { return false; }
	virtual bool CanDuplicateNode() const override { return false; }
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	//~ End UEdGraphNode Interface.

	//~ Begin UUTGraphNode_Base Interface
	virtual bool IsSinkNode() const override;

	//~ End UUTGraphNode_Base Interface
};
