// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "WxCueNotify_HitStop.generated.h"

/**
 * 역경직(히트 스톱) GameplayCue.
 *
 * 대상 캐릭터의 재생 중인 몽타주를 PauseDuration 동안 일시 정지한 뒤 자동 재개.
 * WxWeaponBase 적중 시 공격자에서 실행된다.
 *
 * GameplayCueManager에 등록되려면 이 클래스의 Blueprint 서브클래스 에셋이 필요.
 */
UCLASS(Abstract, Blueprintable)
class WXCOMBAT_API UWxCueNotify_HitStop : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UWxCueNotify_HitStop();
	
	virtual void HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

private:
	/** 캐릭터별 Resume 타이머 핸들. 연속 적중 시 이전 타이머를 교체하여 조기 Resume 방지 */
	TMap<TWeakObjectPtr<ACharacter>, FTimerHandle> ActiveTimerMap;
};
