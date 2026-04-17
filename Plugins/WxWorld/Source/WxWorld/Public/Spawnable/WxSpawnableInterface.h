// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxSpawnableInterface.generated.h"

class UMeshComponent;

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
	/** 에디터 미리보기의 기준이 될 메시 컴포넌트. 스포너가 메시/트랜스폼/머티리얼을 추출한다. */
	virtual const UMeshComponent* GetEditorPreviewMeshComponent() const = 0;
#endif
};
