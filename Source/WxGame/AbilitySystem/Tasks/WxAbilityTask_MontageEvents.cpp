// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Tasks/WxAbilityTask_MontageEvents.h"
#include "AbilitySystem/Abilities/WxAbilityBase.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/Tasks/WxAbilityTask_Rush.h"
#include "Animation/WxAnimNotifyState_Rush.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Minion/WxMinionComponent.h"
#include "Targeting/WxLockOnComponent.h"
#include "AbilitySystem/Tasks/WxAbilityTask_SlowTime.h"
#include "AbilitySystemComponent.h"
#include "Animation/WxAnimNotify_AreaDamage.h"
#include "Animation/WxAnimNotify_DespawnMinion.h"
#include "Animation/WxAnimNotify_SpawnMinion.h"
#include "Animation/WxAnimNotify_SpawnProjectile.h"
#include "Animation/WxAnimNotify_StartRecovery.h"
#include "Animation/WxAnimNotifyState_ApplyGameplayEffect.h"
#include "Animation/WxAnimNotifyState_ComboWindow.h"
#include "Animation/WxAnimNotifyState_SlowTime.h"
#include "Animation/WxAnimNotifyState_WeaponAttack.h"
#include "Combat/WxCombatLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "Minion/WxMinionSubsystem.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Types/TargetingSystemTypes.h"
#include "Weapons/WxProjectileBase.h"
#include "Weapons/WxWeaponBase.h"

UWxAbilityTask_MontageEvents* UWxAbilityTask_MontageEvents::CreateTask(UGameplayAbility* OwningAbility)
{
	return NewAbilityTask<UWxAbilityTask_MontageEvents>(OwningAbility);
}

void UWxAbilityTask_MontageEvents::Activate()
{
	Super::Activate();
	if (UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(AbilitySystemComponent.Get()))
	{
		MontageNotifyHandle = ASC->OnMontageNotifyState.AddUObject(this, &ThisClass::HandleMontageNotifyState);
		MontageInstantNotifyHandle = ASC->OnMontageNotify.AddUObject(this, &ThisClass::HandleMontageNotify);
	}
	else
	{
		EndTask();
	}
}

void UWxAbilityTask_MontageEvents::BindToMontage(UAnimMontage* Montage)
{
	if (IsFinished() || !Ability)
	{
		return;
	}
	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	const UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr;
	const FAnimMontageInstance* Instance = AnimInstance ? AnimInstance->GetActiveInstanceForMontage(Montage) : nullptr;
	OwnedMontageInstanceID = Instance ? Instance->GetInstanceID() : INDEX_NONE;
}

void UWxAbilityTask_MontageEvents::OnDestroy(bool bInOwnerFinished)
{
	OwnedMontageInstanceID = INDEX_NONE;
	if (UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(AbilitySystemComponent.Get()))
	{
		ASC->OnMontageNotifyState.Remove(MontageNotifyHandle);
		ASC->OnMontageNotify.Remove(MontageInstantNotifyHandle);
	}
	MontageNotifyHandle.Reset();
	MontageInstantNotifyHandle.Reset();
	ClearMontageWindows();
	ClearRushTask();
	Super::OnDestroy(bInOwnerFinished);
}

bool UWxAbilityTask_MontageEvents::OwnsMontageSignal(const FBranchingPointNotifyPayload& Payload) const
{
	const FGameplayAbilityActorInfo* CurrentActorInfo = Ability ? Ability->GetCurrentActorInfo() : nullptr;
	const UAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	return !IsFinished() && Ability && Ability->IsActive() && CurrentActorInfo && ASC && ASC->GetAnimatingAbility() == Ability
		&& Payload.SkelMeshComponent == CurrentActorInfo->SkeletalMeshComponent.Get()
		&& OwnedMontageInstanceID != INDEX_NONE && Payload.MontageInstanceID == OwnedMontageInstanceID;
}

void UWxAbilityTask_MontageEvents::HandleMontageNotify(const UAnimNotify* Notify, const FBranchingPointNotifyPayload& Payload)
{
	if (!OwnsMontageSignal(Payload))
	{
		return;
	}
	if (Notify->IsA<UWxAnimNotify_StartRecovery>())
	{
		if (UWxAbilityBase* FlowAbility = Cast<UWxAbilityBase>(Ability))
		{
			FlowAbility->StartRecovery(Payload.MontageInstanceID);
		}
		return;
	}
	// 생성과 피해는 서버에서만 실행한다. 신호 자체는 RPC가 아니며 각 머신의 몽타주에서 온다.
	AActor* Avatar = Ability->GetAvatarActorFromActorInfo();
	if (!(Ability->GetCurrentActivationInfo().ActivationMode == EGameplayAbilityActivationMode::Authority) || !Avatar)
	{
		return;
	}
	if (const UWxAnimNotify_SpawnProjectile* Projectile = Cast<UWxAnimNotify_SpawnProjectile>(Notify))
	{
		const FTransform Transform(Avatar->GetActorRotation(), Payload.SkelMeshComponent->GetSocketLocation(Projectile->SpawnSocketName));
		AWxProjectileBase::SpawnProjectile(*Avatar, Projectile->ProjectileClass, Transform, Ability->GetAbilityLevel());
	}
	else if (const UWxAnimNotify_SpawnMinion* Spawn = Cast<UWxAnimNotify_SpawnMinion>(Notify))
	{
		APawn* Pawn = Cast<APawn>(Avatar);
		UWxMinionSubsystem* Subsystem = Avatar->GetWorld()->GetSubsystem<UWxMinionSubsystem>();
		if (Pawn && Subsystem)
		{
			Subsystem->SpawnMinion(*Pawn, Spawn->MinionClass, Spawn->LocalSpawnOffset * Pawn->GetActorTransform());
		}
	}
	else if (const UWxAnimNotify_DespawnMinion* Despawn = Cast<UWxAnimNotify_DespawnMinion>(Notify))
	{
		const APawn* Pawn = Cast<APawn>(Avatar);
		UWxMinionSubsystem* Subsystem = Avatar->GetWorld()->GetSubsystem<UWxMinionSubsystem>();
		if (Pawn && Subsystem)
		{
			Subsystem->DespawnMinions(*Pawn, Despawn->MinionClass);
		}
	}
	else if (const UWxAnimNotify_AreaDamage* Area = Cast<UWxAnimNotify_AreaDamage>(Notify))
	{
		UTargetingSubsystem* Subsystem = UTargetingSubsystem::Get(Avatar->GetWorld());
		if (!Area->TargetingPreset || !Subsystem)
		{
			return;
		}
		FTargetingSourceContext Context;
		Context.SourceActor = Avatar;
		Context.InstigatorActor = Avatar;
		FTargetingRequestHandle Request = UTargetingSubsystem::MakeTargetRequestHandle(Area->TargetingPreset, Context);
		Subsystem->ExecuteTargetingRequestWithHandle(Request);
		// 피해 콜백의 다른 쿼리가 결과 저장소를 재할당할 수 있으므로 복사 후 핸들을 먼저 반납한다.
		TArray<FHitResult> Results;
		Subsystem->GetTargetingResults(Request, Results);
		UTargetingSubsystem::ReleaseTargetRequestHandle(Request);
		for (const FHitResult& Result : Results)
		{
			if (AActor* Target = Result.GetActor())
			{
				UWxCombatLibrary::ApplyDamage(Avatar, Target, Area->DamageDataRow, Result);
			}
		}
	}
}

void UWxAbilityTask_MontageEvents::ReleaseMontageWindow(const FWxMontageWindow& Window)
{
	if (Window.Effect.IsValid())
	{
		if (UAbilitySystemComponent* ASC = AbilitySystemComponent.Get())
		{
			ASC->RemoveActiveGameplayEffect(Window.Effect, 1);
		}
	}
	if (AWxWeaponBase* Weapon = Window.Weapon.Get())
	{
		Weapon->EndAttack();
	}
	if (UWxAbilityTask_SlowTime* SlowTime = Window.SlowTime.Get())
	{
		SlowTime->EndTask();
	}
}

void UWxAbilityTask_MontageEvents::ClearMontageWindows()
{
	// 효과 제거 콜백이 어빌리티를 재진입해도 같은 구간을 두 번 정리하지 않는다.
	TArray<FWxMontageWindow> Windows = MoveTemp(MontageWindows);
	MontageWindows.Reset();
	for (const FWxMontageWindow& Window : Windows)
	{
		ReleaseMontageWindow(Window);
	}
}

void UWxAbilityTask_MontageEvents::HandleGameplayWindow(const UAnimNotifyState* Notify, const FBranchingPointNotifyPayload& Payload, bool bBegin)
{
	if (!Payload.NotifyEvent)
	{
		return;
	}
	const int32 Index = MontageWindows.IndexOfByPredicate([Notify, &Payload](const FWxMontageWindow& Window)
	{
		return Window.Notify.Get() == Notify && Window.StartTime == Payload.NotifyEvent->GetTime();
	});
	if (!bBegin)
	{
		if (Index != INDEX_NONE)
		{
			const FWxMontageWindow Window = MontageWindows[Index];
			MontageWindows.RemoveAt(Index);
			ReleaseMontageWindow(Window);
			if (Notify->IsA<UWxAnimNotifyState_ComboWindow>())
			{
				if (UWxAbilityBase* FlowAbility = Cast<UWxAbilityBase>(Ability))
				{
					FlowAbility->CloseComboWindow(Payload.MontageInstanceID);
				}
			}
		}
		return;
	}
	if (Index != INDEX_NONE)
	{
		return;
	}
	const bool bCombo = Notify->IsA<UWxAnimNotifyState_ComboWindow>();
	const UWxAnimNotifyState_ApplyGameplayEffect* Effect = Cast<UWxAnimNotifyState_ApplyGameplayEffect>(Notify);
	const UWxAnimNotifyState_WeaponAttack* Attack = Cast<UWxAnimNotifyState_WeaponAttack>(Notify);
	const UWxAnimNotifyState_SlowTime* Slow = Cast<UWxAnimNotifyState_SlowTime>(Notify);
	if ((!bCombo && !Effect && !Attack && !Slow) || (!bCombo && !(Ability->GetCurrentActivationInfo().ActivationMode == EGameplayAbilityActivationMode::Authority)))
	{
		return;
	}
	FWxMontageWindow Window;
	Window.ID = ++NextMontageWindowID;
	Window.Notify = Notify;
	Window.StartTime = Payload.NotifyEvent->GetTime();
	MontageWindows.Add(Window);
	if (bCombo)
	{
		// 버퍼 입력으로 즉시 어빌리티가 재발동할 수 있으므로 창 등록 이후 호출한다.
		if (UWxAbilityBase* FlowAbility = Cast<UWxAbilityBase>(Ability))
		{
			FlowAbility->OpenComboWindow(Payload.MontageInstanceID);
		}
		return;
	}
	if (Effect)
	{
		const UGameplayEffect* Definition = Effect->EffectClass.GetDefaultObject();
		if (Definition && Definition->DurationPolicy == EGameplayEffectDurationType::Infinite)
		{
			Window.Effect = UWxCombatLibrary::ApplyEffect(AbilitySystemComponent.Get(), Effect->EffectClass, Ability);
		}
	}
	else if (Attack)
	{
		Window.Weapon = AWxWeaponBase::FindWeapon(Ability->GetAvatarActorFromActorInfo());
		if (AWxWeaponBase* Weapon = Window.Weapon.Get())
		{
			Weapon->BeginAttack(Attack->DamageDataRow);
		}
	}
	else if (Slow)
	{
		Window.SlowTime = UWxAbilityTask_SlowTime::CreateTask(Ability, Slow->TimeDilation, -1.f);
		Window.SlowTime->ReadyForActivation();
	}
	FWxMontageWindow* ActiveWindow = MontageWindows.FindByPredicate([&Window](const FWxMontageWindow& Candidate) { return Candidate.ID == Window.ID; });
	if (ActiveWindow)
	{
		*ActiveWindow = Window;
	}
	else
	{
		// 효과 적용·즉시 겹침 판정에서 취소됐으면 뒤늦게 반환된 자원도 회수한다.
		ReleaseMontageWindow(Window);
	}
}

void UWxAbilityTask_MontageEvents::ClearRushTask()
{
	ActiveRushNotify.Reset();
	if (RushTask)
	{
		RushTask->EndTask();
		RushTask = nullptr;
	}
}

void UWxAbilityTask_MontageEvents::HandleMontageNotifyState(const UAnimNotifyState* Notify, const FBranchingPointNotifyPayload& Payload, bool bBegin)
{
	if (!OwnsMontageSignal(Payload))
	{
		return;
	}
	const UWxAnimNotifyState_Rush* Rush = Cast<UWxAnimNotifyState_Rush>(Notify);
	if (!Rush)
	{
		HandleGameplayWindow(Notify, Payload, bBegin);
		return;
	}
	if (!Payload.NotifyEvent)
	{
		return;
	}
	// 엔진은 종료 신호에 FAnimNotifyEvent 복사본을 전달하므로 포인터 주소로 구간을 비교하지 않는다.
	const bool bSameWindow = ActiveRushNotify.Get() == Notify && ActiveRushStartTime == Payload.NotifyEvent->GetTime();
	if (!bBegin)
	{
		// 이전 구간의 늦은 종료는 새 돌진을 끊지 않는다. 정상 끝은 소스 만료에 맡겨 마지막 이동분을 보존한다.
		if (bSameWindow)
		{
			ActiveRushNotify.Reset();
			if (!Payload.bReachedEnd)
			{
				ClearRushTask();
			}
		}
		return;
	}
	if ((!(Ability->GetCurrentActivationInfo().ActivationMode == EGameplayAbilityActivationMode::Authority) && !Ability->IsLocallyControlled()) || bSameWindow)
	{
		return;
	}
	APawn* Avatar = Cast<APawn>(Ability->GetAvatarActorFromActorInfo());
	UAnimInstance* AnimInstance = Ability->GetCurrentActorInfo()->GetAnimInstance();
	const FAnimMontageInstance* Instance = AnimInstance ? AnimInstance->GetMontageInstanceForID(OwnedMontageInstanceID) : nullptr;
	if (!Avatar || !Instance || Instance->GetPlayRate() <= 0.f)
	{
		return;
	}
	AActor* Target = nullptr;
	switch (Rush->TargetSource)
	{
	case EWxRushTarget::LockOnTarget:
		Target = UWxLockOnComponent::ResolveLockOnTargetActor(Avatar);
		break;
	case EWxRushTarget::Master:
		Target = UWxMinionComponent::GetMaster(*Avatar);
		break;
	case EWxRushTarget::Minion:
		if (const UWxMinionSubsystem* Subsystem = Avatar->GetWorld()->GetSubsystem<UWxMinionSubsystem>())
		{
			Target = Subsystem->FindActiveMinion(*Avatar);
		}
		break;
	}
	ClearRushTask();
	if (Target)
	{
		const float Duration = Payload.NotifyEvent->GetDuration() / Instance->GetPlayRate();
		RushTask = UWxAbilityTask_Rush::CreateTask(Ability, *Target, Duration, Rush->StopDistance, Rush->IgnoreCollisions);
		if (RushTask)
		{
			ActiveRushNotify = Notify;
			ActiveRushStartTime = Payload.NotifyEvent->GetTime();
			RushTask->ReadyForActivation();
		}
	}
}
