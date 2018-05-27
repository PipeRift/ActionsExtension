// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "UtilityTree/AIPins/SGraphPinAI.h"
#include "UtilityTreeEditorStyle.h"

/////////////////////////////////////////////////////
// SGraphPinPose

void SGraphPinAI::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
    SGraphPin::Construct(SGraphPin::FArguments(), InPin);

    CachedImg_Pin_ConnectedHovered = FUtilityTreeEditorStyle::GetBrush("Graph.AIPin.ConnectedHovered");
    CachedImg_Pin_Connected        = FUtilityTreeEditorStyle::GetBrush("Graph.AIPin.Connected");
    CachedImg_Pin_DisconnectedHovered = FUtilityTreeEditorStyle::GetBrush("Graph.AIPin.DisconnectedHovered");
    CachedImg_Pin_Disconnected     = FUtilityTreeEditorStyle::GetBrush("Graph.AIPin.Disconnected");
}

const FSlateBrush* SGraphPinAI::GetPinIcon() const
{
    const FSlateBrush* Brush = NULL;

    if (IsConnected())
    {
        Brush = IsHovered() ? CachedImg_Pin_ConnectedHovered : CachedImg_Pin_Connected;
    }
    else
    {
        Brush = IsHovered() ? CachedImg_Pin_DisconnectedHovered : CachedImg_Pin_Disconnected;
    }

    return Brush;
}
