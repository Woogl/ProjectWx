// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "WxFinisherDamageComponent.generated.h"

class UAbilitySystemComponent;
struct FGameplayEventData;

/** AnimNotify GameplayEvent를 받아 서버 권위로 준비된 처형 피해를 한 번 적용한다. */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXCOMBAT_API UWxFinisherDamageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void BeginFinisherDamage(const AActor* Target, const FDataTableRowHandle& DamageDataRow);
	void EndFinisherDamage(const AActor* Target);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleFinisherDamageEvent(const FGameplayEventData* Payload);

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	FDelegateHandle FinisherDamageEventHandle;
	TWeakObjectPtr<const AActor> PendingTarget;

	UPROPERTY(Transient)
	FDataTableRowHandle PendingDamageDataRow;
};
