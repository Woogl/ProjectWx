// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "WxMinionManagerComponent.generated.h"

class UAbilitySystemComponent;
struct FGameplayEventData;

/**
 * 소환 AnimNotify의 GameplayEvent 페이로드로 소환수와 설치물을 서버에서 생성하며, AI 빙의는 소환물의 AutoPossessAI에 맡긴다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXCOMBAT_API UWxMinionManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** 관리 중인 활성 소환수 모두에게 정확한 식별 태그의 어빌리티 발동을 요청한다. 서버에서 발동을 수락한 소환수 수를 반환한다. */
	int32 TryActivateAbilityOnMinions(const FGameplayTag& AbilityTag);

	/** Payload는 TriggerEventData로 전달되고 EventTag는 Event.CommandMinionAbility로 설정된다. */
	int32 TryActivateAbilityOnMinions(const FGameplayTag& AbilityTag, const FGameplayEventData& Payload);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** 동시에 유지할 소환물 수. 넘치면 가장 오래된 것을 파괴하고 새로 소환한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|Minion")
	int32 MaxMinionCount = 1;

	void HandleSpawnMinionEvent(const FGameplayEventData* Payload);

	void RemoveInvalidOrDeadMinions();

	bool TryActivateAbilityByExactTag(UAbilitySystemComponent& MinionASC, const FGameplayTag& AbilityTag, const FGameplayEventData& Payload) const;

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	FDelegateHandle SpawnMinionEventHandle;

	TArray<TWeakObjectPtr<AActor>> ActiveMinions;
};
