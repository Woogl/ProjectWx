// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WxEffect_MoveSpeedScale.generated.h"

/**
 * 이동 속도를 배율만큼 조절하는 무한 지속 GE.
 * 배율은 SetByCaller.MoveSpeedScale 로 실어 보낸다.
 *
 * SPD 반영은 AWxCharacterBase 의 SPD 콜백이 전담하므로, 일시적인 감속·가속은 CharacterMovement 를 직접 쓰지 말고 이 GE 를 부여·제거해서 표현한다.
 * AI 의 정찰·배회 감속(UWxBTTask_Patrol, UWxBTTask_Wander)이 BT 에셋에서 이 클래스를 지정해 사용한다.
 */
UCLASS()
class WXCOMBAT_API UWxEffect_MoveSpeedScale : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UWxEffect_MoveSpeedScale();
};
