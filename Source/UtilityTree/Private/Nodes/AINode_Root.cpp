// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "Nodes/AINode_Root.h"

/////////////////////////////////////////////////////
// FAINode_Root

FAINode_Root::FAINode_Root()
{
}

void FAINode_Root::Initialize()
{
	FAINode_Base::Initialize();

	Result.Initialize();
}

void FAINode_Root::Update()
{
	//EvaluateGraphExposedInputs.Execute(Context);
	Result.Update();
}

void FAINode_Root::GatherDebugData(FAINodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);
	DebugData.AddDebugItem(DebugLine);
	Result.GatherDebugData(DebugData);
}
