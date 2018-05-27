// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "UtilityTreeEditorStyle.h"

#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "EditorStyleSet.h"
#include "Interfaces/IPluginManager.h"
#include "SlateOptMacros.h"

TSharedPtr< FSlateStyleSet > FUtilityTreeEditorStyle::StyleSet = nullptr;

void FUtilityTreeEditorStyle::Initialize()
{
    if (!StyleSet.IsValid())
    {
        Create();
        FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
    }
}

void FUtilityTreeEditorStyle::Shutdown()
{
    FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
    ensure(StyleSet.IsUnique());
    StyleSet.Reset();
}

FName FUtilityTreeEditorStyle::GetStyleSetName()
{
    static FName StyleSetName(TEXT("UtilityTreeEditorStyle"));
    return StyleSetName;
}

FString FUtilityTreeEditorStyle::InContent(const FString& RelativePath, const ANSICHAR* Extension)
{
    static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("AIExtension"))->GetContentDir();
    FString Debug = (ContentDir / TEXT("Editor") / RelativePath) + Extension;
    return ( ContentDir / TEXT("Editor") / RelativePath) + Extension;
}

#define IMAGE_PLUGIN_BRUSH( RelativePath, ... ) FSlateImageBrush( FUtilityTreeEditorStyle::InContent( TEXT(RelativePath), ".png" ), __VA_ARGS__ )
#define BOX_PLUGIN_BRUSH( RelativePath, ... ) FSlateBoxBrush( FUtilityTreeEditorStyle::InContent( TEXT(RelativePath), ".png" ), __VA_ARGS__ )
#define IMAGE_BRUSH( RelativePath, ...) FSlateImageBrush( StyleSet->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define BOX_BRUSH( RelativePath, ...) FSlateBoxBrush( StyleSet->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define TTF_PLUGIN_FONT( RelativePath, ... ) FSlateFontInfo( FUtilityTreeEditorStyle::InContent( TEXT(RelativePath), ".png" ), __VA_ARGS__ )
#define TTF_CORE_FONT( RelativePath, ... ) FSlateFontInfo( StyleSet->RootToCoreContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FUtilityTreeEditorStyle::Create()
{
    // Const icon sizes
    const FVector2D Icon8x8(8.0f, 8.0f);
    const FVector2D Icon10x10(10.0f, 10.0f);
    const FVector2D Icon16x16(16.0f, 16.0f);
    const FVector2D Icon20x20(20.0f, 20.0f);
    const FVector2D Icon32x32(32.0f, 32.0f);
    const FVector2D Icon40x40(40.0f, 40.0f);
    const FVector2D Icon64x64(64.0f, 64.0f);

    StyleSet = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

    StyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
    StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

    const FTextBlockStyle& NormalText = FEditorStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");

    // Quest Group graph styles
    {
        StyleSet->Set("UtilityTreeEditor.Graph.TextStyle", FTextBlockStyle(NormalText)
            .SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f))
            .SetFont(TTF_CORE_FONT("Fonts/Roboto-Regular", 8)));

        StyleSet->Set("UtilityTreeEditor.Graph.Node.Title", FTextBlockStyle(NormalText)
            .SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f))
            .SetFont(TTF_CORE_FONT("Fonts/Roboto-Bold", 8)));

        StyleSet->Set("UtilityTreeEditor.Graph.NodeBody",         new IMAGE_PLUGIN_BRUSH("UtilityTreeEditor/NodeBody", Icon16x16));
        StyleSet->Set("UtilityTreeEditor.Graph.NodeBody.Latent",  new IMAGE_PLUGIN_BRUSH("UtilityTreeEditor/NodeBodyLatent", Icon32x32, FLinearColor(1, 1, 1, 1), ESlateBrushTileType::Both));
        StyleSet->Set("UtilityTreeEditor.Graph.NodeIcon",         new IMAGE_PLUGIN_BRUSH("UtilityTreeEditor/NodeIcon", Icon16x16));
        StyleSet->Set("UtilityTreeEditor.Graph.LatentIcon",       new IMAGE_PLUGIN_BRUSH("UtilityTreeEditor/LatentIcon", Icon20x20));
        StyleSet->Set("UtilityTreeEditor.Graph.LatentDeniedIcon", new IMAGE_PLUGIN_BRUSH("UtilityTreeEditor/LatentDeniedIcon", Icon20x20));
        StyleSet->Set("UtilityTreeEditor.Graph.Pin.Background",   new IMAGE_PLUGIN_BRUSH("UtilityTreeEditor/NodePin", Icon10x10));
        StyleSet->Set("UtilityTreeEditor.Graph.Pin.BackgroundHovered", new IMAGE_PLUGIN_BRUSH("UtilityTreeEditor/NodePinHoverCue", Icon10x10));
        StyleSet->Set("UtilityTreeEditor.Graph.Pin.Disconnected.Background", new IMAGE_PLUGIN_BRUSH("UtilityTreeEditor/NodeDisconnectedPin", Icon10x10));
        StyleSet->Set("UtilityTreeEditor.Graph.Pin.Disconnected.BackgroundHovered", new IMAGE_PLUGIN_BRUSH("UtilityTreeEditor/NodeDisconnectedPinHoverCue", Icon10x10));
        StyleSet->Set("UtilityTreeEditor.Graph.Node.ShadowSelected", new BOX_PLUGIN_BRUSH("UtilityTreeEditor/NodeShadowSelected", FMargin(18.0f / 64.0f)));
        StyleSet->Set("UtilityTreeEditor.Graph.Node.Shadow", new BOX_BRUSH("Graph/RegularNode_shadow", FMargin(18.0f / 64.0f)));

        StyleSet->Set("Graph.AIPin.Connected",           new IMAGE_PLUGIN_BRUSH("AIGraph/AIPin_Connected",    Icon16x16));
        StyleSet->Set("Graph.AIPin.Disconnected",        new IMAGE_PLUGIN_BRUSH("AIGraph/AIPin_Disconnected", Icon16x16));
        StyleSet->Set("Graph.AIPin.ConnectedHovered",    new IMAGE_PLUGIN_BRUSH("AIGraph/AIPin_Connected",    Icon16x16, FLinearColor(0.8f, 0.8f, 0.8f)));
        StyleSet->Set("Graph.AIPin.DisconnectedHovered", new IMAGE_PLUGIN_BRUSH("AIGraph/AIPin_Disconnected", Icon16x16, FLinearColor(0.8f, 0.8f, 0.8f)));
    }
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef IMAGE_PLUGIN_BRUSH
#undef BOX_PLUGIN_BRUSH
#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef TTF_PLUGIN_FONT
#undef TTF_CORE_FONT

const TSharedPtr<ISlateStyle> FUtilityTreeEditorStyle::Get()
{
    return StyleSet;
}
#undef TTF_FONT