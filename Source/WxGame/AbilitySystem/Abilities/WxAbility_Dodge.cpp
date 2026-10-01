// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/WxAbility_Dodge.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Abilities/Tasks/AbilityTask_NetworkSyncPoint.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "AbilitySystem/Effects/WxEffect_Damage.h"
#include "AbilitySystem/Tasks/WxAbilityTask_LockMovementRotation.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "WxCollisionChannels.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "WxGameplayTags.h"

const FName UWxAbility_Dodge::BackstepSectionName(TEXT("Backstep"));
const FString UWxAbility_Dodge::SuccessSectionPrefix(TEXT("Success"));

UWxAbility_Dodge::UWxAbility_Dodge()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_Action_Dodge);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_Action_Dodge);

	BlockAbilitiesWithTag.AddTag(WxGameplayTags::Ability_Action);
}

void UWxAbility_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	bDodgeSuccessHandled = false;
	DodgeDirection = FVector::ZeroVector;
	bBackstep = false;

	if (!GetMontage() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UWxAbilityTask_LockMovementRotation::CreateTask(this)->ReadyForActivation();

	// 원격 플레이어의 서버 인스턴스는 방향 데이터를 받은 뒤에야 실제로 재생한다.
	if (!PlayMontage(GetMontage()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ListenForInvincibleWindow();
	ListenForDodgeSuccess();
}

void UWxAbility_Dodge::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 어빌리티가 무적 구간 도중 취소되면 태그 해제 콜백을 받지 못하므로 여기서 비활성화한다.
	// 무적 태그 자체는 구간을 소유한 ANS가 걷어낸다.
	DeactivateJudgementCapsule();

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->OnImmunityBlockGameplayEffectDelegate.Remove(ImmunityBlockHandle);
	}
	ImmunityBlockHandle.Reset();
	MovementTask = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FName UWxAbility_Dodge::SelectInputDirectionSection(const UAnimMontage* Montage, const FString& Prefix, const FVector& LocalDirection)
{
	const FVector Local = LocalDirection.GetSafeNormal2D();
	bBackstep = Local.IsNearlyZero();
	if (const AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		const FVector MovementDirection = bBackstep ? -FVector::ForwardVector : Local;
		DodgeDirection = Avatar->GetActorTransform().TransformVectorNoScale(MovementDirection).GetSafeNormal2D();
	}
	if (Local.IsNearlyZero() && Montage->IsValidSectionName(BackstepSectionName))
	{
		return BackstepSectionName;
	}

	const FName SectionName = SelectDirectionalSection(Montage, LocalDirection, Prefix, EWxAbilityDirection::Back);

	// 락온 중에는 락온이 Ability.Action.Dodge를 보고 회전 태스크를 멈춰 회피 내내 몸 방향을 고정하므로, 회피도 몸을 돌리지 않는다.
	// 비락온은 선택된 포즈와 실제 이동 방향이 맞도록 양자화 잔차만큼 몸을 돌린다. 이동 방향 자체는 위에서 확정했다.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const bool bLockedOn = ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_LockOn);
	if (!Local.IsNearlyZero() && !bLockedOn)
	{
		// 방향 섹션을 찾지 못해 NAME_None이면 전방 이동 몽타주로 간주한다.
		float SectionAngleDeg = 0.f;
		if (!SectionName.IsNone())
		{
			SectionAngleDeg = StaticEnum<EWxAbilityDirection>()->GetValueByName(SectionName) * 45.f;
		}

		const float InputAngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X));
		const float ResidualDeg = FRotator::NormalizeAxis(InputAngleDeg - SectionAngleDeg);

		if (AActor* Avatar = GetAvatarActorFromActorInfo())
		{
			Avatar->AddActorWorldRotation(FRotator(0.f, ResidualDeg, 0.f));
		}
	}

	return SectionName;
}

bool UWxAbility_Dodge::PlayMontageInternal(UAnimMontage* Montage, FName StartSection)
{
	if (!Super::PlayMontageInternal(Montage, StartSection))
	{
		return false;
	}

	// 극한 회피는 새 몽타주 구간과 함께 이동을 다시 시작하므로 앞 소스가 겹치지 않게 끝낸다.
	if (MovementTask)
	{
		MovementTask->EndTask();
		MovementTask = nullptr;
	}
	const float Distance = bDodgeSuccessHandled ? SuccessDistance : (bBackstep ? BackstepDistance : DodgeDistance);
	const float Duration = bDodgeSuccessHandled ? SuccessDuration : (bBackstep ? BackstepDuration : DodgeDuration);
	const ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (Movement && !DodgeDirection.IsNearlyZero() && Distance > 0.f && Duration > 0.f)
	{
		MovementTask = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
			this, TEXT("WxDodge"), DodgeDirection, Distance / Duration, Duration, false, nullptr,
			ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, Movement->GetMaxSpeed(), true);
		MovementTask->ReadyForActivation();
	}
	return true;
}

void UWxAbility_Dodge::ListenForDodgeSuccess()
{
	// 원격 회피의 방향 데이터가 먼저 와 있으면 시작 실패로 이미 끝났을 수 있다. 끝난 인스턴스가 구독을 남기면 다른 무적의 차단에도 반응한다.
	if (!IsActive())
	{
		return;
	}

	// 피해 적용은 서버에서만 일어나므로 이 통지도 서버에서만 온다.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ImmunityBlockHandle = ASC->OnImmunityBlockGameplayEffectDelegate.AddUObject(this, &UWxAbility_Dodge::HandleImmunityBlock);
	}

	if (IsPredictingClient())
	{
		// 타인의 공격 예측 키 없이도, 내 회피 활성화에 대한 서버 확정 신호를 받는다.
		UAbilityTask_NetworkSyncPoint* ConfirmationTask = UAbilityTask_NetworkSyncPoint::WaitNetSync(this, EAbilityTaskNetSyncType::OnlyClientWait);
		ConfirmationTask->OnSync.AddDynamic(this, &UWxAbility_Dodge::HandleDodgeSuccess);
		ConfirmationTask->ReadyForActivation();
	}
}

void UWxAbility_Dodge::HandleImmunityBlock(const FGameplayEffectSpec& BlockedSpec, const FActiveGameplayEffect* ImmunityEffect)
{
	// 다른 Immunity(쿨다운 면제 등)의 차단 통지도 같은 델리게이트로 온다.
	if (BlockedSpec.Def && BlockedSpec.Def->IsA<UWxEffect_Damage>())
	{
		HandleDodgeSuccess();
	}
}

void UWxAbility_Dodge::HandleDodgeSuccess()
{
	if (bDodgeSuccessHandled)
	{
		return;
	}

	// 벽에 막히거나 이동이 먼저 끝나 속도가 0이어도 처음 확정한 방향으로 이어간다.
	FName SectionName = NAME_None;
	if (const AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		const FVector LocalDirection = Avatar->GetActorTransform().InverseTransformVectorNoScale(DodgeDirection);
		SectionName = SelectDirectionalSection(LocalDirection, SuccessSectionPrefix, EWxAbilityDirection::Back);
	}

	if (SectionName.IsNone())
	{
		return;
	}
	bDodgeSuccessHandled = true;

	if (HasAuthority(&CurrentActivationInfo) && !IsLocallyControlled())
	{
		// 서버는 기다리지 않는다. 이 회피의 SpecHandle·활성화 키로 소유 클라이언트에만 통지한다.
		UAbilityTask_NetworkSyncPoint* ConfirmationTask = UAbilityTask_NetworkSyncPoint::WaitNetSync(this, EAbilityTaskNetSyncType::OnlyClientWait);
		ConfirmationTask->ReadyForActivation();
	}

	if (!PlayMontage(GetMontage(), SectionName))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UWxAbility_Dodge::ListenForInvincibleWindow()
{
	// 무적 태그는 WxAnimNotifyState_ApplyGameplayEffect가 발행하고, 여기서는 관찰만 해 판정 캡슐의 수명을 태그에 맞춘다.
	// 두 태스크 모두 재무장하므로 극한 회피 섹션에 무적 구간이 또 있어도 그대로 처리된다.
	UAbilityTask_WaitGameplayTagAdded* AddedTask = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(this, WxGameplayTags::Effect_Invincible, nullptr, false);
	AddedTask->Added.AddDynamic(this, &UWxAbility_Dodge::HandleInvincibleTagAdded);
	AddedTask->ReadyForActivation();

	UAbilityTask_WaitGameplayTagRemoved* RemovedTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(this, WxGameplayTags::Effect_Invincible, nullptr, false);
	RemovedTask->Removed.AddDynamic(this, &UWxAbility_Dodge::HandleInvincibleTagRemoved);
	RemovedTask->ReadyForActivation();
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

		// 캐릭터 메시와 같은 방식으로 피격에 참여한다 — 무기·투사체가 Pawn을 Overlap으로 열어 두었고, 채널 판정은 양방향 최솟값이라 WxAttack 응답도 함께 열어야 한다.
		JudgementCapsule->SetCollisionObjectType(ECC_Pawn);
		JudgementCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
		JudgementCapsule->SetCollisionResponseToChannel(ECC_WxAttack, ECR_Overlap);

		// 무기의 틱 Sweep은 터널링 보완용이라 위치가 거의 안 변하는 스윙을 놓친다. 주 경로인 오버랩과 투사체가 닿으려면 이벤트가 필요하다.
		JudgementCapsule->SetGenerateOverlapEvents(true);
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

void UWxAbility_Dodge::HandleInvincibleTagAdded()
{
	ActivateJudgementCapsule();
}

void UWxAbility_Dodge::HandleInvincibleTagRemoved()
{
	DeactivateJudgementCapsule();
}
