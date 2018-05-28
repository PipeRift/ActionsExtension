// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "UtilityTreeGraphFactory.h"

#include "Nodes/AINodeBase.h"
#include "UtilityTree/AIGraphNode_Base.h"
#include "UtilityTree/AIGraphNode_Root.h"

#include "UtilityTree/UtilityTreeGraphSchema.h"

#include "AIPins/SGraphPinAI.h"

#include "KismetPins/SGraphPinExec.h"

TSharedPtr<class SGraphNode> FAIGraphNodeFactory::CreateNode(class UEdGraphNode* InNode) const
{
	if (UAIGraphNode_Base* BaseAINode = Cast<UAIGraphNode_Base>(InNode))
	{
		/*if (UAIGraphNode_Root* RootAINode = Cast<UAIGraphNode_Root>(InNode))
		{
			return SNew(SGraphNodeAIResult, RootAINode);
		}
		else
		{
			return SNew(SAIGraphNode, BaseAnimNode);
		}*/
	}

	return nullptr;
}

TSharedPtr<class SGraphPin> FAIGraphPinFactory::CreatePin(class UEdGraphPin* InPin) const
{
	if (InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
	{
		if (InPin->PinType.PinSubCategoryObject == FAILink::StaticStruct())
		{
			return SNew(SGraphPinAI, InPin);
		}
	}

	return nullptr;
}

class FConnectionDrawingPolicy* FAIGraphPinConnectionFactory::CreateConnectionPolicy(const class UEdGraphSchema* Schema, int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const class FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
	if (Schema->IsA(UUtilityTreeGraphSchema::StaticClass()))
	{
		//return new FUTGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj);
	}

	return nullptr;
}