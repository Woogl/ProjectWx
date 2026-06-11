// Copyright Woogle. All Rights Reserved.

#include "WxAbilityThumbnailRenderer.h"

#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Component/WxAbilityComponent_UIData.h"
#include "Engine/Blueprint.h"
#include "Engine/Texture2D.h"

namespace WxAbilityThumbnailRenderer
{
	/** Blueprint 가 UWxAbilityBase 파생이고 UIData 컴포넌트를 가지면 그 아이콘 소프트 참조를 반환한다. 아니면 Null 참조. */
	static TSoftObjectPtr<UTexture2D> GetAbilityIcon(UObject* Object)
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

		const UWxAbilityComponent_UIData* UIData = AbilityCDO->FindComponent<UWxAbilityComponent_UIData>();
		return UIData != nullptr ? UIData->Icon : nullptr;
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
	if (const UTexture2D* IconTexture = WxAbilityThumbnailRenderer::GetAbilityIcon(Object).LoadSynchronous())
	{
		OutWidth = FMath::TruncToInt(Zoom * static_cast<float>(IconTexture->GetSizeX()));
		OutHeight = FMath::TruncToInt(Zoom * static_cast<float>(IconTexture->GetSizeY()));
		return;
	}

	Super::GetThumbnailSize(Object, Zoom, OutWidth, OutHeight);
}

void UWxAbilityThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	UTexture2D* IconTexture = WxAbilityThumbnailRenderer::GetAbilityIcon(Object).LoadSynchronous();
	if (IconTexture == nullptr || IconTexture->GetResource() == nullptr)
	{
		Super::Draw(Object, X, Y, Width, Height, RenderTarget, Canvas, bAdditionalViewFamily);
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
