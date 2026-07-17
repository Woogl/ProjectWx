// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Interact.h"
#include "AbilitySystem/Task/WxAbilityTask_TurnAround.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/WxInteractionComponent.h"
#include "Interaction/WxInteractionRegistrySubsystem.h"
#include "MVVM/WxViewModel_Selection.h"
#include "System/WxUIManagerSubsystem.h"
#include "TimerManager.h"
#include "WxCollisionChannels.h"
#include "WxGameplayTags.h"

UWxAbility_Interact::UWxAbility_Interact()
{
	// 상호작용 입력을 받은 캐릭터가 선택 대상을 실어 보내는 GameplayEvent 로 발동한다.
	// 감지(스캔)는 부여 동안 타이머로 상주한다.
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = WxGameplayTags::Event_Interact;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);

	// 클라가 선택 대상을 이벤트 페이로드로 실어 보내는 순정 통로(ServerTryActivateAbilityWithEventData)는 LocalPredicted 분기에만 존재한다.
	// 그래서 LocalPredicted 를 쓴다. 다만 예측하는 것은 로컬 몽타주·응시(코스메틱)뿐이고, 실제 실행(TryInteract)은 아래 ExecuteInteract 의 권위 게이트를 통과한다.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 사망 중에는 활성화 거부.
	// 이벤트로 활성화하는 클라/서버 양쪽의 CanActivateAbility 가 검사한다.
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);

	// 처형 연출 중에는 상호작용 재입력을 막는다(WxAbility_Finisher가 State.Finisher를 발행).
	// 연출 도중 근처 다른 대상을 상호작용해 일반 몽타주가 처형 위에 얹히는 것을 차단한다.
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Finisher);
}

void UWxAbility_Interact::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

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

	// 대상은 이벤트 페이로드로 온다. 예측 클라는 자기가 실어 보낸 값을, 서버는 엔진이 전송(ServerTryActivateAbilityWithEventData)한 같은 값을 받는다.
	// OptionalObject 가 const 라 실행을 위해 const_cast 한다(WxAbility_Finisher 의 Target 과 동일).
	UWxInteractionComponent* Selected = TriggerEventData
		? const_cast<UWxInteractionComponent*>(Cast<UWxInteractionComponent>(TriggerEventData->OptionalObject.Get()))
		: nullptr;

	// 실행은 권위에서만. 예측 클라 인스턴스는 연출만 재생한다.
	if (HasAuthority(&ActivationInfo))
	{
		ExecuteInteract(Selected, ActorInfo);
	}

	// 몽타주가 있으면 재생+응시하고 몽타주 종료 시 끝낸다. 없으면 즉시 종료.
	if (!PlayInteractMontage(Selected))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UWxAbility_Interact::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	// 엔진 EndAbility는 이 어빌리티의 모든 월드 타이머를 비운다(ClearAllTimersForObject).
	// 감지는 부여 동안 상시여야 하므로 활성화(상호작용)가 끝나도 스캔 타이머를 다시 건다.
	// 어빌리티 제거 경로면 직후 OnRemoveAbility가 타이머를 최종 정리한다.
	StartScanTimer(ActorInfo);
}

void UWxAbility_Interact::StartScanTimer(const FGameplayAbilityActorInfo* ActorInfo)
{
	// 감지는 로컬 어포던스.
	// 소유 클라/리슨 호스트에서만 주기 스캔 타이머를 건다(데디 서버는 LocalPlayer 부재로 미설정).
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
	World->OverlapMultiByChannel(Overlaps, ScanOrigin, FQuat::Identity, ECC_WxInteractable, FCollisionShape::MakeSphere(ScanRadius), QueryParams);

	// 후보 컴포넌트를 모은다.
	// 오버랩 결과는 볼륨 프리미티브이므로 이를 참조하는 상호작용 컴포넌트로 역참조한다.
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

UWxInteractionRegistrySubsystem* UWxAbility_Interact::GetLocalRegistry(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const APlayerController* PlayerController = ActorInfo ? ActorInfo->PlayerController.Get() : nullptr;
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	return LocalPlayer ? LocalPlayer->GetSubsystem<UWxInteractionRegistrySubsystem>() : nullptr;
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

bool UWxAbility_Interact::PlayInteractMontage(UWxInteractionComponent* Target)
{
	// 대상이 자체 어빌리티로 모션을 구동하면(예: 처형) 범용 몽타주를 재생하지 않는다 — 처형 몽타주와의 중복을 막는다.
	if (!InteractMontage || !Target || !Target->GetUseInteractMontage())
	{
		return false;
	}

	// 응시 회전은 로컬 컨트롤 인스턴스에서만 — 오토노머스 프록시가 회전 권위라 서버/시뮬은 복제 회전을 따른다.
	if (IsLocallyControlled())
	{
		if (const AActor* Avatar = GetAvatarActorFromActorInfo())
		{
			const FVector ToTarget = Target->GetInteractionLocation() - Avatar->GetActorLocation();
			if (!ToTarget.IsNearlyZero())
			{
				if (UWxAbilityTask_TurnAround* TurnTask = UWxAbilityTask_TurnAround::CreateTask(this, ToTarget, FacingInterpSpeed))
				{
					TurnTask->ReadyForActivation();
				}
			}
		}
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, InteractMontage, GetMontagePlayRate(), NAME_None, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		return false;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Interact::HandleInteractMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Interact::HandleInteractMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Interact::HandleInteractMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Interact::HandleInteractMontageCancelled);
	MontageTask->ReadyForActivation();
	return true;
}

void UWxAbility_Interact::HandleInteractMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Interact::HandleInteractMontageBlendOut()
{
	// OnCompleted 가 후속 발동하므로 여기서는 종료하지 않음.
}

void UWxAbility_Interact::HandleInteractMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Interact::HandleInteractMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
