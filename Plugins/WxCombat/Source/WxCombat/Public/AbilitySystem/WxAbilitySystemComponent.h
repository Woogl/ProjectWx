// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "WxAbilitySet.h"
#include "WxAbilitySystemComponent.generated.h"

class UInputAction;

/** 어빌리티 입력 액션이 트리거될 때마다 눌린 액션과 함께 방송한다. 활성 어빌리티의 입력 대기 태스크가 구독한다. */
DECLARE_MULTICAST_DELEGATE_OneParam(FWxInputActionTriggeredSignature, const UInputAction*);

UCLASS()
class WXCOMBAT_API UWxAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UWxAbilitySystemComponent();

	void GiveAbilitySet();

	/** 입력 액션에 매칭되는 어빌리티 활성화 (입력 눌림) */
	void AbilityInputActionTriggered(const UInputAction* Action);

	/** 입력 액션에 매칭되는 어빌리티에 입력 해제 전달 */
	void AbilityInputActionReleased(const UInputAction* Action);

	/** 어빌리티 입력 트리거 방송에 접근. 활성 어빌리티가 자기 관심 입력을 대기할 때 구독한다. */
	FWxInputActionTriggeredSignature& OnInputActionTriggered() { return OnInputActionTriggeredDelegate; }

	/** AbilitySet의 부여 대상 어빌리티들이 요구하는 입력 액션 전체를 반환한다(플레이어 입력 바인딩용). AbilitySet이 없으면 빈 배열을 반환한다. */
	TArray<const UInputAction*> GetAbilityInputActions() const;

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

	FWxInputActionTriggeredSignature OnInputActionTriggeredDelegate;
};
