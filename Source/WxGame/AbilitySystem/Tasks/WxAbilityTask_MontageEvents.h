// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ActiveGameplayEffectHandle.h"
#include "WxAbilityTask_MontageEvents.generated.h"

class UWxAbilityTask_Rush;
class UAnimMontage;
class UAnimNotifyState;
struct FBranchingPointNotifyPayload;

/** 몽타주 한 번의 노티파이 실행 상태. 교체 또는 소유 어빌리티 종료 시 구독과 구간 자원을 함께 정리한다. */
UCLASS()
class WXGAME_API UWxAbilityTask_MontageEvents : public UAbilityTask
{
	GENERATED_BODY()

public:
	static UWxAbilityTask_MontageEvents* CreateTask(UGameplayAbility* OwningAbility);

	/** 재생 전에 활성화해 구독하고, 재생 성공 직후 실제 인스턴스에 연결한다. */
	void BindToMontage(UAnimMontage* Montage);
	virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
	virtual void Activate() override;

private:
	struct FWxMontageWindow
	{
		uint64 ID = 0;
		TWeakObjectPtr<const UAnimNotifyState> Notify;
		float StartTime = 0.f;
		FActiveGameplayEffectHandle Effect;
		TWeakObjectPtr<class AWxWeaponBase> Weapon;
		FGuid WeaponAttackId;
		TWeakObjectPtr<class UWxAbilityTask_SlowTime> SlowTime;
	};
	bool OwnsMontageSignal(const FBranchingPointNotifyPayload& Payload) const;
	void HandleMontageNotify(const class UAnimNotify* Notify, const FBranchingPointNotifyPayload& Payload);
	void HandleGameplayWindow(const UAnimNotifyState* Notify, const FBranchingPointNotifyPayload& Payload, bool bBegin);
	void ClearMontageWindows();
	void ReleaseMontageWindow(const FWxMontageWindow& Window);
	TArray<FWxMontageWindow> MontageWindows;
	uint64 NextMontageWindowID = 0;
	FDelegateHandle MontageInstantNotifyHandle;
	void HandleMontageNotifyState(const UAnimNotifyState* Notify, const FBranchingPointNotifyPayload& Payload, bool bBegin);
	void ClearRushTask();
	FDelegateHandle MontageNotifyHandle;
	int32 OwnedMontageInstanceID = INDEX_NONE;
	TWeakObjectPtr<const UAnimNotifyState> ActiveRushNotify;
	float ActiveRushStartTime = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UWxAbilityTask_Rush> RushTask;
};
