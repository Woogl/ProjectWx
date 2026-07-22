// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ThumbnailRendering/BlueprintThumbnailRenderer.h"
#include "WxAbilityThumbnailRenderer.generated.h"

// UWxAbilityBase 파생 Blueprint 의 에디터 썸네일을 어빌리티 아이콘(AbilityDataRow) 텍스처로 렌더링한다.
// 아이콘이 지정되지 않은 경우, 그리고 어빌리티가 아닌 일반 Blueprint 는 엔진 기본 동작(Super)으로 위임한다.
UCLASS()
class UWxAbilityThumbnailRenderer : public UBlueprintThumbnailRenderer
{
	GENERATED_BODY()

public:
	virtual bool CanVisualizeAsset(UObject* Object) override;

	virtual void GetThumbnailSize(UObject* Object, float Zoom, uint32& OutWidth, uint32& OutHeight) const override;

	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily) override;
};
