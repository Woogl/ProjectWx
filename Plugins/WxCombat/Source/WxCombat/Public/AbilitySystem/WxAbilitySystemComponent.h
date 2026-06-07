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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void GiveAbilitySet();

	/** 입력 태그에 매칭되는 어빌리티 활성화 (입력 눌림) */
	void AbilityInputTagPressed(const FGameplayTag& InputTag);

	/** 입력 태그에 매칭되는 어빌리티에 입력 해제 전달 */
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	/** 가장 최근에 눌린 입력 태그 반환 */
	const FGameplayTag& GetLastPressedInputTag() const;

	/** 가장 최근에 해제된 입력 태그 반환 */
	const FGameplayTag& GetLastReleasedInputTag() const;

	/**
	 * 후딜 캔슬 윈도우를 연다. WxAnimNotifyState_CancelWindow의 NotifyBegin에서 호출.
	 * ANS.CancelWindow 태그를 부여하고 진입 허용 목록을 등록한다.
	 * AllowedAbilityTags가 비어 있으면 어떤 어빌리티도 진입할 수 없다.
	 */
	void OpenCancelWindow(const FGameplayTagContainer& AllowedAbilityTags);

	/** 후딜 캔슬 윈도우를 닫는다. WxAnimNotifyState_CancelWindow의 NotifyEnd에서 호출 */
	void CloseCancelWindow();

	/**
	 * 후딜 캔슬 윈도우가 열려 있고 Tags가 화이트리스트에 부합하면 하드 차단(BlockAbilitiesWithTag)을 무시한다.
	 * 덕분에 CanActivateAbility는 차단 태그 조건만 건너뛰고 비용/쿨다운/상태 등 나머지는 그대로 검사한다.
	 */
	virtual bool AreAbilityTagsBlocked(const FGameplayTagContainer& Tags) const override;

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
	/** Ability, Effect 초기 데이터 */
	UPROPERTY(EditDefaultsOnly, Category = "Wx|GAS")
	TObjectPtr<UWxAbilitySet> AbilitySet;

	FWxAbilitySetGrantedHandles AbilitySetGrantedHandles;

	FGameplayTag LastPressedInputTag;
	FGameplayTag LastReleasedInputTag;

	/** 후딜 캔슬 윈도우 진입을 허용할 어빌리티 카테고리. 비어 있으면 진입 불가. ANS.CancelWindow 태그가 열림 여부를 표시 */
	FGameplayTagContainer CancelWindowAllowedTags;

	UPROPERTY(ReplicatedUsing = OnRep_RagdollActive)
	bool bRagdollActive = false;
};
