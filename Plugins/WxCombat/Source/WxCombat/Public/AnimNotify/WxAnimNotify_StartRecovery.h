// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_StartRecovery.generated.h"

/**
 * 후딜레이 구간 진입 지점 AnimNotify.
 *
 * 어빌리티 몽타주에서 후딜레이가 시작되는 프레임에 배치한다. 발화 시 현재 몽타주를 재생 중인 WxAbilityBase의 StartRecovery를 호출한다.
 * 이 프로젝트에서 후딜레이 구간 = 캔슬 가능 구간이다. 그 지점부터 어빌리티 종료까지, 그 어빌리티가 건 하드 차단이 풀려 평소 막히던 어빌리티로 캔슬 진입할 수 있다.
 *
 * 후딜은 몽타주의 마지막 구간이라 종료=어빌리티 종료이므로 닫는 지점이 따로 필요 없어 State가 아닌 단발 Notify로 둔다.
 * 차단을 안 거는 어빌리티(예: Attack)의 몽타주에 배치하면 해제할 차단이 없어 무효과다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_StartRecovery : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
