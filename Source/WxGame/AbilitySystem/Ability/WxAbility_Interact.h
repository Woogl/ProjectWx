// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Interact.generated.h"

class UAnimMontage;
class UWxInteractionComponent;
class UWxInteractionRegistrySubsystem;

/**
 * 상호작용 어빌리티(감지 + 입력 트리거 실행).
 *
 * 입력(Input.Interact) 시 활성화되어, 로컬 레지스트리의 현재 선택 컴포넌트를 대상으로 TryInteract를 실행하고 즉시 종료한다.
 * 감지(주변 스캔)는 어빌리티 활성화와 무관히 부여 동안 상주한다.
 * OnGiveAbility에서 월드 타이머를 걸어 주기 스캔하고, OnRemoveAbility에서 해제한다.
 * 어빌리티 인스턴스(InstancedPerActor)는 부여 동안 살아 있으므로 타이머가 활성화와 독립적으로 틱한다.
 * 스캔은 아바타 주변을 OverlapMultiByChannel(WxInteractable)로 수집(볼륨 프리미티브 → 상호작용 컴포넌트 역참조)해 거리순으로 로컬 레지스트리에 push 한다(감지는 로컬 어포던스).
 * 단 어빌리티가 활성화 불가(CanActivateAbility 실패, 예: 사망)인 동안에는 스캔을 건너뛰고 후보를 비워 선택/프롬프트/하이라이트를 정리한다.
 *
 * 선택은 클라의 레지스트리(로컬 전용)가 소유하므로, 입력 시 클라가 선택을 읽어 서버로 전달하고 실행은 서버 권한에서만 한다.
 *
 * 입력 흐름(ActivateAbility):
 *  - 리슨 호스트/단일 PIE(권한+로컬): 로컬 선택을 직접 읽어 TryInteract 즉시 호출 후 EndAbility
 *  - 원격 클라: 로컬 선택 컴포넌트를 TargetData로 서버에 전송 후 EndAbility
 *  - 서버(원격 클라 처리): 구독한 TargetData를 받아 선택 컴포넌트의 TryInteract 호출(권한) 후 EndAbility
 *  - 선택이 없으면 무동작. 사망(State.Dead) 중에는 ActivationBlockedTags로 활성화 자체가 막힌다.
 */
UCLASS(Abstract)
class WXGAME_API UWxAbility_Interact : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Interact();

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 주변 상호작용 볼륨을 수집할 반경(cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Interact")
	float ScanRadius = 150.f;

	/** 스캔 주기(초). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Interact")
	float ScanInterval = 0.1f;

	/**
	 * 상호작용 시 재생할 몽타주. 미설정이면 종전대로 즉시 실행 후 종료한다(모션 없음).
	 * 설정 시 활성화 즉시 상호작용을 실행하고, 몽타주 재생이 끝날 때 어빌리티를 종료한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Interact")
	TObjectPtr<UAnimMontage> InteractMontage = nullptr;

	/** 대상 응시 회전 보간 속도. 클수록 빨리 돌아본다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Interact")
	float FacingInterpSpeed = 10.f;

private:
	/**
	 * 월드 타이머매니저에 주기 스캔(ScanAndPush)을 걸고 진입 즉시 1회 스캔한다.
	 * 로컬에서만 설정(데디 서버는 미설정).
	 * OnGiveAbility와 EndAbility 양쪽에서 호출한다 — 엔진 EndAbility가 어빌리티의 모든 타이머를 비우므로 활성화 종료 후 재설정이 필요하다.
	 */
	void StartScanTimer(const FGameplayAbilityActorInfo* ActorInfo);

	/**
	 * 아바타 중심 SphereOverlap으로 후보 컴포넌트를 모아 거리순 정렬 후 레지스트리에 push 한다.
	 * 타이머가 주기 호출한다.
	 */
	void ScanAndPush();

	/**
	 * 서버가 클라이언트로부터 선택 컴포넌트 TargetData를 수신했을 때 호출.
	 * 권한에서 TryInteract 실행 후 EndAbility.
	 */
	void HandleTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag);

	/** ActorInfo의 로컬 플레이어 상호작용 레지스트리(로컬에서만 유효, 데디 서버는 nullptr). */
	UWxInteractionRegistrySubsystem* GetLocalRegistry(const FGameplayAbilityActorInfo* ActorInfo) const;

	/** 레지스트리에서 현재 선택 컴포넌트를 읽는다(로컬에서만 유효). */
	UWxInteractionComponent* GetLocalSelectedComponent(const FGameplayAbilityActorInfo* ActorInfo) const;

	/**
	 * 선택 컴포넌트가 유효하면 아바타를 instigator로 TryInteract 호출.
	 * 권한 분기에서만 호출한다.
	 */
	void ExecuteInteract(UWxInteractionComponent* Selected, const FGameplayAbilityActorInfo* ActorInfo);

	/**
	 * 레지스트리의 현재 선택을 UIManager 소유 글로벌 선택 뷰모델(UWxViewModel_Selection)에 반영한다.
	 * 레지스트리 멤버십 갱신(UpdateInRange) 직후 호출해 표시를 lockstep으로 유지한다(로컬 표시 전용).
	 */
	void PushSelectionToViewModel(UWxInteractionRegistrySubsystem* Registry);

	/**
	 * 상호작용 몽타주를 재생하고(있으면), 로컬 컨트롤 인스턴스에서는 대상 쪽으로 서서히 회전(응시)을 시작한다.
	 * 몽타주를 재생했으면 true — 이 경우 어빌리티는 즉시 끝나지 않고 몽타주 종료 델리게이트에서 종료된다.
	 * 몽타주가 없거나 대상이 없으면 false — 호출자가 종전대로 즉시 EndAbility 한다.
	 */
	bool PlayInteractMontage(UWxInteractionComponent* Target);

	//~ 몽타주 종료 계열 — 재생이 끝나면 어빌리티를 종료한다(Interrupted/Cancelled 는 취소 종료).
	UFUNCTION()
	void HandleInteractMontageCompleted();
	UFUNCTION()
	void HandleInteractMontageBlendOut();
	UFUNCTION()
	void HandleInteractMontageInterrupted();
	UFUNCTION()
	void HandleInteractMontageCancelled();

	/** 주기 스캔 타이머 핸들. OnGiveAbility에서 설정, OnRemoveAbility에서 해제. */
	FTimerHandle ScanTimerHandle;
};
