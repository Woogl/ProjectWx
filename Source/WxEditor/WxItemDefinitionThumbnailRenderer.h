// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ThumbnailRendering/ThumbnailRenderer.h"
#include "WxItemDefinitionThumbnailRenderer.generated.h"

// UWxItemDefinition 의 에디터 썸네일을 해당 아이템의 아이콘 텍스처로 렌더링한다.
// 아이콘이 지정되지 않은 경우 CanVisualizeAsset 이 false 를 반환하여 기본 썸네일로 폴백한다.
UCLASS()
class UWxItemDefinitionThumbnailRenderer : public UThumbnailRenderer
{
	GENERATED_BODY()

public:
	virtual bool CanVisualizeAsset(UObject* Object) override;

	virtual void GetThumbnailSize(UObject* Object, float Zoom, uint32& OutWidth, uint32& OutHeight) const override;

	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily) override;
};
