// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "WxHitStopComponent.generated.h"

class UWxAbilitySystemComponent;

/**
 * Effect.HitStop이 있는 동안 소유자의 CustomTimeDilation을 낮추고 마지막 태그가 사라지면 복원한다.
 *
 * GE 수명은 월드 시간으로 흘러 GAS 쿨다운과 함께 정상 만료된다. 클라이언트도 복제된 태그로 배율을 적용한다.
 * 히트스톱 중 CustomTimeDilation의 쓰기는 이 컴포넌트가 소유한다.
 */
UCLASS()
class WXCOMBAT_API UWxHitStopComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxHitStopComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool IsFrozen() const;

private:
	void HandleHitStopTagChanged(const FGameplayTag Tag, int32 NewCount);

	void SetFrozen(bool bFrozen);

	bool bHitStopApplied = false;
	float SavedCustomTimeDilation = 1.f;

	UPROPERTY()
	TObjectPtr<UWxAbilitySystemComponent> AbilitySystemComponent;
};
