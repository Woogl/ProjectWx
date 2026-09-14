// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "WxHitStopComponent.generated.h"

class UWxAbilitySystemComponent;

/**
 * 히트스톱 반응. 소유자 ASC에 Effect.HitStop이 붙어 있는 동안 아바타의 애니메이션을 세운다.
 *
 * 몽타주 재생 속도가 아니라 메시의 GlobalAnimRateScale을 쓴다 — 몽타주보다 아래층이라 슬롯 그룹·재생 주체·몽타주 교체를 보지 않고, 되돌릴 값이 상수 1이라 무엇을 얼렸는지 기억할 필요가 없다.
 * 배속의 주인은 히트스톱뿐이므로 다른 연출이 같은 값을 쓰기 시작하면 서로 덮는다.
 *
 * 모든 머신은 Effect.HitStop 태그로 판정하며, 클라이언트는 서버 태그의 복제를 따른다.
 * 이동 정지는 게임의 이동 컴포넌트가 같은 태그로 처리하며, 네트워크 갱신을 위해 이동 틱은 유지한다.
 */
UCLASS()
class WXCOMBAT_API UWxHitStopComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWxHitStopComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 지금 이 머신에서 시간이 서 있다. */
	bool IsFrozen() const;

private:
	void HandleHitStopTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** 토글이 아니라 대입이라 이벤트를 놓쳐도 다음 호출에서 참값으로 돌아온다. */
	void RefreshFrozenState();

	UPROPERTY()
	TObjectPtr<UWxAbilitySystemComponent> AbilitySystemComponent;
};
