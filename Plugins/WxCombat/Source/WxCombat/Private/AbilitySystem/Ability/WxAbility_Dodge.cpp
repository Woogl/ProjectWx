// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_Dodge.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "AbilitySystem/TargetData/WxAbilityTargetData_Direction.h"
#include "AbilitySystem/Task/WxAbilityTask_SlowTime.h"
#include "AbilitySystemComponent.h"
#include "WxCollisionChannels.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "WxGameplayTags.h"

UWxAbility_Dodge::UWxAbility_Dodge()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Dodge);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Dodge);

	ActivationGroup = EWxAbilityActivationGroup::Exclusive_Blocking;
}

float UWxAbility_Dodge::GetMontagePlayRate() const
{
	return 1.f;
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
		FVector LocalDodgeDirection = FVector::ZeroVector;
		if (const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
		{
			const FVector WorldInput = Character->GetLastMovementInputVector();
			LocalDodgeDirection = Character->GetActorTransform().InverseTransformVectorNoScale(WorldInput);
		}

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
		// 리모트 플레이어의 서버 인스턴스는 방향 데이터를 받은 뒤에야 몽타주를 재생한다.
		if (ASC)
		{
			// 해제는 엔진 EndAbility의 ClearAbilityReplicatedDataCache가 맵 엔트리째 걷는다.
			FAbilityTargetDataSetDelegate& Delegate = ASC->AbilityTargetDataSetDelegate(
				Handle,
				ActivationInfo.GetActivationPredictionKey());
			Delegate.AddUObject(this, &UWxAbility_Dodge::HandleTargetDataReceived);

			ASC->CallReplicatedTargetDataDelegatesIfSet(
				Handle,
				ActivationInfo.GetActivationPredictionKey());
		}
	}

	ListenForInvincibleWindow();
	ListenForDodgeSuccess();
}

void UWxAbility_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 어빌리티가 무적 구간 도중 취소되면 태그 해제 콜백을 받지 못하므로 여기서 비활성화한다.
	// 무적 태그 자체는 ANS가 건 지속시간 GE라 회피가 끊겨도 스스로 만료된다.
	DeactivateJudgementCapsule();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

EWxDodgeDirection UWxAbility_Dodge::ResolveDodgeDirection(const FVector& LocalDirection) const
{
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

FName UWxAbility_Dodge::SelectDodgeSection(const UAnimMontage* Montage, const FVector& LocalDirection) const
{
	if (!Montage)
	{
		return NAME_None;
	}

	const EWxDodgeDirection DodgeDirection = ResolveDodgeDirection(LocalDirection);
	const FName SectionName(StaticEnum<EWxDodgeDirection>()->GetNameStringByValue(static_cast<int64>(DodgeDirection)));
	if (Montage->IsValidSectionName(SectionName))
	{
		return SectionName;
	}

	const FName ForwardSection(StaticEnum<EWxDodgeDirection>()->GetNameStringByValue(static_cast<int64>(EWxDodgeDirection::Forward)));
	if (Montage->IsValidSectionName(ForwardSection))
	{
		return ForwardSection;
	}

	return NAME_None;
}

bool UWxAbility_Dodge::StartDodge(const FVector& LocalDirection)
{
	const FVector Local = LocalDirection.GetSafeNormal2D();

	if (Local.IsNearlyZero() && BackstepMontage)
	{
		if (!PlayMontage(BackstepMontage))
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
			return false;
		}

		return true;
	}

	const FName SectionName = SelectDodgeSection(DodgeMontage, LocalDirection);

	// 락온 중에는 락온 태스크가 회피 내내 몸을 타겟으로 추적해 호 궤적을 만들므로 잔차 보정을 하지 않는다.
	// 비락온은 섹션 루트모션이 몸 기준 고정 방향이라, 양자화 잔차(±22.5°, 폴백 시 그 이상)만큼 몸을 돌려 이동을 입력 방향에 맞춘다.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const bool bLockedOn = ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_LockOn);
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

void UWxAbility_Dodge::ListenForDodgeSuccess()
{
	// 무적 구간에 여러 공격이 들어오면 데미지 파이프라인이 매 피격마다 Event.DodgeSuccess를 발송한다.
	// 보상은 회피 1회당 한 번이어야 하므로 OnlyTriggerOnce로 바인딩한다.
	UAbilityTask_WaitGameplayEvent* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, WxGameplayTags::Event_DodgeSuccess, nullptr, true);
	if (EventTask)
	{
		EventTask->EventReceived.AddDynamic(this, &UWxAbility_Dodge::HandleDodgeSuccess);
		EventTask->ReadyForActivation();
	}
}

void UWxAbility_Dodge::HandleDodgeSuccess(FGameplayEventData Payload)
{
	if (!PerfectDodgeMontage)
	{
		return;
	}
	
	if (UWxAbilityTask_SlowTime* SlowTimeTask = UWxAbilityTask_SlowTime::CreateTask(this, PerfectDodgeSlowTimeDilation, PerfectDodgeSlowTimeDuration))
	{
		SlowTimeTask->ReadyForActivation();
	}

	// 회피 섹션은 몸을 돌리지 않고 몸 기준 루트모션으로만 흐르므로, 극한 회피도 같은 방향 섹션으로 이어야 이동이 꺾이지 않는다.
	// 루트모션 중 속도가 곧 진행 방향이라, 8방향 양자화·잔차 보정·백스텝이 이 값 하나로 수렴한다.
	FName SectionName = NAME_None;
	if (const AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		const FVector LocalDirection = Avatar->GetActorTransform().InverseTransformVectorNoScale(Avatar->GetVelocity());
		SectionName = SelectDodgeSection(PerfectDodgeMontage, LocalDirection);
	}

	if (!PlayMontage(PerfectDodgeMontage, SectionName))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UWxAbility_Dodge::ListenForInvincibleWindow()
{
	// 무적 태그는 ANS_Invincible이 발행하고, 여기서는 관찰만 해 판정 캡슐의 수명을 태그에 맞춘다.
	// 두 태스크 모두 재무장하므로 PerfectDodgeMontage에 무적 구간이 또 있어도 그대로 처리된다.
	UAbilityTask_WaitGameplayTagAdded* AddedTask = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(this, WxGameplayTags::Effect_Invincible, nullptr, false);
	if (AddedTask)
	{
		AddedTask->Added.AddDynamic(this, &UWxAbility_Dodge::HandleInvincibleTagAdded);
		AddedTask->ReadyForActivation();
	}

	UAbilityTask_WaitGameplayTagRemoved* RemovedTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(this, WxGameplayTags::Effect_Invincible, nullptr, false);
	if (RemovedTask)
	{
		RemovedTask->Removed.AddDynamic(this, &UWxAbility_Dodge::HandleInvincibleTagRemoved);
		RemovedTask->ReadyForActivation();
	}
}

void UWxAbility_Dodge::ActivateJudgementCapsule()
{
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

		JudgementCapsule->SetCollisionObjectType(ECC_WxAttack);
		JudgementCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
		JudgementCapsule->SetGenerateOverlapEvents(false);
		JudgementCapsule->SetupAttachment(BodyCapsule);
		JudgementCapsule->RegisterComponent();
	}

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
	// CallServerSetReplicatedTargetData는 등록된 어떤 파생 타입도 실어 보낼 수 있으므로, 구조체를 확인하고 캐스트한다.
	FVector LocalDirection = FVector::ZeroVector;
	const FGameplayAbilityTargetData* ReceivedData = DataHandle.Get(0);
	if (ReceivedData && ReceivedData->GetScriptStruct() == FWxAbilityTargetData_Direction::StaticStruct())
	{
		LocalDirection = static_cast<const FWxAbilityTargetData_Direction*>(ReceivedData)->Direction;
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
