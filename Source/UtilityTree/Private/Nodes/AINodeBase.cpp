// Copyright 2015-2017 Piperift. All Rights Reserved.

#include "Nodes/AINodeBase.h"
//#include "UtilityTreeProxy.h"


/////////////////////////////////////////////////////
// FAILinkBase

void FAILinkBase::Initialize()
{
	//AttemptRelink();

#if ENABLE_AIGRAPH_TRAVERSAL_DEBUG
	//InitializationCounter.SynchronizeWith(Context.AnimInstanceProxy->GetInitializationCounter());

	// Initialization will require update to be called before an evaluate.
	UpdateCounter.Reset();
#endif

	// Do standard initialization
	if (LinkedNode != NULL)
	{
		LinkedNode->Initialize();
	}
}

void FAILinkBase::Update()
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FAILinkBase_Update);

#if WITH_EDITOR
	if (GIsEditor)
	{
		if (LinkedNode == NULL)
		{
			//@TODO: Should only do this when playing back
			AttemptRelink();
		}

		// Record the node line activation
		if (LinkedNode != NULL)
		{
			/*if (Context.UtilityTreeProxy->IsBeingDebugged())
			{
				Context.UtilityTreeProxy->RecordNodeVisit(LinkID, SourceLinkID, Context.GetFinalBlendWeight());
			}*/
		}
	}
#endif

#if ENABLE_AIGRAPH_TRAVERSAL_DEBUG
	//checkf(InitializationCounter.IsSynchronizedWith(Context.AnimInstanceProxy->GetInitializationCounter()), TEXT("Calling Update without initialization!"));
	//UpdateCounter.SynchronizeWith(Context.AnimInstanceProxy->GetUpdateCounter());
#endif

	if (LinkedNode != NULL)
	{
		LinkedNode->Update();
	}

}

void FAILinkBase::AttemptRelink()
{
	// Do the linkage
	if ((LinkedNode == NULL) && (LinkID != INDEX_NONE))
	{
		/*IAnimClassInterface* AnimBlueprintClass = Context.GetAnimClass();
		check(AnimBlueprintClass);

		// adding ensure. We had a crash on here
		if (ensure(AnimBlueprintClass->GetAnimNodeProperties().IsValidIndex(LinkID)))
		{
		UProperty* LinkedProperty = AnimBlueprintClass->GetAINodeProperties()[LinkID];
		void* LinkedNodePtr = LinkedProperty->ContainerPtrToValuePtr<void>(Context.AnimInstanceProxy->GetAnimInstanceObject());
		LinkedNode = (FAINode_Base*)LinkedNodePtr;
		}*/
	}
}

void FAILinkBase::SetLinkNode(struct FAINode_Base* NewLinkNode)
{
	// this is custom interface, only should be used by native handlers
	LinkedNode = NewLinkNode;
}

void FAILinkBase::GatherDebugData(FAINodeDebugData& DebugData)
{
	if (LinkedNode != NULL)
	{
		LinkedNode->GatherDebugData(DebugData);
	}
}

FAINode_Base* FAILinkBase::GetLinkNode()
{
	return LinkedNode;
}


/////////////////////////////////////////////////////
// FNodeDebugData

void FAINodeDebugData::AddDebugItem(FString DebugData, bool bPoseSource)
{
	check(NodeChain.Num() == 0 || NodeChain.Last().ChildNodeChain.Num() == 0); //Cannot add to this chain once we have branched

	NodeChain.Add( DebugItem(DebugData, bPoseSource) );
	NodeChain.Last().ChildNodeChain.Reserve(AI_NODE_DEBUG_MAX_CHILDREN);
}

FAINodeDebugData& FAINodeDebugData::BranchFlow(float BranchWeight, FString InNodeDescription)
{
	NodeChain.Last().ChildNodeChain.Add(FAINodeDebugData(UtilityTree, BranchWeight*AbsoluteWeight, InNodeDescription, RootNodePtr));
	NodeChain.Last().ChildNodeChain.Last().NodeChain.Reserve(AI_NODE_DEBUG_MAX_CHAIN);
	return NodeChain.Last().ChildNodeChain.Last();
}

FAINodeDebugData* FAINodeDebugData::GetCachePoseDebugData(float GlobalWeight)
{
	check(RootNodePtr);

	RootNodePtr->SaveCachePoseNodes.Add( FAINodeDebugData(UtilityTree, GlobalWeight, FString(), RootNodePtr) );
	RootNodePtr->SaveCachePoseNodes.Last().NodeChain.Reserve(AI_NODE_DEBUG_MAX_CHAIN);
	return &RootNodePtr->SaveCachePoseNodes.Last();
}

void FAINodeDebugData::GetFlattenedDebugData(TArray<FFlattenedDebugData>& FlattenedDebugData, int32 Indent, int32& ChainID)
{
	int32 CurrChainID = ChainID;
	for(DebugItem& Item : NodeChain)
	{
		FlattenedDebugData.Add( FFlattenedDebugData(Item.DebugData, AbsoluteWeight, Indent, CurrChainID, Item.bPoseSource) );
		bool bMultiBranch = Item.ChildNodeChain.Num() > 1;
		int32 ChildIndent = bMultiBranch ? Indent + 1 : Indent;
		for(FAINodeDebugData& Child : Item.ChildNodeChain)
		{
			if(bMultiBranch)
			{
				// If we only have one branch we treat it as the same really
				// as we may have only changed active status
				++ChainID;
			}
			Child.GetFlattenedDebugData(FlattenedDebugData, ChildIndent, ChainID);
		}
	}

	// Do CachePose nodes only from the root.
	if (RootNodePtr == this)
	{
		for (FAINodeDebugData& CachePoseData : SaveCachePoseNodes)
		{
			++ChainID;
			CachePoseData.GetFlattenedDebugData(FlattenedDebugData, 0, ChainID);
		}
	}
}

void FAIExposedValueCopyRecord::PostSerialize(const FArchive& Ar)
{
	// backwards compatibility: check value of deprecated source property and patch up property name
	if (SourceProperty_DEPRECATED && SourcePropertyName == NAME_None)
	{
		SourcePropertyName = SourceProperty_DEPRECATED->GetFName();
	}
}

void FAIExposedValueHandler::Initialize(FAINode_Base* AINode, UObject* UtilityTreeObject)
{
	if (bInitialized)
	{
		return;
	}

	if (BoundFunction != NAME_None)
	{
		// we cant call FindFunction on anything but the game thread as it accesses a shared map in the object's class
		check(IsInGameThread());
		Function = UtilityTreeObject->FindFunction(BoundFunction);
		check(Function);
	}
	else
	{
		Function = NULL;
	}

	// initialize copy records
	for (FAIExposedValueCopyRecord& CopyRecord : CopyRecords)
	{
		UProperty* SourceProperty = UtilityTreeObject->GetClass()->FindPropertyByName(CopyRecord.SourcePropertyName);
		check(SourceProperty);
		if (UArrayProperty* SourceArrayProperty = Cast<UArrayProperty>(SourceProperty))
		{
			// the compiler should not be generating any code that calls down this path at the moment - it is untested
			check(false);
			//	FScriptArrayHelper ArrayHelper(SourceArrayProperty, SourceProperty->ContainerPtrToValuePtr<uint8>(AnimInstanceObject));
			//	check(ArrayHelper.IsValidIndex(CopyRecord.SourceArrayIndex));
			//	CopyRecord.Source = ArrayHelper.GetRawPtr(CopyRecord.SourceArrayIndex);
			//	CopyRecord.Size = ArrayHelper.Num() * SourceArrayProperty->Inner->GetSize();
		}
		else
		{
			if (CopyRecord.SourceSubPropertyName != NAME_None)
			{
				void* Source = SourceProperty->ContainerPtrToValuePtr<uint8>(UtilityTreeObject, 0);
				UStructProperty* SourceStructProperty = CastChecked<UStructProperty>(SourceProperty);
				UProperty* SourceStructSubProperty = SourceStructProperty->Struct->FindPropertyByName(CopyRecord.SourceSubPropertyName);
				CopyRecord.Source = SourceStructSubProperty->ContainerPtrToValuePtr<uint8>(Source, CopyRecord.SourceArrayIndex);
				CopyRecord.Size = SourceStructSubProperty->GetSize();
				CopyRecord.CachedSourceProperty = SourceStructSubProperty;
				CopyRecord.CachedSourceContainer = Source;
			}
			else
			{
				CopyRecord.Source = SourceProperty->ContainerPtrToValuePtr<uint8>(UtilityTreeObject, CopyRecord.SourceArrayIndex);
				CopyRecord.Size = SourceProperty->GetSize();
				CopyRecord.CachedSourceProperty = SourceProperty;
				CopyRecord.CachedSourceContainer = UtilityTreeObject;
			}
		}

		if (UArrayProperty* DestArrayProperty = Cast<UArrayProperty>(CopyRecord.DestProperty))
		{
			FScriptArrayHelper ArrayHelper(DestArrayProperty, CopyRecord.DestProperty->ContainerPtrToValuePtr<uint8>(AINode));
			check(ArrayHelper.IsValidIndex(CopyRecord.DestArrayIndex));
			CopyRecord.Dest = ArrayHelper.GetRawPtr(CopyRecord.DestArrayIndex);

			if (CopyRecord.bInstanceIsTarget)
			{
				CopyRecord.CachedDestContainer = UtilityTreeObject;
			}
			else
			{
				CopyRecord.CachedDestContainer = AINode;
			}
		}
		else
		{
			CopyRecord.Dest = CopyRecord.DestProperty->ContainerPtrToValuePtr<uint8>(AINode, CopyRecord.DestArrayIndex);

			if (CopyRecord.bInstanceIsTarget)
			{
				CopyRecord.CachedDestContainer = UtilityTreeObject;
				CopyRecord.Dest = CopyRecord.DestProperty->ContainerPtrToValuePtr<uint8>(UtilityTreeObject, CopyRecord.DestArrayIndex);
			}
			else
			{
				CopyRecord.CachedDestContainer = AINode;
			}
		}

		if (UBoolProperty* BoolProperty = Cast<UBoolProperty>(CopyRecord.DestProperty))
		{
			CopyRecord.CopyType = EAICopyType::BoolProperty;
		}
		else if (UStructProperty* StructProperty = Cast<UStructProperty>(CopyRecord.DestProperty))
		{
			CopyRecord.CopyType = EAICopyType::StructProperty;
		}
		else if (UObjectPropertyBase* ObjectProperty = Cast<UObjectPropertyBase>(CopyRecord.DestProperty))
		{
			CopyRecord.CopyType = EAICopyType::ObjectProperty;
		}
		else
		{
			CopyRecord.CopyType = EAICopyType::MemCopy;
		}
	}

	bInitialized = true;
}

void FAIExposedValueHandler::Execute() const
{
	if (Function != nullptr)
	{
		//Context.UtilityTreeProxy->GetAnimInstanceObject()->ProcessEvent(Function, NULL);
	}

	for (const FAIExposedValueCopyRecord& CopyRecord : CopyRecords)
	{
		// if any of these checks fail then it's likely that Initialize has not been called.
		// has new anim node type been added that doesnt call the base class Initialize()?
		checkSlow(CopyRecord.Dest != nullptr);
		checkSlow(CopyRecord.Source != nullptr);
		checkSlow(CopyRecord.Size != 0);

		switch (CopyRecord.PostCopyOperation)
		{
		case EAIPostCopyOperation::None:
		{
			switch (CopyRecord.CopyType)
			{
			default:
			case EAICopyType::MemCopy:
				FMemory::Memcpy(CopyRecord.Dest, CopyRecord.Source, CopyRecord.Size);
				break;
			case EAICopyType::BoolProperty:
			{
				bool bValue = static_cast<UBoolProperty*>(CopyRecord.CachedSourceProperty)->GetPropertyValue_InContainer(CopyRecord.CachedSourceContainer);
				static_cast<UBoolProperty*>(CopyRecord.DestProperty)->SetPropertyValue_InContainer(CopyRecord.CachedDestContainer, bValue, CopyRecord.DestArrayIndex);
			}
			break;
			case EAICopyType::StructProperty:
				static_cast<UStructProperty*>(CopyRecord.DestProperty)->Struct->CopyScriptStruct(CopyRecord.Dest, CopyRecord.Source);
				break;
			case EAICopyType::ObjectProperty:
			{
				UObject* Value = static_cast<UObjectPropertyBase*>(CopyRecord.CachedSourceProperty)->GetObjectPropertyValue_InContainer(CopyRecord.CachedSourceContainer);
				static_cast<UObjectPropertyBase*>(CopyRecord.DestProperty)->SetObjectPropertyValue_InContainer(CopyRecord.CachedDestContainer, Value, CopyRecord.DestArrayIndex);
			}
			break;
			}
		}
		break;
		case EAIPostCopyOperation::LogicalNegateBool:
		{
			check(CopyRecord.CachedSourceProperty != nullptr && CopyRecord.DestProperty != nullptr);

			bool bValue = static_cast<UBoolProperty*>(CopyRecord.CachedSourceProperty)->GetPropertyValue_InContainer(CopyRecord.CachedSourceContainer);
			static_cast<UBoolProperty*>(CopyRecord.DestProperty)->SetPropertyValue_InContainer(CopyRecord.CachedDestContainer, !bValue, CopyRecord.DestArrayIndex);
		}
		break;
		}
	}
}


/////////////////////////////////////////////////////
// FAINode_Base

void FAINode_Base::Initialize()
{
	//EvaluateGraphExposedInputs.Initialize(this, Context.AnimInstanceProxy->GetAnimInstanceObject());
}

void FAINode_Base::Update() {}

//void FAINode_Base::Evaluate(FPoseContext& Output) {}

void FAINode_Base::OnInitializeUtilityTree(/*const FUtilityTreeProxy* InProxy, */const UUtilityTree* InUtilityTree)
{}
