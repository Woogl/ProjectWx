// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "WxAbilitySet.h"
#include "WxAbilitySystemComponent.generated.h"

class UInputAction;
class USkeletalMeshComponent;
struct FOnAttributeChangeData;

UCLASS()
class WXCOMBAT_API UWxAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UWxAbilitySystemComponent();

	virtual void BeginPlay() override;

	/** 몽타주를 시작한 엔진의 AnimatingAbility 전이에 맞춰 메시 본 갱신 정책을 승격한다. */
	virtual float PlayMontage(UGameplayAbility* AnimatingAbility, FGameplayAbilityActivationInfo ActivationInfo, UAnimMontage* Montage, float InPlayRate, FName StartSectionName = NAME_None, float StartTimeSeconds = 0.0f) override;

	/** 마지막 AnimatingAbility가 해제될 때 몽타주 전의 메시 본 갱신 정책을 복원한다. */
	virtual void ClearAnimatingAbility(UGameplayAbility* Ability) override;

	void GiveAbilitySets();

	/**
	 * 홀드형 트리거는 눌려 있는 동안 매 프레임 들어온다.
	 *
	 * 라이브 입력 라우팅의 유일한 진입점이다. 발동이 성립했는지를 돌려주고, 실패한 입력을 기억할지는 UWxInputBufferComponent가 정한다.
	 */
	bool AbilityInputActionTriggered(const UInputAction* Action);

	void AbilityInputActionReleased(const UInputAction* Action);

	/** 버퍼 재생 경로다. 뗀 뒤의 재생이라 스펙의 키 상태는 세우지 않는다. */
	bool TryActivateByInputAction(const UInputAction* Action);

	TArray<const UInputAction*> GetAbilityInputActions() const;

	/** 이 액터의 ASPD가 반영된 몽타주 재생 속도. 어빌리티가 오버라이드하지 않으면 그 어빌리티의 몽타주 재생 속도가 된다. */
	float GetMontagePlayRate() const;

	virtual void NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReason) override;

	/** 후딜에 들어 점유를 놓은 배타 어빌리티를 끊는다. */
	void CancelRecoveringAbilities(UGameplayAbility* IgnoreAbility);

private:
	/**
	 * SP를 소모하면 자연 회복을 멈춘다 — 소모 경로가 어빌리티 코스트와 질주 드레인으로 갈려 있어 어트리뷰트 감소를 접점으로 삼는다.
	 * 회복으로 늘어난 변화와 스태미나를 쓰지 않는 아바타는 제외한다.
	 */
	void HandleSPChanged(const FOnAttributeChangeData& ChangeData);

	void EnableAnimatingMontageMeshTick();
	void RestoreAnimatingMontageMeshTick();

	/** AnimatingAbility가 존재하는 동안에만 강제한 메시와 원래 옵션. 단일 소유자라 별도 참조 수가 필요 없다. */
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

	bool bAbilitySetsGranted = false;

};
