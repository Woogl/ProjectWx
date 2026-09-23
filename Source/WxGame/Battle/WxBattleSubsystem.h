// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WxBattleSubsystem.generated.h"

class AWxCharacterBase;

DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnCurrentBossChanged, AWxCharacterBase* /*CurrentBoss*/);

/**
 * 월드의 전투 상황을 모은다. 교전 중인 보스를 교전 순서로 들고, 맨 앞 보스를 현재 보스로 삼는다.
 * 교전 상태는 머신마다 계산되므로 복제 없이 각 머신이 자기 월드에서 모은다.
 */
UCLASS()
class WXGAME_API UWxBattleSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 소멸도 비교전으로 알려야 목록에 남지 않는다. */
	void NotifyEngagementChanged(AWxCharacterBase* Character, bool bEngaged);

	AWxCharacterBase* GetCurrentBoss() const;

	/** 새 보스가 합류해도 먼저 교전한 보스가 빠질 때까지는 발행하지 않는다. */
	FWxOnCurrentBossChanged OnCurrentBossChanged;

private:
	TArray<TWeakObjectPtr<AWxCharacterBase>> EngagedBosses;
};
