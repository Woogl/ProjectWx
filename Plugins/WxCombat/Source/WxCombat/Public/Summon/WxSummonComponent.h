// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "WxSummonComponent.generated.h"

class APawn;
class UAbilitySystemComponent;

/**
 * 플레이어가 소환한 전투 폰들을 보관한다.
 * 서버만 등록·해제하며, 각 소환수의 사망과 종료를 감지해 개별적으로 정리한다.
 */
UCLASS(ClassGroup=(Wx), meta=(BlueprintSpawnableComponent))
class WXCOMBAT_API UWxSummonComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	static UWxSummonComponent* FindComponent(const AActor* Actor);

	/** 소환수를 등록한다. 이미 등록된 소환수는 거부한다. 서버 전용이다. */
	bool RegisterSummon(APawn* Summon);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	struct FActiveSummon
	{
		TWeakObjectPtr<APawn> Pawn;
		TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
		FDelegateHandle DeathTagDelegateHandle;
	};

	void RemoveSummon(APawn* Summon, bool bDestroyActor);

	void RemoveSummonAt(int32 SummonIndex, bool bDestroyActor);

	void HandleSummonDeath(FGameplayTag CallbackTag, int32 NewCount, TWeakObjectPtr<APawn> Summon);

	UFUNCTION()
	void HandleSummonEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason);

	TArray<FActiveSummon> ActiveSummons;
};
