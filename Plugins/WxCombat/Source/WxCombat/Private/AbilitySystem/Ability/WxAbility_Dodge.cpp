// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Dodge.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "AbilitySystem/Task/WxAbilityTask_WaitInputActionPressed.h"
#include "AbilitySystem/Effect/WxEffect_RecoverResource.h"
#include "AbilitySystem/TargetData/WxAbilityTargetData_Direction.h"
#include "AbilitySystem/Task/WxAbilityTask_SlowTime.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "WxGameplayTags.h"

UWxAbility_Dodge::UWxAbility_Dodge()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Dodge);
	SetAssetTags(AssetTags);
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	CancelAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability);
}

bool UWxAbility_Dodge::IsObservedInput(const UInputAction* Action) const
{
	return Super::IsObservedInput(Action) || (Action && Action == CounterInputAction);
}

void UWxAbility_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!DodgeMontage || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (IsLocallyControlled())
	{
		// 로컬 클라이언트(또는 리슨 서버 호스트): 입력 방향을 캐릭터 로컬 공간으로 변환해 8방향 섹션 선택.
		// 로컬 공간(정면 기준)으로 변환해 두면 서버는 자신의 facing과 무관하게 동일한 방향을 계산한다(몽타주 정합성 보장).
		FVector LocalDodgeDirection = FVector::ZeroVector;
		if (const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
		{
			const FVector WorldInput = Character->GetLastMovementInputVector();
			LocalDodgeDirection = Character->GetActorTransform().InverseTransformVectorNoScale(WorldInput);
		}

		// 리모트 클라이언트인 경우 서버에 방향 전송 (서버가 동일 방향 섹션을 선택)
		if (ASC && !HasAuthority(&ActivationInfo))
		{
			FGameplayAbilityTargetDataHandle DataHandle;
			FWxAbilityTargetData_Direction* DirectionData = new FWxAbilityTargetData_Direction();
			DirectionData->Direction = LocalDodgeDirection;
			DataHandle.Add(DirectionData);

			ASC->CallServerSetReplicatedTargetData(
				Handle,
				ActivationInfo.GetActivationPredictionKey(),
				DataHandle,
				FGameplayTag(),
				ASC->ScopedPredictionKey);
		}

		if (!StartDodge(LocalDodgeDirection))
		{
			return;
		}
	}
	else if (HasAuthority(&ActivationInfo))
	{
		// 서버(리모트 플레이어 처리): 클라이언트로부터 방향 데이터 수신 후 몽타주 재생(HandleTargetDataReceived)
		if (ASC)
		{
			FAbilityTargetDataSetDelegate& Delegate = ASC->AbilityTargetDataSetDelegate(
				Handle,
				ActivationInfo.GetActivationPredictionKey());
			Delegate.AddUObject(this, &UWxAbility_Dodge::HandleTargetDataReceived);

			ASC->CallReplicatedTargetDataDelegatesIfSet(
				Handle,
				ActivationInfo.GetActivationPredictionKey());
		}
	}

	// 극한 회피 여부와 무관하게, 회피 중 ANS_ComboWindow 구간 공격 입력으로 반격 전환. 반격 입력 라우팅은 HandlesInput override가 선언적으로 처리한다.
	if (DodgeCounterMontage)
	{
		ListenForCounterInput();
	}

	ListenForInvincibleWindow();
	ListenForDodgeSuccess();
}

void UWxAbility_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 어빌리티가 무적 구간 도중 취소되면 태그 해제 콜백을 받지 못하므로 여기서 비활성화한다.
	DeactivateJudgementCapsule();

	// 무적 구간 도중 취소되어 ANS_Invincible의 NotifyEnd가 스킵되면 State.Invincible이 잔존해 영구 무적이 된다.
	// 판정 캡슐과 동일하게 실패복구로 태그를 정리한다(State.Invincible은 ANS_Invincible만 부여하므로 이 시점 잔존분은 회피가 흘린 것).
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		if (ASC->HasMatchingGameplayTag(WxGameplayTags::State_Invincible))
		{
			ASC->RemoveLooseGameplayTag(WxGameplayTags::State_Invincible);
		}
	}

	if (WaitInputTask)
	{
		WaitInputTask->EndTask();
		WaitInputTask = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ────────────────────────────────────────────────────────────────────────────
//  방향 처리
// ────────────────────────────────────────────────────────────────────────────

EWxDodgeDirection UWxAbility_Dodge::ResolveDodgeDirection(const FVector& LocalDirection) const
{
	// 입력은 캐릭터 로컬 공간(정면 +X, 오른쪽 +Y). facing을 참조하지 않으므로 클라이언트/서버가 동일 결과를 낸다.
	// 이동 입력이 없으면 후방 회피(백스텝)로 처리한다.
	const FVector Local = LocalDirection.GetSafeNormal2D();
	if (Local.IsNearlyZero())
	{
		return EWxDodgeDirection::Back;
	}

	// 정면 기준 부호 있는 각도(+Y=오른쪽=시계 방향 +)를 45° 단위로 양자화해 8분면 인덱스(0=Forward)로 매핑.
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X));
	const int32 Octant = ((FMath::RoundToInt(AngleDeg / 45.f) % 8) + 8) % 8;
	return static_cast<EWxDodgeDirection>(Octant);
}

FName UWxAbility_Dodge::SelectDodgeSection(const FVector& LocalDirection) const
{
	if (!DodgeMontage)
	{
		return NAME_None;
	}

	// 섹션 이름은 EWxDodgeDirection 항목명과 동일한 규약을 사용한다.
	const EWxDodgeDirection DodgeDirection = ResolveDodgeDirection(LocalDirection);
	const FName SectionName(StaticEnum<EWxDodgeDirection>()->GetNameStringByValue(static_cast<int64>(DodgeDirection)));
	if (DodgeMontage->IsValidSectionName(SectionName))
	{
		return SectionName;
	}

	// 구성되지 않은 방향 섹션은 전방 회피로 폴백
	const FName ForwardSection(StaticEnum<EWxDodgeDirection>()->GetNameStringByValue(static_cast<int64>(EWxDodgeDirection::Forward)));
	if (DodgeMontage->IsValidSectionName(ForwardSection))
	{
		return ForwardSection;
	}

	return NAME_None;
}

bool UWxAbility_Dodge::StartDodge(const FVector& LocalDirection)
{
	const FVector Local = LocalDirection.GetSafeNormal2D();

	// 이동 입력이 없으면 전용 백스텝 몽타주를 재생한다. 미설정 시 아래에서 DodgeMontage의 Back 섹션으로 폴백한다.
	if (Local.IsNearlyZero() && BackstepMontage)
	{
		if (!PlayMontage(BackstepMontage))
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
			return false;
		}

		return true;
	}

	const FName SectionName = SelectDodgeSection(LocalDirection);

	// 락온 중에는 락온 태스크가 회피 내내 몸을 타겟으로 추적해 호 궤적을 만들므로 잔차 보정을 하지 않는다.
	// 비락온은 섹션 루트모션이 몸 기준 고정 방향이라, 양자화 잔차(±22.5°, 폴백 시 그 이상)만큼 시작 시 몸을 돌려 이동을 입력 방향과 일치시킨다.
	// 클라이언트/서버가 동일한 LocalDirection과 락온 태그로 같은 분기를 타므로 결과 facing도 일치한다.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const bool bLockedOn = ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::State_LockOn);
	if (!Local.IsNearlyZero() && !bLockedOn)
	{
		// NAME_None(섹션 없는 몽타주)은 전방 이동 몽타주로 간주한다.
		float SectionAngleDeg = 0.f;
		if (!SectionName.IsNone())
		{
			SectionAngleDeg = StaticEnum<EWxDodgeDirection>()->GetValueByName(SectionName) * 45.f;
		}

		const float InputAngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X));
		const float ResidualDeg = FRotator::NormalizeAxis(InputAngleDeg - SectionAngleDeg);

		if (AActor* Avatar = GetAvatarActorFromActorInfo())
		{
			Avatar->AddActorWorldRotation(FRotator(0.f, ResidualDeg, 0.f));
		}
	}

	if (!PlayMontage(DodgeMontage, SectionName))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return false;
	}

	return true;
}

// ────────────────────────────────────────────────────────────────────────────
//  극한 회피 / 반격 처리
// ────────────────────────────────────────────────────────────────────────────

void UWxAbility_Dodge::ListenForDodgeSuccess()
{
	// 무적 구간에 다수의 공격이 들어오면 데미지 파이프라인이 매 피격마다 Event.DodgeSuccess를 발송한다.
	// 회피 1회당 극한 회피 보상은 한 번만 적용되어야 하므로 OnlyTriggerOnce로 바인딩한다.
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, WxGameplayTags::Event_DodgeSuccess, nullptr, true);
	if (EventTask)
	{
		EventTask->EventReceived.AddDynamic(this, &UWxAbility_Dodge::HandleDodgeSuccess);
		EventTask->ReadyForActivation();
	}
}

bool UWxAbility_Dodge::PlayMontage(UAnimMontage* Montage, FName StartSection)
{
	// EndTask가 AnimInstance 바인딩을 해제하므로 구 태스크의 후속 이벤트는 발송되지 않는다.
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	if (!Montage)
	{
		return false;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, 1.f, StartSection, true, 1.f, 0.f, true);
	if (!MontageTask)
	{
		return false;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UWxAbility_Dodge::HandleMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UWxAbility_Dodge::HandleMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UWxAbility_Dodge::HandleMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UWxAbility_Dodge::HandleMontageCancelled);
	MontageTask->ReadyForActivation();
	return true;
}

void UWxAbility_Dodge::ListenForCounterInput()
{
	WaitInputTask = UWxAbilityTask_WaitInputActionPressed::CreateTask(this, CounterInputAction);
	if (WaitInputTask)
	{
		WaitInputTask->OnPressed.AddDynamic(this, &UWxAbility_Dodge::HandleCounterInputPressed);
		WaitInputTask->ReadyForActivation();
	}
}

void UWxAbility_Dodge::HandleDodgeSuccess(FGameplayEventData Payload)
{
	if (!PerfectDodgeMontage)
	{
		return;
	}

	// 극한 회피 성공 보상: MP 회복
	UWxEffect_RecoverResource::ApplyTo(GetAbilitySystemComponentFromActorInfo(), 0.f, PerfectDodgeMPRecovery);

	// 극한 회피 성공 시 짧은 슬로우 타임 연출
	if (UWxAbilityTask_SlowTime* SlowTimeTask = UWxAbilityTask_SlowTime::CreateTask(this, PerfectDodgeSlowTimeDilation, PerfectDodgeSlowTimeDuration))
	{
		SlowTimeTask->ReadyForActivation();
	}

	if (!PlayMontage(PerfectDodgeMontage))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UWxAbility_Dodge::HandleCounterInputPressed()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// ANS_ComboWindow 구간 내에서만 반격 허용
	if (!ASC->HasMatchingGameplayTag(WxGameplayTags::ANS_ComboWindow))
	{
		ListenForCounterInput();
		return;
	}

	if (WaitInputTask)
	{
		WaitInputTask->EndTask();
		WaitInputTask = nullptr;
	}

	if (!PlayMontage(DodgeCounterMontage))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

// ────────────────────────────────────────────────────────────────────────────
//  극한 회피 판정 캡슐
// ────────────────────────────────────────────────────────────────────────────

void UWxAbility_Dodge::ListenForInvincibleWindow()
{
	// 무적 태그는 ANS_Invincible이 발행한다. 여기서는 관찰만 해 판정 캡슐의 수명을 태그에 일치시킨다.
	// 두 태스크 모두 재무장하므로, PerfectDodgeMontage에 무적 구간이 또 있어도 그대로 처리된다.
	UAbilityTask_WaitGameplayTagAdded* AddedTask = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(this, WxGameplayTags::State_Invincible, nullptr, false);
	if (AddedTask)
	{
		AddedTask->Added.AddDynamic(this, &UWxAbility_Dodge::HandleInvincibleTagAdded);
		AddedTask->ReadyForActivation();
	}

	UAbilityTask_WaitGameplayTagRemoved* RemovedTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(this, WxGameplayTags::State_Invincible, nullptr, false);
	if (RemovedTask)
	{
		RemovedTask->Removed.AddDynamic(this, &UWxAbility_Dodge::HandleInvincibleTagRemoved);
		RemovedTask->ReadyForActivation();
	}
}

void UWxAbility_Dodge::ActivateJudgementCapsule()
{
	// 판정 캡슐은 첫 무적 구간에서 한 번만 만들고 이후 회피에서 재사용한다.
	// 평상시엔 콜리전을 끈 채 몸통에 붙어 함께 움직이므로, 여기서 떼어내기만 하면 무적이 시작된 자리에 남는다.
	if (!JudgementCapsule)
	{
		ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
		if (!Character)
		{
			return;
		}

		UCapsuleComponent* BodyCapsule = Character->GetCapsuleComponent();
		if (!BodyCapsule)
		{
			return;
		}

		JudgementCapsule = NewObject<UCapsuleComponent>(Character, TEXT("DodgeJudgementCapsule"));
		JudgementCapsule->SetCapsuleSize(BodyCapsule->GetScaledCapsuleRadius(), BodyCapsule->GetScaledCapsuleHalfHeight());

		// 공격은 Pawn 오브젝트 타입 오버랩으로 대상을 찾는다. 오브젝트 타입 쿼리는 채널 응답을 보지 않으므로,
		// 모든 채널을 무시해도 공격에는 잡히면서 이동·시야·카메라 트레이스에는 전혀 걸리지 않는다.
		JudgementCapsule->SetCollisionObjectType(ECC_Pawn);
		JudgementCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
		JudgementCapsule->SetGenerateOverlapEvents(false);
		JudgementCapsule->SetupAttachment(BodyCapsule);
		JudgementCapsule->RegisterComponent();
	}

	// 콜리전을 켜고 아바타에서 떼어내 무적이 시작된 자리에 남긴다.
	JudgementCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	JudgementCapsule->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
}

void UWxAbility_Dodge::DeactivateJudgementCapsule()
{
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!JudgementCapsule || !Character)
	{
		return;
	}

	JudgementCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	JudgementCapsule->AttachToComponent(Character->GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

void UWxAbility_Dodge::HandleTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag)
{
	// 클라이언트가 캐릭터 로컬 공간으로 변환해 보낸 방향. 서버는 그대로 사용해 동일한 8방향 섹션을 선택한다.
	FVector LocalDirection = FVector::ZeroVector;
	if (const FWxAbilityTargetData_Direction* DirectionData = static_cast<const FWxAbilityTargetData_Direction*>(DataHandle.Get(0)))
	{
		LocalDirection = DirectionData->Direction;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	}

	StartDodge(LocalDirection);
}

void UWxAbility_Dodge::HandleInvincibleTagAdded()
{
	ActivateJudgementCapsule();
}

void UWxAbility_Dodge::HandleInvincibleTagRemoved()
{
	DeactivateJudgementCapsule();
}

// ────────────────────────────────────────────────────────────────────────────
//  몽타주 콜백
// ────────────────────────────────────────────────────────────────────────────

void UWxAbility_Dodge::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_Dodge::HandleMontageBlendOut()
{
	// OnCompleted가 후속 발동하므로 여기서는 처리하지 않음
}

void UWxAbility_Dodge::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbility_Dodge::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
