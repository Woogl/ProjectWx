// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "WxAbility_Interact.generated.h"

class UWxInteractionComponent;

/**
 * 상호작용 어빌리티.
 *
 * 선택은 클라이언트의 UWxInteractionRegistrySubsystem(LocalPlayerSubsystem, 로컬 전용)이 소유하므로,
 * 클라가 선택을 읽어 서버로 전달하고 실행(TryInteract)은 서버 권한에서만 한다. 클라 선처리(예측)는 없다.
 *
 * 사용 흐름(LocalPredicted):
 *  1. 플레이어가 Input.Interact 입력 → 클라/서버에서 어빌리티 활성화
 *  2. 원격 클라: 로컬 레지스트리의 선택 컴포넌트를 TargetData로 서버에 전송 후 즉시 종료
 *  3. 서버(원격 클라 처리): TargetData 수신 → 선택 컴포넌트의 TryInteract 호출(권한)
 *  4. 리슨서버 호스트(권한+로컬): 로컬 선택을 직접 읽어 TryInteract 즉시 호출(RPC 왕복 없음)
 *  5. 선택이 없으면 아무 동작 없이 종료
 */
UCLASS()
class WXGAME_API UWxAbility_Interact : public UWxAbilityBase
{
	GENERATED_BODY()

public:
	UWxAbility_Interact();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	/** 서버가 클라이언트로부터 선택 컴포넌트 TargetData를 수신했을 때 호출. 권한에서 TryInteract 실행. */
	void HandleTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag);

	/** ActorInfo의 로컬 플레이어 레지스트리에서 현재 선택 컴포넌트를 읽는다(로컬에서만 유효). */
	UWxInteractionComponent* GetLocalSelectedComponent(const FGameplayAbilityActorInfo* ActorInfo) const;

	/** 선택 컴포넌트가 유효하면 아바타를 instigator로 TryInteract 호출. 권한 분기에서만 호출한다. */
	void ExecuteInteract(UWxInteractionComponent* Selected, const FGameplayAbilityActorInfo* ActorInfo);
};
