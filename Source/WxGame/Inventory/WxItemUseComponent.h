// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WxItemUseComponent.generated.h"

class UAbilitySystemComponent;
class UWxItemDefinition;
struct FGameplayEventData;

/** AnimNotify GameplayEvent를 받아 서버 권위로 준비된 소비 아이템을 한 번 사용한다. */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXGAME_API UWxItemUseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	bool CanUseItem(const UWxItemDefinition* ItemDefinition) const;
	void BeginUseItem(UWxItemDefinition* ItemDefinition);
	void EndUseItem(const UWxItemDefinition* ItemDefinition);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleUseItemEvent(const FGameplayEventData* Payload);

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	FDelegateHandle UseItemEventHandle;

	UPROPERTY(Transient)
	TObjectPtr<UWxItemDefinition> PendingItemDefinition;
};
