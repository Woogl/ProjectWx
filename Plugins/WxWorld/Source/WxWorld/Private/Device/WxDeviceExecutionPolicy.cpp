// Copyright Woogle. All Rights Reserved.

#include "Device/WxDeviceExecutionPolicy.h"
#include "Device/WxDeviceStateTreeComponent.h"
#include "GameFramework/Actor.h"
#include "StateTreeExecutionContext.h"

bool FWxDeviceExecutionPolicy::IsRestoring(const FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition)
{
	// 트리 시작(재시작 포함)은 엔진이 소스 상태를 비워 둔다. 그 밖의 복원은 장치가 스냅샷으로 요청한 전이다.
	if (!Transition.SourceStateID.IsValid())
	{
		return true;
	}

	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	const UWxDeviceStateTreeComponent* Component = Owner ? Owner->FindComponentByClass<UWxDeviceStateTreeComponent>() : nullptr;
	return Component && Component->IsRestoringState();
}
