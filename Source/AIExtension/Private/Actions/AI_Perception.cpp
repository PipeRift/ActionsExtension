// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "AIExtensionPrivatePCH.h"
#include "AI_Perception.h"




void UAI_Perception::OnActivation()
{
	Super::OnActivation();

	if (!IsRunning())
		return;

	Perception = AI->GetPerceptionComponent();
	if (!Perception)
		Abort();
	else
	{
		Perception->OnTargetPerceptionUpdated.AddDynamic(this, &UAI_Perception::OnTargetUpdate);
	}

}
