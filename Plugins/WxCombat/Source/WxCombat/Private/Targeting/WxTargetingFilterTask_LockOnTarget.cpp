// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxTargetingFilterTask_LockOnTarget.h"

#include "Targeting/WxLockOnComponent.h"
#include "Types/TargetingSystemTypes.h"

bool UWxTargetingFilterTask_LockOnTarget::ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const
{
	// 조회는 소스가 널이어도 안전하므로, 컨텍스트가 없는 요청도 "겨누는 대상 없음" 과 같은 경로로 답한다.
	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	const AActor* LockOnTarget = UWxLockOnComponent::ResolveLockOnTargetActor(SourceContext ? SourceContext->SourceActor.Get() : nullptr);

	// 반환 true 는 엔진이 "이 결과를 뺀다" 로 읽는다.
	if (!LockOnTarget)
	{
		return !bSkipIfEmpty;
	}

	return TargetData.HitResult.GetActor() != LockOnTarget;
}
