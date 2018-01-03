// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UnrealType.h"
#include "UtilityTreeBlueprint.h"
#include "UTBlueprintGeneratedClass.h"
#include "Nodes/AINodeBase.h"
#include "Editor.h"
#include "K2Node.h"
#include "AIGraphNode_Base.generated.h"

class FUTBlueprintCompiler;
class FUTGraphNodeDetails;
class FBlueprintActionDatabaseRegistrar;
class FCanvas;
class FCompilerResultsLog;
class FPrimitiveDrawInterface;
class IDetailLayoutBuilder;
class UAIGraphNode_Base;
class UEdGraphSchema;
class USkeletalMeshComponent;

struct FAILinkMappingRecord
{
public:
	static FAILinkMappingRecord MakeFromArrayEntry(UAIGraphNode_Base* LinkingNode, UAIGraphNode_Base* LinkedNode, UArrayProperty* ArrayProperty, int32 ArrayIndex)
	{
		checkSlow(CastChecked<UStructProperty>(ArrayProperty->Inner)->Struct->IsChildOf(FAILinkBase::StaticStruct()));

		FAILinkMappingRecord Result;
		Result.LinkingNode = LinkingNode;
		Result.LinkedNode = LinkedNode;
		Result.ChildProperty = ArrayProperty;
		Result.ChildPropertyIndex = ArrayIndex;

		return Result;
	}

	static FAILinkMappingRecord MakeFromMember(UAIGraphNode_Base* LinkingNode, UAIGraphNode_Base* LinkedNode, UStructProperty* MemberProperty)
	{
		checkSlow(MemberProperty->Struct->IsChildOf(FAILinkBase::StaticStruct()));

		FAILinkMappingRecord Result;
		Result.LinkingNode = LinkingNode;
		Result.LinkedNode = LinkedNode;
		Result.ChildProperty = MemberProperty;
		Result.ChildPropertyIndex = INDEX_NONE;

		return Result;
	}

	static FAILinkMappingRecord MakeInvalid()
	{
		FAILinkMappingRecord Result;
		return Result;
	}

	bool IsValid() const
	{
		return LinkedNode != nullptr;
	}

	UAIGraphNode_Base* GetLinkedNode() const
	{
		return LinkedNode;
	}

	UAIGraphNode_Base* GetLinkingNode() const
	{
		return LinkingNode;
	}

	UTILITYTREEEDITOR_API void PatchLinkIndex(uint8* DestinationPtr, int32 LinkID, int32 SourceLinkID) const;
protected:
	FAILinkMappingRecord()
		: LinkedNode(nullptr)
		, LinkingNode(nullptr)
		, ChildProperty(nullptr)
		, ChildPropertyIndex(INDEX_NONE)
	{
	}

protected:
	// Linked node for this pose link, can be nullptr
	UAIGraphNode_Base* LinkedNode;

	// Linking node for this pose link, can be nullptr
	UAIGraphNode_Base* LinkingNode;

	// Will either be an array property containing FAILinkBase derived structs, indexed by ChildPropertyIndex, or a FAILinkBase derived struct property 
	UProperty* ChildProperty;

	// Index when ChildProperty is an array
	int32 ChildPropertyIndex;
};

UENUM()
enum class EUTBlueprintUsage : uint8
{
	NoProperties,
	DoesNotUseBlueprint,
	UsesBlueprint
};

/** Enum that indicates level of support of this node for a particular asset class */
enum class EUTAssetHandlerType : uint8
{
	PrimaryHandler,
	Supported,
	NotSupported
};

/**
  * This is the base class for any utility tree graph nodes.
  *
  * Any concrete implementations will be paired with a runtime graph node derived from FAINode_Base
  */
UCLASS(Abstract)
class UTILITYTREEEDITOR_API UAIGraphNode_Base : public UK2Node
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=PinOptions, EditFixedSize)
	TArray<FOptionalPinFromProperty> ShowPinForProperties;

	UPROPERTY(Transient)
	EUTBlueprintUsage BlueprintUsage;

	// UObject interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PreEditChange(UProperty* PropertyAboutToChange) override;
	// End of UObject interface

	// UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FString GetDocumentationLink() const override;
	virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;
	virtual bool ShowPaletteIconOnNode() const override{ return false; }
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	// End of UEdGraphNode interface

	// UK2Node interface
	virtual bool NodeCausesStructuralBlueprintChange() const override { return true; }
	virtual bool ShouldShowNodeProperties() const override { return true; }
	virtual bool CanPlaceBreakpoints() const override { return false; }
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const override;
	virtual void GetNodeAttributes(TArray<TKeyValuePair<FString, FString>>& OutNodeAttributes) const override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;

	// By default return any utility tree assets we have
	virtual UObject* GetJumpTargetForDoubleClick() const override { return nullptr;/* GetUtilityTreeAsset(); */}
	// End of UK2Node interface

	// UUTGraphNode_Base interface

	// Gets the menu category this node belongs in
	virtual FString GetNodeCategory() const;

	// Is this node a sink that has no pose outputs?
	virtual bool IsSinkNode() const { return false; }

	// Create any output pins necessary for this node
	virtual void CreateOutputPins();

	// customize pin data based on the input
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const {}

	// Gives each visual node a chance to do final validation before it's node is harvested for use at runtime
	virtual void ValidateAINodeDuringCompilation(FCompilerResultsLog& MessageLog) {}

	// Gives each visual node a chance to validate that they are still valid in the context of the compiled class, giving a last shot at error or warning generation after primary compilation is finished
	virtual void ValidateAINodePostCompile(FCompilerResultsLog& MessageLog, UUTBlueprintGeneratedClass* CompiledClass, int32 CompiledNodeIndex) {}

	// Gives each visual node a chance to update the node template before it is inserted in the compiled class
	virtual void BakeDataDuringCompilation(FCompilerResultsLog& MessageLog) {}

	// preload asset required for this node in this function
	virtual void PreloadRequiredAssets() override {}

	// Give the node a chance to change the display name of a pin
	virtual void PostProcessPinName(const UEdGraphPin* Pin, FString& DisplayName) const;

	/** Get the utility tree blueprint to which this node belongs */
	UUtilityTreeBlueprint* GetUTBlueprint() const { return CastChecked<UUtilityTreeBlueprint>(GetBlueprint()); }

	/**
	 * Selection notification callback.
	 * If a node needs to handle viewport input etc. then it should push an editor mode here.
	 * @param	bInIsSelected	Whether we selected or deselected the node
	 * @param	InModeTools		The mode tools. Use this to push the editor mode if required.
	 * @param	InRuntimeNode	The runtime node to go with this skeletal control. This may be NULL in some cases when bInIsSelected is false.
	 */
	virtual void OnNodeSelected(bool bInIsSelected, class FEditorModeTools& InModeTools, struct FAINode_Base* InRuntimeNode);

	// Draw function for supporting visualization
	virtual void Draw(FPrimitiveDrawInterface* PDI) const {}
	// Canvas draw function to draw to viewport
	virtual void DrawCanvas(FViewport& InViewport, FSceneView& View, FCanvas& Canvas) const {}
	// Function to collect strings from nodes to display in the viewport.
	// Use this rather than DrawCanvas when adding general text to the viewport.
	virtual void GetOnScreenDebugInfo(TArray<FText>& DebugInfo, FAINode_Base* RuntimeAINode, USkeletalMeshComponent* PreviewSkelMeshComp) const {}

	/** Called after editing a default value to update internal node from pin defaults. This is needed for forwarding code to propagate values to preview. */
	virtual void CopyPinDefaultsToNodeData(UEdGraphPin* InPin) {}

	/** Called to propagate data from the internal node to the preview in Persona. */
	virtual void CopyNodeDataToPreviewNode(FAINode_Base* InPreviewNode) {}

	// BEGIN Interface to support transition getter
	// if you return true for DoesSupportExposeTimeForTransitionGetter
	// you should implement all below functions
	virtual bool DoesSupportTimeForTransitionGetter() const { return false; }
	//virtual UUtilityTreeAsset* GetUtilityTreeAsset() const { return nullptr; }
	virtual const TCHAR* GetTimePropertyName() const { return nullptr; }
	virtual UScriptStruct* GetTimePropertyStruct() const { return nullptr; }
	// END Interface to support transition getter

	// can customize details tab 
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder){ }

	/** Try to find the preview node instance for this ai graph node */
	FAINode_Base* FindDebugAINode() const;

	template<typename NodeType>
	NodeType* GetActiveInstanceNode(UObject* UtilityTreeObject) const
	{
		if(!UtilityTreeObject)
		{
			return nullptr;
		}

		if(UUTBlueprintGeneratedClass* UTClass = Cast<UUTBlueprintGeneratedClass>(UtilityTreeObject->GetClass()))
		{
			return UTClass->GetPropertyInstance<NodeType>(UtilityTreeObject, NodeGuid);
		}

		return nullptr;
	}

	/** 
	 *	Returns whether this node supports the supplied asset class
	 *	@param	bPrimaryAssetHandler	Is this the 'primary' handler for this asset (the node that should be created when asset is dropped)
	 */
	virtual EUTAssetHandlerType SupportsAssetClass(const UClass* AssetClass) const;

	// Event that observers can bind to so that they are notified about changes
	// made to this node through the property system
	DECLARE_EVENT_OneParam(UAIGraphNode_Base, FOnNodePropertyChangedEvent, FPropertyChangedEvent&);
	FOnNodePropertyChangedEvent& OnNodePropertyChanged() { return PropertyChangeEvent;	}

	/**
	 * Helper function to check whether a pin is valid and linked to something else in the graph
	 * @param	InPinName		The name of the pin @see UEdGraphNode::FindPin
	 * @param	InPinDirection	The direction of the pin we are looking for. If this is EGPD_MAX, all directions are considered
	 * @return true if the pin is present and connected
	 */
	bool IsPinExposedAndLinked(const FString& InPinName, const EEdGraphPinDirection Direction = EGPD_MAX) const;

protected:
	friend FUTBlueprintCompiler;
	friend FUTGraphNodeDetails;

	// Gets the ai FNode type represented by this ed graph node
	UScriptStruct* GetFNodeType() const;

	// Gets the ai FNode property represented by this ed graph node
	UStructProperty* GetFNodeProperty() const;

	// This will be called when a pose link is found, and can be called with PoseProperty being either of:
	//  - an array property (ArrayIndex >= 0)
	//  - a single pose property (ArrayIndex == INDEX_NONE)
	virtual void CreatePinsForPoseLink(UProperty* PoseProperty, int32 ArrayIndex);

	//
	virtual FAILinkMappingRecord GetLinkIDLocation(const UScriptStruct* NodeType, UEdGraphPin* InputLinkPin);

	/** Get the property (and possibly array index) associated with the supplied pin */
	virtual void GetPinAssociatedProperty(const UScriptStruct* NodeType, const UEdGraphPin* InputPin, UProperty*& OutProperty, int32& OutIndex) const;

	// Allocates or reallocates pins
	void InternalPinCreation(TArray<UEdGraphPin*>* OldPins);

	FOnNodePropertyChangedEvent PropertyChangeEvent;

private:
	TArray<FName> OldShownPins;
};
