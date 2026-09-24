// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WxItemUseComponent.generated.h"

class UAbilitySystemComponent;
struct FGameplayEventData;

/** AnimNotify GameplayEvent를 받아 서버 권위로 소비 아이템을 한 번 사용한다. 어떤 아이템인지는 인벤토리가 고른다. */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXINVENTORY_API UWxItemUseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	bool CanUseItem() const;
	void BeginUseItem();
	void EndUseItem();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleUseItemEvent(const FGameplayEventData* Payload);

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	FDelegateHandle UseItemEventHandle;

	/** 사용을 시작한 뒤 아직 소비하지 않았다. 노티파이 한 번에 한 번만 소비한다. */
	bool bUsePending = false;
};
