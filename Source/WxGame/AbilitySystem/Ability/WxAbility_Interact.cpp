// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Interact.h"
#include "AbilitySystem/TargetData/WxAbilityTargetData_Interaction.h"
#include "AbilitySystemComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameplayPrediction.h"
#include "Interaction/WxInteractionComponent.h"
#include "Interaction/WxInteractionRegistrySubsystem.h"
#include "MVVM/WxViewModel_Selection.h"
#include "System/WxUIManagerSubsystem.h"
#include "TimerManager.h"
#include "WxCollisionChannels.h"
#include "WxGameplayTags.h"

UWxAbility_Interact::UWxAbility_Interact()
{
	// 입력(Input.Interact)으로 활성화되는 1회성 실행 어빌리티. 감지(스캔)는 부여 동안 타이머로 상주한다.
	ActivationPolicy = EWxAbilityActivationPolicy::OnInputTriggered;

	// 클라가 선택을 읽어 서버로 전달해야 하므로 클라/서버 모두에서 활성화되는 LocalPredicted를 쓴다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 사망 중에는 활성화 거부. 입력 트리거라 ActivationBlockedTags가 활성화 자체를 막는다.
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
}

void UWxAbility_Interact::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	StartScanTimer(ActorInfo);
}

void UWxAbility_Interact::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	// 엔진 EndAbility는 이 어빌리티의 모든 월드 타이머를 비운다(ClearAllTimersForObject). 감지는 부여 동안 상시여야 하므로
	// 활성화(상호작용)가 끝나도 스캔 타이머를 다시 건다. 어빌리티 제거 경로면 직후 OnRemoveAbility가 타이머를 최종 정리한다.
	StartScanTimer(ActorInfo);
}

void UWxAbility_Interact::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	// 어빌리티 제거(언포제스/사망 리스폰 등) 시 스캔 타이머를 해제하고 잔여 후보를 비워 HUD 리스트·하이라이트를 정리한다.
	if (UWxInteractionRegistrySubsystem* Registry = GetLocalRegistry(ActorInfo))
	{
		const APlayerController* PlayerController = ActorInfo ? ActorInfo->PlayerController.Get() : nullptr;
		if (UWorld* World = PlayerController ? PlayerController->GetWorld() : nullptr)
		{
			World->GetTimerManager().ClearTimer(ScanTimerHandle);
		}

		Registry->UpdateInRange({});
		PushSelectionToViewModel(Registry);
	}

	Super::OnRemoveAbility(ActorInfo, Spec);
}

void UWxAbility_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (HasAuthority(&ActivationInfo) && IsLocallyControlled())
	{
		// 리슨 호스트/단일 PIE: 로컬 선택을 직접 읽어 즉시 실행(RPC 왕복 불필요).
		ExecuteInteract(GetLocalSelectedComponent(ActorInfo), ActorInfo);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (HasAuthority(&ActivationInfo))
	{
		// 서버의 원격 클라 인스턴스: 클라가 보낼 선택 컴포넌트 TargetData를 구독한다. 실행/종료는 수신 핸들러에서 한다.
		FAbilityTargetDataSetDelegate& Delegate = ASC->AbilityTargetDataSetDelegate(Handle, ActivationInfo.GetActivationPredictionKey());
		Delegate.AddUObject(this, &UWxAbility_Interact::HandleTargetDataReceived);

		// 활성화 RPC보다 TargetData가 먼저 도착해 위 구독을 놓치는 레이스를 방어한다(이미 와 있으면 즉시 발화).
		ASC->CallReplicatedTargetDataDelegatesIfSet(Handle, ActivationInfo.GetActivationPredictionKey());
		return;
	}

	// 원격 클라: 로컬 선택을 TargetData로 서버에 전송한 뒤 종료. 선택이 없으면 null을 보내 서버가 무동작하게 한다.
	FScopedPredictionWindow ScopedPrediction(ASC);

	FGameplayAbilityTargetDataHandle DataHandle;
	FWxAbilityTargetData_Interaction* TargetData = new FWxAbilityTargetData_Interaction();
	TargetData->Component = GetLocalSelectedComponent(ActorInfo);
	DataHandle.Add(TargetData);

	ASC->CallServerSetReplicatedTargetData(
		Handle,
		ActivationInfo.GetActivationPredictionKey(),
		DataHandle,
		FGameplayTag(),
		ASC->ScopedPredictionKey);

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UWxAbility_Interact::StartScanTimer(const FGameplayAbilityActorInfo* ActorInfo)
{
	// 감지는 로컬 어포던스. 소유 클라/리슨 호스트에서만 주기 스캔 타이머를 건다(데디 서버는 LocalPlayer 부재로 미설정).
	// 타이머는 월드 타이머매니저에 걸려 어빌리티 활성화와 무관히 부여 동안 틱한다(인스턴스는 InstancedPerActor로 상주).
	const APlayerController* PlayerController = ActorInfo ? ActorInfo->PlayerController.Get() : nullptr;
	UWorld* World = PlayerController ? PlayerController->GetWorld() : nullptr;
	if (!GetLocalRegistry(ActorInfo) || !World)
	{
		return;
	}

	World->GetTimerManager().SetTimer(ScanTimerHandle, this, &UWxAbility_Interact::ScanAndPush, FMath::Max(ScanInterval, 0.01f), true);

	// 설정 즉시 1회 스캔해 진입/재개 시점의 주변 상호작용을 바로 반영한다.
	ScanAndPush();
}

void UWxAbility_Interact::ScanAndPush()
{
	UWxInteractionRegistrySubsystem* Registry = GetLocalRegistry(GetCurrentActorInfo());
	if (!Registry)
	{
		return;
	}

	// 어빌리티가 지금 활성화 불가(사망 등 차단 태그·비용·쿨다운·요구 태그 미충족)면 스캔하지 않고 후보를 비운다.
	// 활성화 자체도 막히지만, 후보를 비워 선택/프롬프트/하이라이트까지 정리해 상호작용을 완전히 막는다.
	if (!CanActivateAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo()))
	{
		Registry->UpdateInRange({});
		PushSelectionToViewModel(Registry);
		return;
	}

	const APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	UWorld* World = AvatarPawn ? AvatarPawn->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	const FVector ScanOrigin = AvatarPawn->GetActorLocation();

	// 볼륨 메시가 WxInteractable 채널에 Overlap 응답으로 표식되므로 채널 오버랩으로 수집한다(메시의 ObjectType 은 불변).
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WxInteractionScan), false);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(Overlaps, ScanOrigin, FQuat::Identity, WxCollision::WxInteractable, FCollisionShape::MakeSphere(ScanRadius), QueryParams);

	// 후보 컴포넌트를 모은다. 오버랩 결과는 볼륨 프리미티브이므로 이를 참조하는 상호작용 컴포넌트로 역참조한다.
	// 한 액터에 여러 영역이 있으면(예: 엘리베이터) 컴포넌트 단위로 각각 수집한다.
	TArray<UWxInteractionComponent*> Candidates;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (UWxInteractionComponent* Component = UWxInteractionComponent::FindByCollisionVolume(Overlap.GetComponent()))
		{
			Candidates.AddUnique(Component);
		}
	}

	// 가까운 영역이 먼저 오도록 거리순 정렬한다(레지스트리가 신규를 이 순서로 append).
	Candidates.Sort([ScanOrigin](const UWxInteractionComponent& A, const UWxInteractionComponent& B)
	{
		return FVector::DistSquared(ScanOrigin, A.GetInteractionLocation()) < FVector::DistSquared(ScanOrigin, B.GetInteractionLocation());
	});

	Registry->UpdateInRange(Candidates);
	PushSelectionToViewModel(Registry);
}

void UWxAbility_Interact::PushSelectionToViewModel(UWxInteractionRegistrySubsystem* Registry)
{
	if (!Registry)
	{
		return;
	}

	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UWxUIManagerSubsystem* UIManager = GameInstance ? GameInstance->GetSubsystem<UWxUIManagerSubsystem>() : nullptr;
	UWxViewModel_Selection* ViewModel = UIManager ? UIManager->GetSelectionViewModel() : nullptr;
	if (!ViewModel)
	{
		return;
	}

	// 상호작용 컴포넌트는 현재 표시 데이터로 InteractionText 만 노출한다(Description/Icon 은 비움).
	if (const UWxInteractionComponent* Selected = Registry->GetSelectedComponent())
	{
		ViewModel->SetSelection(Selected->GetInteractionText(), FText::GetEmpty(), nullptr);
	}
	else
	{
		ViewModel->ClearSelection();
	}
}

void UWxAbility_Interact::HandleTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag)
{
	UWxInteractionComponent* Selected = nullptr;
	if (const FWxAbilityTargetData_Interaction* TargetData = static_cast<const FWxAbilityTargetData_Interaction*>(DataHandle.Get(0)))
	{
		Selected = TargetData->Component;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	}

	ExecuteInteract(Selected, CurrentActorInfo);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

UWxInteractionRegistrySubsystem* UWxAbility_Interact::GetLocalRegistry(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const APlayerController* PlayerController = ActorInfo ? ActorInfo->PlayerController.Get() : nullptr;
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	return LocalPlayer ? LocalPlayer->GetSubsystem<UWxInteractionRegistrySubsystem>() : nullptr;
}

UWxInteractionComponent* UWxAbility_Interact::GetLocalSelectedComponent(const FGameplayAbilityActorInfo* ActorInfo) const
{
	UWxInteractionRegistrySubsystem* Registry = GetLocalRegistry(ActorInfo);
	return Registry ? Registry->GetSelectedComponent() : nullptr;
}

void UWxAbility_Interact::ExecuteInteract(UWxInteractionComponent* Selected, const FGameplayAbilityActorInfo* ActorInfo)
{
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Selected || !Avatar)
	{
		return;
	}

	// 서버 권위 거리 검증: 감지·선택은 클라 로컬이라, 변조 클라가 임의의 원거리 컴포넌트를 보내 상호작용하는 것을 막는다.
	// 클라 스캔의 overlap 과 동일 판정 — 중심간 거리 <= ScanRadius + 볼륨 바운딩 반경 — 으로 사거리를 재확인한다.
	const float ReachRadius = ScanRadius + Selected->GetInteractionReachRadius();
	if (FVector::DistSquared(Avatar->GetActorLocation(), Selected->GetInteractionLocation()) > FMath::Square(ReachRadius))
	{
		return;
	}

	Selected->TryInteract(Avatar);
}
