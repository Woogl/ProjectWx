// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_StartRecovery.generated.h"

/**
 * 후딜레이 구간 진입 지점 AnimNotify.
 * 후딜이 시작되는 프레임에 배치하면 재생 중인 어빌리티의 StartRecovery를 호출한다(자세한 의미는 그쪽 주석 참고).
 *
 * 후딜은 몽타주의 마지막 구간이라 종료가 곧 어빌리티 종료다.
 * 닫는 지점이 따로 필요 없어 State가 아닌 단발 Notify로 둔다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_StartRecovery : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
