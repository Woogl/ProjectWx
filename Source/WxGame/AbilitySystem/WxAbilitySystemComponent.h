// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/WxAbilitySet.h"
#include "WxAbilitySystemComponent.generated.h"

class UInputAction;
class USkeletalMeshComponent;
struct FOnAttributeChangeData;
class UAnimNotifyState;
class UAnimNotify;
struct FBranchingPointNotifyPayload;

/** 로컬 몽타주 구간 신호. 수신자가 재생 인스턴스와 권한을 검증하며 RPC로 전달하지 않는다. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FWxMontageNotifyStateSignal, const UAnimNotifyState*, const FBranchingPointNotifyPayload&, bool);
DECLARE_MULTICAST_DELEGATE_TwoParams(FWxMontageNotifySignal, const UAnimNotify*, const FBranchingPointNotifyPayload&);

UCLASS()
class WXGAME_API UWxAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UWxAbilitySystemComponent();
	FWxMontageNotifyStateSignal OnMontageNotifyState;
	FWxMontageNotifySignal OnMontageNotify;

	virtual void BeginPlay() override;

	/** 어빌리티 몽타주가 시작되면 메시 본 갱신 정책을 승격하고, 그 메시의 몽타주가 모두 끝나면 되돌린다. */
	virtual float PlayMontage(UGameplayAbility* AnimatingAbility, FGameplayAbilityActivationInfo ActivationInfo, UAnimMontage* Montage, float InPlayRate, FName StartSectionName = NAME_None, float StartTimeSeconds = 0.0f) override;

	/** 서버가 호출한다. 다른 슬롯 그룹의 몽타주가 ASC 추적 자리를 차지해도 지정 몽타주를 모든 피어에서 정지한다. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopMontage(UAnimMontage* Montage);

	void GiveAbilitySets();

	/**
	 * 홀드형 트리거는 눌려 있는 동안 매 프레임 들어온다.
	 *
	 * 라이브 입력 라우팅의 유일한 진입점이다. 입력이 소비됐는지(콤보 창이 받았거나 발동이 성립)를 돌려주고, 실패한 입력을 기억할지는 UWxInputBufferComponent가 정한다.
	 */
	bool AbilityInputActionTriggered(const UInputAction* Action);

	void AbilityInputActionReleased(const UInputAction* Action);

	/**
	 * 버퍼 재생 경로이자 라이브 입력의 소비 판정이다. 뗀 뒤의 재생이라 스펙의 키 상태는 세우지 않는다.
	 * 순정 AbilityLocalInputPressed처럼 활성 스펙에는 InputPressed 이벤트를 보내고, 받은 태스크가 없을 때만 발동을 시도한다.
	 */
	bool TryActivateByInputAction(const UInputAction* Action);

	TArray<const UInputAction*> GetAbilityInputActions() const;

	/** 이 액터의 ASPD가 반영된 몽타주 재생 속도. UWxAbility_Combo 계열만 이 값을 몽타주 재생 속도로 쓰고, 나머지 어빌리티는 1이다. */
	float GetMontagePlayRate() const;

	/** Recovery 상태의 액션을 취소하되, 새로 발동한 IgnoreAbility는 제외한다. */
	void CancelRecoveringAbilities(UGameplayAbility* IgnoreAbility);

	/** 활성 어빌리티 하나가 건 차단 기여만 제외해 조회한다. 같은 태그를 건 다른 어빌리티·GE의 차단은 남는다. */
	bool AreAbilityTagsBlockedIgnoringContribution(const FGameplayTagContainer& AbilityTags, const FGameplayTagContainer& IgnoredBlockTags) const;

private:
	bool bAbilitySetsInitialized = false;

	/**
	 * SP를 소모하면 자연 회복을 멈춘다 — 소모 경로가 어빌리티 코스트와 질주 드레인으로 갈려 있어 어트리뷰트 감소를 접점으로 삼는다.
	 * 회복으로 늘어난 변화와 스태미나를 쓰지 않는 아바타는 제외한다.
	 */
	void HandleSPChanged(const FOnAttributeChangeData& ChangeData);

	void EnableAnimatingMontageMeshTick();

	UFUNCTION()
	void RestoreAnimatingMontageMeshTick();

	/** 몽타주가 남아 있는 동안 강제한 메시와 원래 옵션. 단일 소유자라 별도 참조 수가 필요 없다. */
	TWeakObjectPtr<USkeletalMeshComponent> MontageTickMesh;
	EVisibilityBasedAnimTickOption PreviousMontageTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

protected:
	/**
	 * 앞에서부터 순서대로 부여한다. 두 세트가 같은 어트리뷰트 행을 지정하면 뒤 세트가 앞 세트 값을 덮는다.
	 *
	 * 소유 캐릭터가 "Wx|GAS"를 쓰므로 여기서 같은 경로를 쓰면 Class Defaults 패널에 GAS 헤더가 두 번 그려진다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx")
	TArray<TObjectPtr<UWxAbilitySet>> AbilitySets;

};
