// Copyright Woogle. All Rights Reserved.

#include "WxAbilityThumbnailRenderer.h"

#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Blueprint.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"

namespace WxAbilityThumbnailRenderer
{
	static TSoftObjectPtr<UObject> GetAbilityIcon(UObject* Object)
	{
		const UBlueprint* Blueprint = Cast<UBlueprint>(Object);
		if (Blueprint == nullptr || Blueprint->GeneratedClass == nullptr || !Blueprint->GeneratedClass->IsChildOf(UWxAbilityBase::StaticClass()))
		{
			return nullptr;
		}

		const UWxAbilityBase* AbilityCDO = Blueprint->GeneratedClass->GetDefaultObject<UWxAbilityBase>();
		if (AbilityCDO == nullptr)
		{
			return nullptr;
		}

		return AbilityCDO->GetIcon();
	}
}

bool UWxAbilityThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	if (!WxAbilityThumbnailRenderer::GetAbilityIcon(Object).IsNull())
	{
		return true;
	}

	return Super::CanVisualizeAsset(Object);
}

void UWxAbilityThumbnailRenderer::GetThumbnailSize(UObject* Object, float Zoom, uint32& OutWidth, uint32& OutHeight) const
{
	// 텍스처만 고유 해상도를 갖는다.
	if (const UTexture2D* IconTexture = Cast<UTexture2D>(WxAbilityThumbnailRenderer::GetAbilityIcon(Object).LoadSynchronous()))
	{
		OutWidth = FMath::TruncToInt(Zoom * static_cast<float>(IconTexture->GetSizeX()));
		OutHeight = FMath::TruncToInt(Zoom * static_cast<float>(IconTexture->GetSizeY()));
		return;
	}

	Super::GetThumbnailSize(Object, Zoom, OutWidth, OutHeight);
}

void UWxAbilityThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	UObject* IconAsset = WxAbilityThumbnailRenderer::GetAbilityIcon(Object).LoadSynchronous();

	if (const UTexture2D* IconTexture = Cast<UTexture2D>(IconAsset))
	{
		if (IconTexture->GetResource() != nullptr)
		{
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
	}
	// 머터리얼 아이콘의 블렌드 모드는 머터리얼 자신이 정한다.
	else if (const UMaterialInterface* IconMaterial = Cast<UMaterialInterface>(IconAsset))
	{
		FCanvasTileItem TileItem(
			FVector2D(X, Y),
			IconMaterial->GetRenderProxy(),
			FVector2D(Width, Height));
		Canvas->DrawItem(TileItem);
		return;
	}

	Super::Draw(Object, X, Y, Width, Height, RenderTarget, Canvas, bAdditionalViewFamily);
}
