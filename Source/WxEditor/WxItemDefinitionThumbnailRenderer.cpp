// Copyright Woogle. All Rights Reserved.

#include "WxItemDefinitionThumbnailRenderer.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Texture2D.h"
#include "Items/WxItemDefinition.h"

bool UWxItemDefinitionThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	const UWxItemDefinition* ItemDefinition = Cast<UWxItemDefinition>(Object);
	return ItemDefinition != nullptr && !ItemDefinition->Icon.IsNull();
}

void UWxItemDefinitionThumbnailRenderer::GetThumbnailSize(UObject* Object, float Zoom, uint32& OutWidth, uint32& OutHeight) const
{
	const UWxItemDefinition* ItemDefinition = Cast<UWxItemDefinition>(Object);
	if (ItemDefinition != nullptr)
	{
		if (const UTexture2D* IconTexture = ItemDefinition->Icon.LoadSynchronous())
		{
			OutWidth = FMath::TruncToInt(Zoom * static_cast<float>(IconTexture->GetSizeX()));
			OutHeight = FMath::TruncToInt(Zoom * static_cast<float>(IconTexture->GetSizeY()));
			return;
		}
	}

	OutWidth = 0;
	OutHeight = 0;
}

void UWxItemDefinitionThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	const UWxItemDefinition* ItemDefinition = Cast<UWxItemDefinition>(Object);
	if (ItemDefinition == nullptr)
	{
		return;
	}

	UTexture2D* IconTexture = ItemDefinition->Icon.LoadSynchronous();
	if (IconTexture == nullptr || IconTexture->GetResource() == nullptr)
	{
		return;
	}

	// 아이콘은 알파 채널을 가질 수 있으므로 Translucent 블렌드로 그린다.
	FCanvasTileItem TileItem(
		FVector2D(X, Y),
		IconTexture->GetResource(),
		FVector2D(Width, Height),
		FLinearColor::White);
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem);
}
