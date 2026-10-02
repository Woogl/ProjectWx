// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/WxAbility_Combo.h"
#include "WxAbility_Skill.generated.h"

/**
 * ComboMontages의 첫 몽타주를 재생하고, 콤보 창 구간의 입력이 다음 단으로 넘긴다(터미널 단에서는 첫 단으로 되돌아간다).
 *
 * 한 활성화를 유지하며 WaitInputPress로 다음 단 입력을 동기화한다. 단계마다 CommitAbility가 새로 걸린다.
 *
 * 단계가 하나면 ComboWindow를 배치하지 않는 한 그 하나만 재생하고 종료한다.
 * 타겟 방향 회전은 WxAnimNotifyState_SnapToTarget이 담당.
 */
UCLASS(Abstract)
class WXGAME_API UWxAbility_Skill : public UWxAbility_Combo
{
	GENERATED_BODY()

public:
	UWxAbility_Skill();
};
