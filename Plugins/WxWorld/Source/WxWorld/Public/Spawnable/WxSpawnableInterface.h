// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxSpawnableInterface.generated.h"

class UStreamableRenderAsset;

UINTERFACE(MinimalAPI)
class UWxSpawnableInterface : public UInterface
{
	GENERATED_BODY()
};

class WXWORLD_API IWxSpawnableInterface
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	/** 에디터 미리보기에 사용할 메시. USkeletalMesh 또는 UStaticMesh 반환 */
	virtual UStreamableRenderAsset* GetEditorPreviewMesh() const = 0;
#endif
};
