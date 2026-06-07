// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_ClearAbilityBlock.generated.h"

/**
 * 후딜 캔슬 수용 구간 AnimNotifyState.
 *
 * 어빌리티 몽타주의 후딜레이 구간에 배치하면, NotifyBegin 시점에 오너 ASC가 현재 걸고 있는 어빌리티 차단(BlockedAbilityTags)을 해제한다.
 * 그 결과 후딜 동안 다른 어빌리티가 발동 가능해지고, 진입한 어빌리티가 CancelAbilitiesWithTag로 후딜 중인 어빌리티를 캔슬한다.
 *
 * 비용/쿨다운/ActivationBlockedTags 등 나머지 발동 조건은 그대로 검사되므로, 정작 못 쓰는 입력(쿨다운 중 등)은 후딜을 끊지 않는다.
 *
 * 차단 해제는 의도적으로 NotifyEnd에서 복구하지 않는다.
 * 후딜은 몽타주의 마지막 구간이므로 한 번 열리면 어빌리티 종료까지 캔슬 가능한 상태로 두고, 차단은 어빌리티가 끝날 때 엔진이 자연히 되돌린다(EndAbility의 BlockAbilitiesWithTag 해제).
 *
 * NotifyBegin~NotifyEnd 동안 ANS.CancelWindow 태그를 부여한다(게이팅이 아닌 관찰/디버그용 신호).
 * 차단을 걸지 않는 어빌리티(예: Attack)의 후딜에 배치하면 해제할 차단이 없어 무효과다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotifyState_ClearAbilityBlock : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
