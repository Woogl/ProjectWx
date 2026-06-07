// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "WxAnimNotifyState_CancelWindow.generated.h"

/**
 * 후딜 캔슬 수용 구간 AnimNotifyState.
 *
 * 어빌리티 몽타주의 후딜레이 구간에 배치하면 NotifyBegin~NotifyEnd 동안 캐릭터 ASC에 ANS.CancelWindow 태그를 부여하고 진입 허용 목록을 등록한다.
 * 이 구간에서 AllowedAbilities에 해당하는 다른 어빌리티가 실제로 발동에 성공하면, 후딜 중인 현재 어빌리티를 캔슬한다.
 *
 * AllowedAbilities를 비워두면 어떤 어빌리티도 진입할 수 없다.
 * 발동 가능 여부(비용/쿨다운/상태)는 그대로 검사되므로, 정작 못 쓰는 입력(쿨다운 중 등)은 후딜을 끊지 않는다.
 * 후딜 어빌리티가 BlockAbilitiesWithTag로 하드 차단하더라도, 화이트리스트에 있으면 차단을 무시하고 진입한다.
 */
UCLASS()
class WXCOMBAT_API UWxAnimNotifyState_CancelWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	/** 이 구간에서 현재 어빌리티를 캔슬하고 진입할 수 있는 어빌리티 카테고리. 비우면 어떤 어빌리티도 진입 불가 */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (Categories = "Ability"))
	FGameplayTagContainer AllowedAbilities;
};
