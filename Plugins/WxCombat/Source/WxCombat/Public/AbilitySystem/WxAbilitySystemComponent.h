// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "WxAbilitySet.h"
#include "WxAbilitySystemComponent.generated.h"

class UInputAction;

UCLASS()
class WXCOMBAT_API UWxAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UWxAbilitySystemComponent();

	void GiveAbilitySet();

	/** 입력 액션에 매칭되는 어빌리티 활성화 (입력 눌림) */
	void AbilityInputActionPressed(const UInputAction* Action);

	/** 입력 액션에 매칭되는 어빌리티에 입력 해제 전달 */
	void AbilityInputActionReleased(const UInputAction* Action);

	/** 가장 최근에 눌린 입력 액션 반환 */
	const UInputAction* GetLastPressedInputAction() const;

private:
	/** LastPressedInputAction을 설정하고, 클라이언트이면 서버에 동기화 */
	void SetLastPressedInputAction(const UInputAction* Action);

	UFUNCTION(Server, Reliable)
	void ServerSetLastPressedInputAction(const UInputAction* Action);

protected:
	/** Ability, Effect 초기 데이터 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|GAS")
	TObjectPtr<UWxAbilitySet> AbilitySet;

	FWxAbilitySetGrantedHandles AbilitySetGrantedHandles;

	UPROPERTY()
	TObjectPtr<const UInputAction> LastPressedInputAction;
};
