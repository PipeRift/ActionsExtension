// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "UtilityTree/AIGraphNode_Root.h"
#include "GraphEditorSettings.h"


/////////////////////////////////////////////////////
// FAILinkMappingRecord

void FAILinkMappingRecord::PatchLinkIndex(uint8* DestinationPtr, int32 LinkID, int32 SourceLinkID) const
{
	checkSlow(IsValid());

	DestinationPtr = ChildProperty->ContainerPtrToValuePtr<uint8>(DestinationPtr);
	
	if (ChildPropertyIndex != INDEX_NONE)
	{
		UArrayProperty* ArrayProperty = CastChecked<UArrayProperty>(ChildProperty);

		FScriptArrayHelper ArrayHelper(ArrayProperty, DestinationPtr);
		check(ArrayHelper.IsValidIndex(ChildPropertyIndex));

		DestinationPtr = ArrayHelper.GetRawPtr(ChildPropertyIndex);
	}

	// Check to guard against accidental infinite loops
	check((LinkID == INDEX_NONE) || (LinkID != SourceLinkID));

	// Patch the pose link
	FAILinkBase& PoseLink = *((FAILinkBase*)DestinationPtr);
	PoseLink.LinkID = LinkID;
	PoseLink.SourceLinkID = SourceLinkID;
}

/////////////////////////////////////////////////////
// UUTGraphNode_Root

#define LOCTEXT_NAMESPACE "UTGraphNode"

UAIGraphNode_Root::UAIGraphNode_Root(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FLinearColor UAIGraphNode_Root::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->ResultNodeTitleColor;
}

FText UAIGraphNode_Root::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("UTGraphNodeRoot_Title", "Root");
}

FText UAIGraphNode_Root::GetTooltipText() const
{
	return LOCTEXT("UTGraphNodeRoot_Tooltip", "Wire the final ai behavior into this node");
}

bool UAIGraphNode_Root::IsSinkNode() const
{
	return true;
}

void UAIGraphNode_Root::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	// Intentionally empty. This node is auto-generated when a new graph is created.
}

#undef LOCTEXT_NAMESPACE
