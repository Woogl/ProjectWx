// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_StartRecovery.generated.h"

/**
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
