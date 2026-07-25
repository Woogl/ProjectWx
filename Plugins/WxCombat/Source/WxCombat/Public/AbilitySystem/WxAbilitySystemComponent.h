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

	/**
	 * 입력 액션을 누른 순간(엣지). 트리거 종류와 무관하게 누름당 한 번 들어온다.
	 * 최근 입력 기록·입력 대기 방송과, 이미 돌고 있는 어빌리티에 "다시 눌렀다"를 전달하는 일(콤보 재발동, 패링 중 가드 복귀)을 맡는다.
	 */
	void AbilityInputActionStarted(const UInputAction* Action);

	/**
	 * 입력 액션이 트리거 조건을 만족하고 있다(레벨). 홀드형 트리거는 눌려 있는 동안 매 프레임 들어온다.
	 * 아직 돌고 있지 않은 어빌리티를 발동시키는 일만 맡는다. 조건이 유지되는 동안 계속 시도하므로,
	 * 다른 어빌리티가 걸어 둔 차단이 풀리면 쥐고 있던 입력이 그 시점에 발동한다.
	 */
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
