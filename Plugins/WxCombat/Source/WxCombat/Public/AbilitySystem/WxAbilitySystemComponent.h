// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "WxAbilitySet.h"
#include "WxAbilitySystemComponent.generated.h"

UCLASS()
class WXCOMBAT_API UWxAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UWxAbilitySystemComponent();

	void GiveAbilitySet();
	
	/** 입력 태그에 매칭되는 어빌리티 활성화 (입력 눌림) */
	void AbilityInputTagPressed(const FGameplayTag& InputTag);

	/** 입력 태그에 매칭되는 어빌리티에 입력 해제 전달 */
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

protected:
	/** Ability, Effect 초기 데이터 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|GAS")
	TObjectPtr<UWxAbilitySet> AbilitySet;

	FWxAbilitySetGrantedHandles AbilitySetGrantedHandles;
};
