// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "WxAnimNotify_FinisherDamage.generated.h"

/**
 * 처형 피해 적용 시점에 GameplayEvent를 발행한다.
 * 자신과 Mesh를 페이로드로 전달할 뿐, 피해 적용은 FinisherDamageComponent에 맡긴다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotify_FinisherDamage : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
