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

	// ── Combo ──────────────────────────────────────────────────────────────

	/** 콤보 윈도우가 열려 있는지 확인 */
	bool IsComboWindowOpen() const;

	/** 콤보 공격 요청. 콤보 윈도우 상태에 따라 적절한 단계의 어빌리티를 발동한다 */
	void RequestComboAttack(const FGameplayTag& InputTag);

protected:
	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;
	
	/** Ability, Effect 초기 데이터 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|GAS")
	TObjectPtr<UWxAbilitySet> AbilitySet;

	FWxAbilitySetGrantedHandles AbilitySetGrantedHandles;

	/** 입력 태그별 콤보 어빌리티 핸들 캐시. ComboIndex 순서로 정렬 */
	TMap<FGameplayTag, TArray<FGameplayAbilitySpecHandle>> ComboHandleMap;

	FGameplayTag ActiveComboInputTag;

	FGameplayAbilitySpecHandle ActiveComboHandle;

	int32 CurrentComboIndex = 0;
};
