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

	/** 가장 최근에 눌린 입력 태그 반환 */
	const FGameplayTag& GetLastPressedInputTag() const;

	/** 가장 최근에 해제된 입력 태그 반환 */
	const FGameplayTag& GetLastReleasedInputTag() const;

	/** 래그돌 활성 여부 설정. 서버에서 호출하면 ReplicatedUsing으로 모든 클라이언트(late joiner 포함)에 동기화된다. */
	void SetRagdollActive(bool bNewActive);

private:
	UFUNCTION()
	void OnRep_RagdollActive();

	/** LastPressedInputTag를 설정하고, 클라이언트이면 서버에 동기화 */
	void SetLastPressedInputTag(const FGameplayTag& InputTag);

	/** LastReleasedInputTag를 설정하고, 클라이언트이면 서버에 동기화 */
	void SetLastReleasedInputTag(const FGameplayTag& InputTag);

	UFUNCTION(Server, Reliable)
	void ServerSetLastPressedInputTag(const FGameplayTag& InputTag);

	UFUNCTION(Server, Reliable)
	void ServerSetLastReleasedInputTag(const FGameplayTag& InputTag);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Ability, Effect 초기 데이터 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|GAS")
	TObjectPtr<UWxAbilitySet> AbilitySet;

	FWxAbilitySetGrantedHandles AbilitySetGrantedHandles;

	FGameplayTag LastPressedInputTag;
	FGameplayTag LastReleasedInputTag;

	UPROPERTY(ReplicatedUsing = OnRep_RagdollActive)
	bool bRagdollActive = false;
};
