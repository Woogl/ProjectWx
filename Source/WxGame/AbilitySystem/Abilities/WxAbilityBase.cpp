// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Abilities/WxAbilityBase.h"
#include "AbilitySystem/Tasks/WxAbilityTask_MontageEvents.h"
#include "AbilitySystem/Effects/WxEffect_Cost.h"
#include "AbilitySystem/WxAbilityTargetData_Direction.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimMontage.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WxGame.h"
#include "WxGameplayTags.h"

const FName UWxAbilityBase::LandingSectionName(TEXT("Grounded"));

UWxAbilityBase::UWxAbilityBase()
{
	InstancingPolicy  = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	CostGameplayEffectClass = UWxEffect_Cost::StaticClass();
}

float UWxAbilityBase::QueryCost(const UAbilitySystemComponent& ASC, FGameplayAttribute& OutCostAttribute) const
{
	OutCostAttribute = FGameplayAttribute();

	const UGameplayEffect* CostGE = GetCostGameplayEffect();
	if (!CostGE)
	{
		return 0.f;
	}

	// 비용 계산은 소스 어빌리티에서 수치를 읽으므로 컨텍스트에 어빌리티를 실어야 한다.
	FGameplayEffectContextHandle CostContext = ASC.MakeEffectContext();
	CostContext.SetAbility(this);

	FGameplayEffectSpec CostSpec(CostGE, CostContext, GetAbilityLevel());
	CostSpec.CalculateModifierMagnitudes();

	for (int32 ModifierIndex = 0; ModifierIndex < CostGE->Modifiers.Num(); ++ModifierIndex)
	{
		const FGameplayAttribute& ModifierAttribute = CostGE->Modifiers[ModifierIndex].Attribute;
		const float Magnitude = CostSpec.GetModifierMagnitude(ModifierIndex);
		if (!ModifierAttribute.IsValid() || FMath::IsNearlyZero(Magnitude))
		{
			continue;
		}

		OutCostAttribute = ModifierAttribute;
		return FMath::Abs(Magnitude);
	}

	return 0.f;
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
	const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE || CooldownGE->GetStackingType() == EGameplayEffectStackingType::None)
	{
		return 1;
	}

	return FMath::Max(1, CooldownGE->StackLimitCount);
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
	return 1.f;
}

void UWxAbilityBase::StartRecovery()
{
	SetActionBlocking(false);
}

void UWxAbilityBase::SetActionBlocking(bool bBlocking)
{
	// 액션이 아닌 어빌리티는 몽타주에 후딜 노티파이가 섞여 있어도 차단을 유지한다.
	if (!IsActive() || !GetAssetTags().HasTag(WxGameplayTags::Ability_Action) || IsBlockingOtherAbilities() == bBlocking)
	{
		return;
	}

	SetShouldBlockOtherAbilities(bBlocking);
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayEventData Payload;
		Payload.EventTag = WxGameplayTags::Event_Ability_BlockingChanged;
		Payload.Instigator = GetAvatarActorFromActorInfo();
		// 관찰자는 입력 버퍼의 재발동·종료까지 끝난 뒤 최종 상태를 평가한다.
		ASC->HandleGameplayEvent(Payload.EventTag, &Payload);
	}
}

bool UWxAbilityBase::DoesAbilitySatisfyTagRequirements(const UAbilitySystemComponent& AbilitySystemComponent, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UWxAbilityBase* Occupant = Cast<UWxAbilityBase>(AbilitySystemComponent.GetAnimatingAbility());
	if (Occupant && !(Occupant->IsActive() && Occupant->IsBlockingOtherAbilities()))
	{
		Occupant = nullptr;
	}
	const bool bCancelEntry = Occupant && Occupant != this && Occupant->GetAssetTags().HasAny(CancelAbilitiesWithTag);
	const bool bIgnoreActivationTags = AbilitySystemComponent.HasMatchingGameplayTag(WxGameplayTags::Effect_IgnoreAbilityActivationTags);
	if (!bCancelEntry && !bIgnoreActivationTags)
	{
		return Super::DoesAbilitySatisfyTagRequirements(AbilitySystemComponent, SourceTags, TargetTags, OptionalRelevantTags);
	}

	bool bBlocked = AbilitySystemComponent.AreAbilityTagsBlocked(GetAssetTags());
	if (bCancelEntry)
	{
		if (const UWxAbilitySystemComponent* WxASC = Cast<UWxAbilitySystemComponent>(&AbilitySystemComponent))
		{
			// 같은 태그를 쓰는 다른 GA까지 풀지 않고, 재생 중인 액션이 등록한 차단 1건만 제외한다.
			bBlocked = WxASC->AreAbilityTagsBlockedIgnoringContribution(GetAssetTags(), Occupant->BlockAbilitiesWithTag);
		}
	}

	// 호출자가 지정한 소유자 조건만 면제하며 소스·대상 조건은 항상 검사한다.
	bool bMissing = false;
	if (!bIgnoreActivationTags)
	{
		bBlocked |= AbilitySystemComponent.HasAnyMatchingGameplayTags(ActivationBlockedTags);
		bMissing |= !AbilitySystemComponent.HasAllMatchingGameplayTags(ActivationRequiredTags);
	}
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

bool UWxAbilityBase::DoesOwnerSatisfyActivationTags(const UAbilitySystemComponent& AbilitySystemComponent) const
{
	return AbilitySystemComponent.HasAllMatchingGameplayTags(ActivationRequiredTags) && !AbilitySystemComponent.HasAnyMatchingGameplayTags(ActivationBlockedTags);
}

void UWxAbilityBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	ResetActionState();

	if (GetAssetTags().HasTag(WxGameplayTags::Ability_Action))
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
	if (MontageEventsTask)
	{
		UWxAbilityTask_MontageEvents* PreviousEvents = MontageEventsTask;
		MontageEventsTask = nullptr;
		PreviousEvents->EndTask();
	}
	ClearPendingDirectionalMontage();
	// 재생 태스크는 엔진 EndAbility가 소유자 종료로 정리해야 루핑 몽타주까지 멈춘다.
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
		StartSection = SelectInputDirectionSection(Montage, Prefix, MontageInputDirection);
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

void UWxAbilityBase::ResetActionState()
{
	ClearPendingDirectionalMontage();
	bHasMontageInputDirection = false;
	MontageInputDirection = FVector::ZeroVector;
	// 활성화는 엔진이 차단을 켠 채 시작하고, 콤보·패턴의 다음 단계는 후딜에서 이어질 수 있다.
	SetActionBlocking(true);
}

bool UWxAbilityBase::PlayMontageInternal(UAnimMontage* Montage, FName StartSection)
{
	if (MontageEventsTask)
	{
		UWxAbilityTask_MontageEvents* PreviousEvents = MontageEventsTask;
		MontageEventsTask = nullptr;
		PreviousEvents->EndTask();
	}
	// EndTask가 구 태스크를 가비지로 표시하므로, 바인딩은 남아도 약참조가 끊겨 후속 이벤트는 발송되지 않는다.
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	UWxAbilityTask_MontageEvents* NewEvents = UWxAbilityTask_MontageEvents::CreateTask(this);
	MontageEventsTask = NewEvents;
	NewEvents->ReadyForActivation();

	UAbilityTask_PlayMontageAndWait* NewMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, GetMontagePlayRate(), StartSection, true, 1.f, 0.f, true);
	MontageTask = NewMontageTask;

	NewMontageTask->OnCompleted.AddDynamic(this, &UWxAbilityBase::HandleMontageCompleted);
	NewMontageTask->OnBlendOut.AddDynamic(this, &UWxAbilityBase::HandleMontageBlendOut);
	NewMontageTask->OnInterrupted.AddDynamic(this, &UWxAbilityBase::HandleMontageInterrupted);
	NewMontageTask->OnCancelled.AddDynamic(this, &UWxAbilityBase::HandleMontageCancelled);
	NewMontageTask->ReadyForActivation();
	if (IsActive() && MontageEventsTask == NewEvents)
	{
		NewEvents->BindToMontage(Montage);
	}
	// 재생이 동기로 실패하면 태스크가 같은 호출 안에서 취소를 방송해 어빌리티가 이미 끝나 있다.
	return IsActive();
}

FName UWxAbilityBase::SelectInputDirectionSection(const UAnimMontage* Montage, const FString& Prefix, const FVector& LocalDirection)
{
	return SelectDirectionalSection(Montage, LocalDirection, Prefix);
}

FVector UWxAbilityBase::GetLocalMontageInputDirection() const
{
	const APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn)
	{
		return FVector::ZeroVector;
	}

	FVector WorldDirection = Pawn->GetLastMovementInputVector();
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

bool UWxAbilityBase::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	// 충전은 쿨다운 GE의 스택이 센다. 소모한 충전 하나가 스택 하나이고, 엔진의 스택 만료 정책이 하나씩 되돌린다.
	const int32 MaxRecharges = GetMaxRecharges();
	const FGameplayTagContainer* CooldownTags = GetCooldownTags();
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (MaxRecharges > 1 && CooldownTags && !CooldownTags->IsEmpty() && ASC
		&& ASC->GetAggregatedStackCount(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(*CooldownTags)) < MaxRecharges)
	{
		return true;
	}

	// 실패 사유 태그를 채워 NotifyAbilityFailed 파이프라인에 전달하는 것까지 순정에 맡긴다.
	return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
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
bool UWxAbilityBase::IsActivationExclusive(const UWxAbilityBase& Other) const
{
	// 요구한 태그를 가지면 그 태그나 부모를 막는 쪽은 발동할 수 없다.
	return ActivationRequiredTags.HasAny(Other.ActivationBlockedTags) || Other.ActivationRequiredTags.HasAny(ActivationBlockedTags);
}
#endif
