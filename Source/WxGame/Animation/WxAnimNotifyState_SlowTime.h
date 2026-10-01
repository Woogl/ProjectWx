// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/WxAnimNotify_AbilityEvent.h"
#include "WxAnimNotifyState_SlowTime.generated.h"

/**
 * 구간이 열려 있는 동안 전역 시간을 느리게 흘린다. 극한 회피·퍼펙트 가드의 슬로우 모션이 이걸로 열린다.
 *
 * 시작·종료 신호를 받은 어빌리티가 태스크를 소유한다. 구간 종료·몽타주 교체·어빌리티 취소 시 태스크를 정리한다.
 * 애니메이팅 어빌리티가 없는 Persona 프리뷰에서는 아예 걸리지 않는다.
 */
UCLASS()
class WXGAME_API UWxAnimNotifyState_SlowTime : public UWxAnimNotifyState_AbilityEvent
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual FLinearColor GetEditorColor() override;
#endif

	virtual FString GetNotifyName_Implementation() const override;

	/** 엔진이 Min/MaxGlobalTimeDilation으로 클램프한다. */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0.01"))
	float TimeDilation = 0.4f;
};
