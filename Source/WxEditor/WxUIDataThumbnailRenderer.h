// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ThumbnailRendering/BlueprintThumbnailRenderer.h"
#include "WxUIDataThumbnailRenderer.generated.h"

// 어빌리티·캐릭터·GameplayEffect의 저작 아이콘을 썸네일로 사용한다.
// 아이콘이 없거나 지원 대상이 아닌 Blueprint는 엔진 기본 동작으로 위임한다.
UCLASS()
class UWxUIDataThumbnailRenderer : public UBlueprintThumbnailRenderer
{
	GENERATED_BODY()

public:
	virtual bool CanVisualizeAsset(UObject* Object) override;

	virtual void GetThumbnailSize(UObject* Object, float Zoom, uint32& OutWidth, uint32& OutHeight) const override;

	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget* RenderTarget, FCanvas* Canvas, bool bAdditionalViewFamily) override;

private:
	static TSoftObjectPtr<UObject> GetIcon(UObject* Object);
};
