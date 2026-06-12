// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxLockOnPointComponent.h"
#include "GameFramework/Actor.h"

UWxLockOnPointComponent::UWxLockOnPointComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 움직이는 캐릭터/본에 부착되어 매 프레임 위치가 갱신되어야 하므로 Movable로 둔다.
	Mobility = EComponentMobility::Movable;

#if WITH_EDITORONLY_DATA
	// 보이지 않는 마커이므로 에디터에서 위치를 스프라이트로 표시해 배치·선택을 돕는다.
	bVisualizeComponent = true;
#endif
}

USceneComponent* UWxLockOnPointComponent::ResolveLockOnTarget(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	// 락온 지점이 없는 액터는 락온 대상이 될 수 없다(nullptr 반환).
	return Actor->FindComponentByClass<UWxLockOnPointComponent>();
}
