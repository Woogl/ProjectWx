// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WxBossComponent.generated.h"

class AWxEnemyCharacter;
class UWxBossComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnBossReady, UWxBossComponent* /*BossComponent*/);

/**
 * 부착된 적을 보스로 표시한다. 보스 체력바(UWxViewModel_BossCharacter)가 이 표식으로 대상을 고른다.
 *
 * 보스는 종류가 아니라 역할이라 상속이 아닌 부착으로 가른다 — 같은 적 BP를 필드몹으로도 보스로도 쓸 수 있다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXGAME_API UWxBossComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	AWxEnemyCharacter* GetBossCharacter() const;

	/**
	 * 보스가 쓸 수 있게 될 때마다 발행된다. 배치(서버)·복제 도착(클라) 어느 경로든 BeginPlay 로 수렴한다.
	 * 관찰자가 보스보다 먼저 존재할 수 있어(HUD 뷰모델) 인스턴스가 아니라 클래스 차원에 둔다 — 구독자는 월드로 자기 것인지 가린다.
	 */
	static FWxOnBossReady OnAnyBossReady;
};
