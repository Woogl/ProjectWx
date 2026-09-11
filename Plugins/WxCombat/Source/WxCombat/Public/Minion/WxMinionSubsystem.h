// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "WxMinionSubsystem.generated.h"

class APawn;
class UAbilitySystemComponent;
struct FGameplayEventData;

/**
 * 소환물을 서버 권위로 생성해 주인별로 관리한다. AI 빙의는 소환물의 AutoPossessAI에 맡긴다.
 *
 * 주인·소환물 참조 규칙:
 * - 소환물 → 주인은 Instigator 하나가 답한다(GetMaster). 소환물이 죽어 로스터에서 내려가도 남는 영구 사실이다.
 * - 주인 → 소환물은 이 로스터 하나가 답한다. 서버 전용이고, 지금 살아서 명령을 받을 수 있는 것만 담는다.
 * - Owner는 소환 관계에 쓰지 않는다. 폰의 Owner는 빙의 시 Controller로 덮인다.
 * - 주인은 Pawn이다. 소환물이 주인을 Instigator로 무는 이상 다른 타입은 관계를 절반만 맺는다.
 *
 * 살아 있는 소환물 보유 여부는 로스터 변경 시 주인 ASC의 State.Minion.Active 태그로 복제한다.
 */
UCLASS()
class WXCOMBAT_API UWxMinionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * 이 폰을 소환한 주인. 소환된 적 없으면 null이라 이 값이 곧 소환물 여부다.
	 * APawn이 빈 인스티게이터를 자기 자신으로 채우므로 소환 여부 플래그를 따로 두지 않는다.
	 */
	static APawn* GetMaster(const APawn& Minion);

	/** 서버 로스터에서 가장 먼저 소환된 활성 소환물을 반환한다. */
	APawn* FindActiveMinion(const APawn& Master) const;

	/** SpawnTransform 은 월드 기준이다. 소환물 클래스가 선언한 상한을 넘치면 주인의 가장 오래된 소환물부터 파괴한다. */
	APawn* SpawnMinion(APawn& Master, TSubclassOf<APawn> MinionClass, const FTransform& SpawnTransform);

	/**
	 * 주인이 관리 중인 활성 소환물 모두에게 정확한 식별 태그의 어빌리티 발동을 요청하고, 발동을 수락한 소환물 수를 반환한다.
	 * Payload는 TriggerEventData로 전달되고 EventTag는 Event.CommandMinionAbility로 설정된다.
	 */
	int32 TryActivateAbilityOnMinions(APawn& Master, const FGameplayTag& AbilityTag, const FGameplayEventData& Payload);

protected:
	/** 에디터 월드에서는 시퀀서 프리뷰의 노티파이가 권위를 통과해 레벨에 스폰해 버리므로 게임 월드에만 만든다. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	UFUNCTION()
	void HandleMasterEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	UFUNCTION()
	void HandleMinionEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	/** 태그 이벤트는 어느 ASC에서 왔는지 주지 않으므로 구독할 때 소환물을 페이로드로 실어 둔다. */
	void HandleMinionDeathTagChanged(const FGameplayTag Tag, int32 NewCount, TWeakObjectPtr<APawn> Minion);

	void ReleaseMinion(APawn& Minion);

	void RefreshMasterStateTag(APawn& Master) const;

	bool TryActivateAbilityByExactTag(UAbilitySystemComponent& MinionASC, const FGameplayTag& AbilityTag, const FGameplayEventData& Payload) const;

	/** 주인 → 소환 순서의 살아 있는 소환물. 사망·파괴는 구독으로 즉시 내린다. */
	TMap<TWeakObjectPtr<APawn>, TArray<TWeakObjectPtr<APawn>>> Rosters;
};
