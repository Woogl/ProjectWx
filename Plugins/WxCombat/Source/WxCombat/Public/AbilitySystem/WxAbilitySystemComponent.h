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
	 * 최근 입력 기록과 입력 대기 방송만 맡는다. 어빌리티 라우팅은 AbilityInputActionTriggered가 전담한다.
	 * Pressed 트리거는 이 둘이 같은 프레임에 들어오므로, 라우팅을 양쪽에 두면 한 번의 입력이 두 번 처리된다.
	 */
	void AbilityInputActionStarted(const UInputAction* Action);

	/**
	 * 입력 액션이 트리거 조건을 만족하고 있다(레벨). 홀드형 트리거는 눌려 있는 동안 매 프레임 들어온다.
	 * 어빌리티 라우팅의 유일한 진입점이다. 매칭된 어빌리티에 발동을 시도하고, 재발동을 받지 않는 어빌리티에는 입력을 그대로 전달한다(락온 토글 해제, 패링 중 가드 복귀).
	 * 발동은 조건이 유지되는 동안 계속 시도하므로, 다른 어빌리티가 걸어 둔 차단이 풀리면 쥐고 있던 입력이 그 시점에 발동한다.
	 * 반대로 이미 돌고 있는 어빌리티에는 홀드 반복분을 넘기지 않는다.
	 */
	void AbilityInputActionTriggered(const UInputAction* Action);

	/** 입력 액션에 매칭되는 어빌리티에 입력 해제 전달 */
	void AbilityInputActionReleased(const UInputAction* Action);

	/** 어빌리티 입력 트리거 방송에 접근. 활성 어빌리티가 자기 관심 입력을 대기할 때 구독한다. */
	FWxInputActionTriggeredSignature& OnInputActionTriggered();

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
	// 소유 캐릭터가 "Wx|GAS"를 쓰므로 여기서 같은 경로를 쓰면 Class Defaults 패널에 GAS 헤더가 두 번 그려진다.
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TObjectPtr<UWxAbilitySet> AbilitySet;

	FWxAbilitySetGrantedHandles AbilitySetGrantedHandles;

	UPROPERTY()
	TObjectPtr<const UInputAction> LastPressedInputAction;

	FWxInputActionTriggeredSignature OnInputActionTriggeredDelegate;
};
