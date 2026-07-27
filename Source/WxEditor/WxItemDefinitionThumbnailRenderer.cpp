// Copyright Woogle. All Rights Reserved.

#include "WxItemDefinitionThumbnailRenderer.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Texture2D.h"
#include "Items/WxItemDefinition.h"
#include "Materials/MaterialInterface.h"

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
		UObject* IconAsset = ItemDefinition->Icon.LoadSynchronous();

		if (const UTexture2D* IconTexture = Cast<UTexture2D>(IconAsset))
		{
			OutWidth = FMath::TruncToInt(Zoom * static_cast<float>(IconTexture->GetSizeX()));
			OutHeight = FMath::TruncToInt(Zoom * static_cast<float>(IconTexture->GetSizeY()));
			return;
		}

		// 머터리얼은 고유 해상도가 없으므로 엔진 기본 썸네일 한 변(128)을 기준 정사각으로 그린다.
		if (IconAsset != nullptr && IconAsset->IsA<UMaterialInterface>())
		{
			OutWidth = FMath::TruncToInt(Zoom * 128.f);
			OutHeight = OutWidth;
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

	UObject* IconAsset = ItemDefinition->Icon.LoadSynchronous();

	if (const UTexture2D* IconTexture = Cast<UTexture2D>(IconAsset))
	{
		if (IconTexture->GetResource() == nullptr)
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
		return;
	}

	// 머터리얼 아이콘은 렌더 프록시로 그린다(블렌드 모드는 머터리얼 자신이 정한다).
	if (const UMaterialInterface* IconMaterial = Cast<UMaterialInterface>(IconAsset))
	{
		FCanvasTileItem TileItem(
			FVector2D(X, Y),
			IconMaterial->GetRenderProxy(),
			FVector2D(Width, Height));
		Canvas->DrawItem(TileItem);
	}
}
