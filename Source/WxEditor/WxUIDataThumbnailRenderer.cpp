// Copyright Woogle. All Rights Reserved.

#include "WxUIDataThumbnailRenderer.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Blueprint.h"
#include "Engine/Texture2D.h"
#include "GameplayEffect.h"
#include "Materials/MaterialInterface.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Effect/WxEffectComponent_UIData.h"
#include "Character/WxCharacterBase.h"

TSoftObjectPtr<UObject> UWxUIDataThumbnailRenderer::GetIcon(UObject* Object)
{
	const UBlueprint* Blueprint = Cast<UBlueprint>(Object);
	const UClass* GeneratedClass = Blueprint ? Blueprint->GeneratedClass.Get() : nullptr;
	if (GeneratedClass == nullptr)
	{
		return nullptr;
	}

	const UObject* CDO = GeneratedClass->GetDefaultObject();

	if (const UWxAbilityBase* Ability = Cast<UWxAbilityBase>(CDO))
	{
		return Ability->GetIcon();
	}
	if (const AWxCharacterBase* Character = Cast<AWxCharacterBase>(CDO))
	{
		return Character->GetIcon();
	}
	if (const UGameplayEffect* Effect = Cast<UGameplayEffect>(CDO))
	{
		if (const UWxEffectComponent_UIData* Data = Effect->FindComponent<UWxEffectComponent_UIData>())
		{
			return Data->GetIcon();
		}
	}
	return nullptr;
}

bool UWxUIDataThumbnailRenderer::CanVisualizeAsset(UObject* Object)
{
	if (!GetIcon(Object).IsNull())
	{
		return true;
	}

	return Super::CanVisualizeAsset(Object);
}

void UWxUIDataThumbnailRenderer::GetThumbnailSize(UObject* Object, float Zoom, uint32& OutWidth, uint32& OutHeight) const
{
	// 텍스처만 고유 해상도를 갖는다.
	if (const UTexture2D* IconTexture = Cast<UTexture2D>(GetIcon(Object).LoadSynchronous()))
	{
		OutWidth = FMath::TruncToInt(Zoom * static_cast<float>(IconTexture->GetSizeX()));
		OutHeight = FMath::TruncToInt(Zoom * static_cast<float>(IconTexture->GetSizeY()));
		return;
	}

	Super::GetThumbnailSize(Object, Zoom, OutWidth, OutHeight);
}

void UWxUIDataThumbnailRenderer::Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily)
{
	UObject* IconAsset = GetIcon(Object).LoadSynchronous();

	if (const UTexture2D* IconTexture = Cast<UTexture2D>(IconAsset))
	{
		if (IconTexture->GetResource() != nullptr)
		{
			// 아이콘은 알파 채널을 가질 수 있다.
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
