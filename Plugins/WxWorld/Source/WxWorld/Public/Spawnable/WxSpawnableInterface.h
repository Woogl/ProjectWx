// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WxSpawnableInterface.generated.h"

class AWxSpawner;
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
	/**
	 * 스포너가 인스턴스를 스폰한 직후(빙의 전)에 호출한다. per-instance 컨텍스트를 스포너에서 끌어가는 훅.
	 * Deferred Spawn 의 FinishSpawning 이전에 불리므로, 여기서 세팅한 값은 AIController 빙의/BeginPlay 에서 사용할 수 있다.
	 */
	virtual void OnSpawnedBy(AWxSpawner* Spawner) {}

#if WITH_EDITOR
	/** 에디터 미리보기의 기준이 될 메시 컴포넌트. 스포너가 메시/트랜스폼/머티리얼을 추출한다. */
	virtual const UMeshComponent* GetEditorPreviewMeshComponent() const = 0;
#endif
};
