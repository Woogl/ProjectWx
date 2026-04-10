// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WxLockOnComponent.generated.h"

/**
 * 락온 대상을 관리하는 컴포넌트.
 * 캐릭터에 부착하여 현재 락온 중인 액터를 저장/조회한다.
 */
UCLASS()
class WXCOMBAT_API UWxLockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 액터에서 UWxLockOnComponent를 찾아 반환. 없으면 nullptr */
	static UWxLockOnComponent* FindComponent(const AActor* Actor);

	/** 락온 대상 설정/해제. nullptr로 해제 */
	void SetLockOnTarget(AActor* InTarget);

	/** 현재 락온 대상 반환. 없으면 nullptr */
	AActor* GetLockOnTarget() const;

private:
	TWeakObjectPtr<AActor> LockOnTarget;
};
