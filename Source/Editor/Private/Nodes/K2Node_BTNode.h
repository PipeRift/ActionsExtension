// Copyright 2015-2017 Piperift. All Rights Reserved.

#pragma once
#include "K2Node.h"
#include "K2Node_ConstructAsyncObjectFromClass.h"
#include "K2Node_BTNode.generated.h"
 
UCLASS(BlueprintType, Blueprintable)
class AIEXTENSIONEDITOR_API UK2Node_BTNode : public UK2Node_ConstructAsyncObjectFromClass
{
    GENERATED_UCLASS_BODY()
 
    // Begin UEdGraphNode interface.
    virtual void AllocateDefaultPins() override;
    virtual FLinearColor GetNodeTitleColor() const override;
    //this is where node will be expanded with additional pins!
    virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
    // End UEdGraphNode interface.
 
    // Begin UK2Node interface
    virtual FText GetMenuCategory() const override;
    // End UK2Node interface.
 
protected:
    /** Gets the default node title when no class is selected */
    virtual FText GetBaseNodeTitle() const;
    /** Gets the node title when a class has been selected. */
    virtual FText GetNodeTitleFormat() const;
    /** Gets base class to use for the 'class' pin.  UObject by default. */
    virtual UClass* GetClassPinBaseClass() const;
    /**  */
    virtual bool IsSpawnVarPin(UEdGraphPin* Pin) override;
};