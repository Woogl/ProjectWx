// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbilityBase.h"
#include "AbilitySystem/Effect/WxEffect_Cooldown.h"
#include "AbilitySystem/Effect/WxEffect_Cost.h"
#include "AbilitySystem/TargetData/WxAbilityTargetData_Direction.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystem/WxInputBufferComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/DataValidation.h"
#include "WxCombatModule.h"
#include "WxGameplayTags.h"

const FName UWxAbilityBase::LandingSectionName(TEXT("Grounded"));

UWxAbilityBase::UWxAbilityBase()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 쿨다운 GE도 공용이다 — 쌓지 않고 어빌리티의 CooldownTags로 구분하므로 어빌리티끼리 섞이지 않는다.
	CostGameplayEffectClass = UWxEffect_Cost::StaticClass();
	CooldownGameplayEffectClass = UWxEffect_Cooldown::StaticClass();
}

FGameplayTagContainer UWxAbilityBase::GetAbilityBlockTags() const
{
	FGameplayTagContainer BlockTags = BlockAbilitiesWithTag;
	if (ActivationGroup == EWxAbilityActivationGroup::Independent)
	{
		return BlockTags;
	}

	// 강공격이 약공격을 취소하려면 순정 발동 검사를 먼저 통과해야 한다.
	// 명시한 BlockAbilitiesWithTag는 유지하고, 공통 규칙에서만 Heavy를 제외한다.
	if (ActivationGroup == EWxAbilityActivationGroup::Exclusive && GetAssetTags().HasTag(WxGameplayTags::Ability_Attack_Light))
	{
		BlockTags.AddTag(WxGameplayTags::Ability_Attack_Light);
		BlockTags.AddTag(WxGameplayTags::Ability_Attack_Air);
		BlockTags.AddTag(WxGameplayTags::Ability_Attack_DodgeCounter);
	}
	else
	{
		BlockTags.AddTag(WxGameplayTags::Ability_Attack);
	}
	BlockTags.AddTag(WxGameplayTags::Ability_Skill);
	BlockTags.AddTag(WxGameplayTags::Ability_Pattern);
	BlockTags.AddTag(WxGameplayTags::Ability_Ultimate);
	BlockTags.AddTag(WxGameplayTags::Ability_Dodge);
	BlockTags.AddTag(WxGameplayTags::Ability_Guard);
	BlockTags.AddTag(WxGameplayTags::Ability_UseItem);
	BlockTags.AddTag(WxGameplayTags::Ability_Interact);
	BlockTags.AddTag(WxGameplayTags::Ability_Jump);
	return BlockTags;
}

FText UWxAbilityBase::GetTitle() const
{
	return Title;
}

FText UWxAbilityBase::GetDescription() const
{
	return Description;
}

TSoftObjectPtr<UObject> UWxAbilityBase::GetIcon() const
{
	return Icon;
}

int32 UWxAbilityBase::GetMaxRecharges() const
{
	return FMath::Max(1, MaxRecharges);
}

float UWxAbilityBase::GetCooldownTime() const
{
	return FMath::Max(0.f, CooldownTime);
}

UAnimMontage* UWxAbilityBase::GetMontage() const
{
	return AbilityMontage;
}

EWxAbilityDirection UWxAbilityBase::ResolveDirection(const FVector& LocalDirection, EWxAbilityDirection DefaultDirection)
{
	const FVector Local = LocalDirection.GetSafeNormal2D();
	if (Local.IsNearlyZero())
	{
		return DefaultDirection;
	}

	const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X));
	const int32 Octant = ((FMath::RoundToInt(AngleDeg / 45.f) % 8) + 8) % 8;
	return static_cast<EWxAbilityDirection>(Octant);
}

FName UWxAbilityBase::SelectDirectionalSection(const FVector& LocalDirection, const FString& Prefix, EWxAbilityDirection DefaultDirection) const
{
	return SelectDirectionalSection(GetMontage(), LocalDirection, Prefix, DefaultDirection);
}

FName UWxAbilityBase::SelectDirectionalSection(const UAnimMontage* Montage, const FVector& LocalDirection, const FString& Prefix, EWxAbilityDirection DefaultDirection)
{
	if (!Montage)
	{
		return NAME_None;
	}

	const EWxAbilityDirection Direction = ResolveDirection(LocalDirection, DefaultDirection);
	const FName SectionName(Prefix + StaticEnum<EWxAbilityDirection>()->GetNameStringByValue(static_cast<int64>(Direction)));
	if (Montage->IsValidSectionName(SectionName))
	{
		return SectionName;
	}

	const FName ForwardSection(Prefix + StaticEnum<EWxAbilityDirection>()->GetNameStringByValue(static_cast<int64>(EWxAbilityDirection::Forward)));
	if (Montage->IsValidSectionName(ForwardSection))
	{
		return ForwardSection;
	}

	return NAME_None;
}

bool UWxAbilityBase::HasMontageSection(const UAnimMontage* Montage, FName SectionName)
{
	const FString Prefix = SectionName.IsNone() ? FString() : SectionName.ToString();
	return Montage && (Montage->IsValidSectionName(SectionName) || Montage->IsValidSectionName(FName(Prefix + TEXT("Forward"))));
}

float UWxAbilityBase::GetMontagePlayRate() const
{
	const UWxAbilitySystemComponent* ASC = Cast<UWxAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	return ASC ? ASC->GetMontagePlayRate() : 1.f;
}

EWxAbilityActionPhase UWxAbilityBase::GetActionPhase() const
{
	return ActionPhase;
}

void UWxAbilityBase::SetActionPhase(EWxAbilityActionPhase NewPhase)
{
	if (ActionPhase == NewPhase)
	{
		return;
	}

	ActionPhase = NewPhase;
	if (IsActive() && ActivationGroup == EWxAbilityActivationGroup::Exclusive)
	{
		SetShouldBlockOtherAbilities(NewPhase != EWxAbilityActionPhase::Recovery);
	}
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayEventData Payload;
		Payload.EventTag = WxGameplayTags::Event_Ability_ActionPhaseChanged;
		Payload.Instigator = GetAvatarActorFromActorInfo();
		// 관찰자는 입력 버퍼의 재발동·종료까지 끝난 뒤 최종 상태를 평가한다.
		ASC->HandleGameplayEvent(Payload.EventTag, &Payload);
	}
}

void UWxAbilityBase::OpenComboWindow(int32 MontageInstanceID)
{
	// 공용 몽타주의 노티파이가 Independent·Override에 콤보 재발동 예외를 열지 않게 한다.
	if (ActivationGroup == EWxAbilityActivationGroup::Exclusive && ActionPhase == EWxAbilityActionPhase::Blocking && IsPlayingMontageInstance(MontageInstanceID))
	{
		SetActionPhase(EWxAbilityActionPhase::ComboWindow);

		// 입력 버퍼 처리로 같은 인스턴스가 재발동할 수 있으므로 이후 상태를 덮어쓰지 않는다.
		const AActor* Avatar = GetAvatarActorFromActorInfo();
		if (UWxInputBufferComponent* InputBuffer = Avatar ? Avatar->FindComponentByClass<UWxInputBufferComponent>() : nullptr)
		{
			InputBuffer->FlushBufferedInputs();
		}
	}
}

void UWxAbilityBase::CloseComboWindow(int32 MontageInstanceID)
{
	if (!IsPlayingMontageInstance(MontageInstanceID))
	{
		return;
	}

	// 콤보 창이 후딜보다 늦게 닫혀도 이미 시작한 Recovery를 되돌리지 않는다.
	if (ActionPhase == EWxAbilityActionPhase::ComboWindow)
	{
		SetActionPhase(EWxAbilityActionPhase::Blocking);
	}

	OnComboWindowClosed();
}

void UWxAbilityBase::OnComboWindowClosed()
{
}

void UWxAbilityBase::StartRecovery(int32 MontageInstanceID)
{
	// 공용 몽타주의 노티파이가 Independent·Override의 단계까지 바꾸지 않게 한다.
	if (ActivationGroup == EWxAbilityActivationGroup::Exclusive && ActionPhase != EWxAbilityActionPhase::Recovery && IsPlayingMontageInstance(MontageInstanceID))
	{
		SetActionPhase(EWxAbilityActionPhase::Recovery);

		// 입력 버퍼에서 발동한 어빌리티가 이 인스턴스를 끝낼 수 있으므로 이후 상태를 덮어쓰지 않는다.
		const AActor* Avatar = GetAvatarActorFromActorInfo();
		if (UWxInputBufferComponent* InputBuffer = Avatar ? Avatar->FindComponentByClass<UWxInputBufferComponent>() : nullptr)
		{
			InputBuffer->FlushBufferedInputs();
		}
	}
}

bool UWxAbilityBase::IsPlayingMontageInstance(int32 MontageInstanceID) const
{
	// 단계마다 같은 몽타주를 새로 트는 경우도 있어 에셋이 아니라 인스턴스로 가른다.
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const UAnimInstance* AnimInstance = CurrentActorInfo ? CurrentActorInfo->GetAnimInstance() : nullptr;
	const FAnimMontageInstance* MontageInstance = ASC && AnimInstance ? AnimInstance->GetActiveInstanceForMontage(ASC->GetCurrentMontage()) : nullptr;
	return MontageInstance && MontageInstance->GetInstanceID() == MontageInstanceID;
}

bool UWxAbilityBase::DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& AbilitySystemComponent, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const bool bComboRetrigger = ActivationGroup == EWxAbilityActivationGroup::Exclusive && IsActive() && ActionPhase == EWxAbilityActionPhase::ComboWindow;
	const bool bIgnoreActivationTags = AbilitySystemComponent.HasMatchingGameplayTag(WxGameplayTags::Effect_IgnoreAbilityActivationTags);
	if (!bComboRetrigger && !bIgnoreActivationTags)
	{
		return Super::DoesAbilitySatisfyTagRequirements(AbilitySystemComponent, SourceTags, TargetTags, OptionalRelevantTags);
	}

	bool bBlocked = AbilitySystemComponent.AreAbilityTagsBlocked(GetAssetTags());
	if (bComboRetrigger && IsBlockingOtherAbilities())
	{
		if (const UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(&AbilitySystemComponent))
		{
			// 같은 태그를 쓰는 다른 GA까지 풀지 않고, 이 실행이 등록한 차단 1건만 제외한다.
			bBlocked = WxASC->AreAbilityTagsBlockedIgnoringContribution(GetAssetTags(), GetAbilityBlockTags());
		}
	}

	// 콤보는 이미 성립한 액션의 다음 단이므로 소유자 발동 조건을 다시 요구하지 않는다.
	bool bMissing = false;
	if (SourceTags)
	{
		bBlocked |= SourceTags->HasAny(SourceBlockedTags);
		bMissing |= !SourceTags->HasAll(SourceRequiredTags);
	}
	if (TargetTags)
	{
		bBlocked |= TargetTags->HasAny(TargetBlockedTags);
		bMissing |= !TargetTags->HasAll(TargetRequiredTags);
	}
	if (OptionalRelevantTags)
	{
		if (bBlocked)
		{
			OptionalRelevantTags->AddTag(UAbilitySystemGlobals::Get().ActivateFailTagsBlockedTag);
		}
		if (bMissing)
		{
			OptionalRelevantTags->AddTag(UAbilitySystemGlobals::Get().ActivateFailTagsMissingTag);
		}
	}
	return !bBlocked && !bMissing;
}

bool UWxAbilityBase::CanBeCanceled() const
{
	return ActivationGroup != EWxAbilityActivationGroup::Override && Super::CanBeCanceled();
}

void UWxAbilityBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	bHasMontageInputDirection = false;
	MontageInputDirection = FVector::ZeroVector;
	SetActionPhase(EWxAbilityActionPhase::Blocking);

	if (ActivationGroup != EWxAbilityActivationGroup::Independent)
	{
		if (UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr))
		{
			// 본동작의 차단·취소는 순정 GAS가 처리하고, Recovery는 전용 태그가 없으므로 단계로 찾아 취소한다.
			WxASC->CancelRecoveringAbilities(this);
		}
	}

	// 구체 어빌리티가 Super를 먼저 부르므로, 커밋 실패로 곧장 종료하는 경우엔 EndAbility가 같은 프레임에 다시 걷는다.
	for (const TSubclassOf<UGameplayEffect>& EffectClass : ActivationOwnedEffects)
	{
		if (EffectClass)
		{
			// 권위도 예측 키도 없는 머신에는 적용 자체가 일어나지 않는다 — 빈 핸들은 걷을 것도 없어 담지 않는다.
			FActiveGameplayEffectHandle EffectHandle = ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, EffectClass.GetDefaultObject(), GetAbilityLevel());
			if (EffectHandle.WasSuccessfullyApplied())
			{
				ActivationOwnedEffectHandles.Add(EffectHandle);
			}
		}
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UWxAbilityBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ClearPendingDirectionalMontage();
	// 태스크를 여기서 끝내면 안 된다 — 엔진 EndAbility가 소유자 종료로 끝내는 경로만 재생 중인 몽타주를 멈추므로, 미리 끊으면 루핑 가드 몽타주처럼 스스로 끝나지 않는 것이 종료 후에도 계속 돈다.
	MontageTask = nullptr;

	// 캔슬·중단도 이 경로를 지나므로 효과가 새지 않는다. 활성 중에 이미 걷힌 것은 조회에 걸리지 않아 무해하다.
	// 제거는 권위만 한다 — 클라의 예측본은 예측 키 확인이, 서버본은 복제가 걷는다.
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->IsOwnerActorAuthoritative())
	{
		// 스택형 GE는 다른 소유자의 적용과 한 핸들로 합쳐지므로 이 활성화의 몫인 스택 하나만 뺀다.
		for (FActiveGameplayEffectHandle EffectHandle : ActivationOwnedEffectHandles)
		{
			ASC->RemoveActiveGameplayEffect(EffectHandle, 1);
		}
	}
	ActivationOwnedEffectHandles.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UWxAbilityBase::PlayMontage(UAnimMontage* Montage, FName StartSection)
{
	if (!Montage)
	{
		return false;
	}

	// 이미 고른 방향·Backstep은 그대로 쓴다. Forward는 자동 선택의 진입점이자 누락 방향의 대체 섹션이다.
	const FString Prefix = StartSection.IsNone() ? FString() : StartSection.ToString();
	const bool bSelectDirection = (StartSection.IsNone() || !Montage->IsValidSectionName(StartSection))
		&& Montage->IsValidSectionName(FName(Prefix + TEXT("Forward")));
	if (bSelectDirection)
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		const bool bSharesClientDirection = NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalPredicted
			|| NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerInitiated;
		if (!bHasMontageInputDirection && ASC && bSharesClientDirection && HasAuthority(&CurrentActivationInfo)
			&& CurrentActorInfo && CurrentActorInfo->PlayerController.IsValid() && !IsLocallyControlled())
		{
			PendingDirectionalMontage = Montage;
			PendingDirectionalSection = StartSection;
			if (!MontageDirectionHandle.IsValid())
			{
				MontageDirectionHandle = ASC->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey())
					.AddUObject(this, &UWxAbilityBase::HandleMontageDirectionReceived);
			}
			ASC->CallReplicatedTargetDataDelegatesIfSet(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
			return IsActive();
		}

		if (!bHasMontageInputDirection)
		{
			MontageInputDirection = GetLocalMontageInputDirection();
			bHasMontageInputDirection = true;
			if (ASC && bSharesClientDirection && IsLocallyControlled() && !HasAuthority(&CurrentActivationInfo))
			{
				FGameplayAbilityTargetDataHandle DataHandle;
				FWxAbilityTargetData_Direction* DirectionData = new FWxAbilityTargetData_Direction();
				DirectionData->Direction = MontageInputDirection;
				DataHandle.Add(DirectionData);
				ASC->CallServerSetReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey(), DataHandle, FGameplayTag(), ASC->ScopedPredictionKey);
			}
		}
		StartSection = SelectDirectionalSection(Montage, MontageInputDirection, Prefix);
	}

	// 대기 중 다른 섹션으로 교체됐으면 늦게 온 방향 데이터로 앞 요청을 재생하지 않는다.
	ClearPendingDirectionalMontage();
	if (!StartSection.IsNone() && !Montage->IsValidSectionName(StartSection))
	{
		UE_LOG(LogWxCombat, Warning, TEXT("%s: 몽타주 %s에 섹션 %s가 없다."), *GetName(), *Montage->GetName(), *StartSection.ToString());
		return false;
	}

	return PlayMontageInternal(Montage, StartSection);
}

bool UWxAbilityBase::PlayMontageInternal(UAnimMontage* Montage, FName StartSection)
{
	// EndTask가 구 태스크를 가비지로 표시하므로, 바인딩은 남아도 약참조가 끊겨 후속 이벤트는 발송되지 않는다.
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	UAbilityTask_PlayMontageAndWait* NewMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, GetMontagePlayRate(), StartSection, true, 1.f, 0.f, true);
	MontageTask = NewMontageTask;

	NewMontageTask->OnCompleted.AddDynamic(this, &UWxAbilityBase::HandleMontageCompleted);
	NewMontageTask->OnBlendOut.AddDynamic(this, &UWxAbilityBase::HandleMontageBlendOut);
	NewMontageTask->OnInterrupted.AddDynamic(this, &UWxAbilityBase::HandleMontageInterrupted);
	NewMontageTask->OnCancelled.AddDynamic(this, &UWxAbilityBase::HandleMontageCancelled);
	NewMontageTask->ReadyForActivation();
	return true;
}

FVector UWxAbilityBase::GetLocalMontageInputDirection() const
{
	const APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn)
	{
		return FVector::ZeroVector;
	}

	FVector WorldDirection = Pawn->GetLastMovementInputVector();
	// 직접 제어하지 않는 캐릭터는 마지막 입력 벡터 대신 이동 컴포넌트가 받은 가속도를 사용한다.
	if (!Pawn->IsLocallyControlled())
	{
		if (const ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			WorldDirection = Character->GetCharacterMovement()->GetCurrentAcceleration();
		}
	}
	return Pawn->GetActorTransform().InverseTransformVectorNoScale(WorldDirection).GetSafeNormal2D();
}

void UWxAbilityBase::HandleMontageDirectionReceived(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ApplicationTag)
{
	MontageInputDirection = FVector::ZeroVector;
	const FGameplayAbilityTargetData* Data = DataHandle.Get(0);
	if (Data && Data->GetScriptStruct() == FWxAbilityTargetData_Direction::StaticStruct())
	{
		const FVector Direction = static_cast<const FWxAbilityTargetData_Direction*>(Data)->Direction;
		if (!Direction.ContainsNaN())
		{
			MontageInputDirection = Direction.GetSafeNormal2D();
		}
	}
	bHasMontageInputDirection = true;

	UAnimMontage* Montage = PendingDirectionalMontage;
	const FName StartSection = PendingDirectionalSection;
	ClearPendingDirectionalMontage();
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());
	}
	if (IsActive() && Montage && !PlayMontage(Montage, StartSection))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UWxAbilityBase::ClearPendingDirectionalMontage()
{
	if (MontageDirectionHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->AbilityTargetDataSetDelegate(CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey()).Remove(MontageDirectionHandle);
		}
		MontageDirectionHandle.Reset();
	}
	PendingDirectionalMontage = nullptr;
	PendingDirectionalSection = NAME_None;
}

void UWxAbilityBase::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbilityBase::HandleMontageBlendOut()
{
}

void UWxAbilityBase::HandleMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWxAbilityBase::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

UGameplayEffect* UWxAbilityBase::GetCooldownGameplayEffect() const
{
	UGameplayEffect* CooldownGE = Super::GetCooldownGameplayEffect();

	// 지속시간이 0인 GE는 만료 타이머가 걸리지 않는다.
	if (CooldownGE && CooldownGE->IsA<UWxEffect_Cooldown>() && GetCooldownTime() <= 0.f)
	{
		return nullptr;
	}

	return CooldownGE;
}

bool UWxAbilityBase::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 쿨다운 태그가 없으면 쿨다운도 없다. 순정 판정은 이때 공용 쿨다운 GE가 지정돼 있다고 경고만 낸다.
	if (CooldownTags.IsEmpty())
	{
		return true;
	}

	// 순정 판정은 쿨다운 태그가 붙어 있기만 하면 막는다.
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC)
	{
		const FGameplayEffectQuery CooldownQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTags);
		if (ASC->GetAggregatedStackCount(CooldownQuery) < GetMaxRecharges())
		{
			return true;
		}
	}

	// 실패 사유 태그를 채워 NotifyAbilityFailed 파이프라인에 전달하는 것까지 순정에 맡긴다.
	return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
}

const FGameplayTagContainer* UWxAbilityBase::GetCooldownTags() const
{
	return &CooldownTags;
}

void UWxAbilityBase::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE)
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CooldownGE->GetClass(), GetAbilityLevel(Handle, ActorInfo));
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 같은 쿨다운 태그의 쿨다운이 남아 있으면 그 끝부터 회복을 시작해 충전이 차례로 돌아온다.
	// 엔진은 GE를 활성 목록에 넣은 뒤 지속시간을 다시 계산하므로, 목록을 보는 계산은 적용 전에 끝내 값으로 넘긴다.
	float QueuedTime = 0.f;
	if (const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		for (const float TimeRemaining : ASC->GetActiveEffectsTimeRemaining(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTags)))
		{
			QueuedTime = FMath::Max(QueuedTime, TimeRemaining);
		}
	}

	SpecHandle.Data->SetSetByCallerMagnitude(WxGameplayTags::SetByCaller_Duration, QueuedTime + GetCooldownTime());
	SpecHandle.Data->DynamicGrantedTags.AppendTags(CooldownTags);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
}

bool UWxAbilityBase::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::Effect_IgnoreCosts))
	{
		return true;
	}

	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

void UWxAbilityBase::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::Effect_IgnoreCosts))
	{
		return;
	}

	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
}

#if WITH_EDITOR
EDataValidationResult UWxAbilityBase::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Result = Super::IsDataValid(Context);
	const uint32 NumErrors = Context.GetNumErrors();

	// 쿨다운 태그가 없으면 순정 판정이 통과시켜 쿨다운이 조용히 사라진다.
	if (CooldownTime > 0.f && CooldownTags.IsEmpty())
	{
		Context.AddError(INVTEXT("쿨다운 시간이 있는데 쿨다운 태그가 없어 쿨다운이 걸리지 않는다."));
	}

	return CombineDataValidationResults(Result, Context.GetNumErrors() > NumErrors ? EDataValidationResult::Invalid : EDataValidationResult::Valid);
}

bool UWxAbilityBase::IsActivationExclusive(const UWxAbilityBase& Other) const
{
	// 요구한 태그를 가지면 그 태그나 부모를 막는 쪽은 발동할 수 없다.
	return ActivationRequiredTags.HasAny(Other.ActivationBlockedTags) || Other.ActivationRequiredTags.HasAny(ActivationBlockedTags);
}
#endif
