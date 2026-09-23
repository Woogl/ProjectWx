// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_LockOn.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayTag.h"
#include "AbilitySystem/Task/WxAbilityTask_LockOnCamera.h"
#include "AbilitySystem/Task/WxAbilityTask_RotateToTarget.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "Targeting/WxLockOnComponent.h"
#include "Targeting/WxLockOnPointComponent.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Types/TargetingSystemTypes.h"
#include "WxGameplayTags.h"

UWxAbility_LockOn::UWxAbility_LockOn()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(WxGameplayTags::Ability_LockOn);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(WxGameplayTags::Ability_LockOn);
}

void UWxAbility_LockOn::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);
	
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UWxAbility_LockOn::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!TargetingPreset || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// autonomous proxy의 회전 정합을 위해 서버에서도 꺼야 하므로 IsLocallyControlled 게이트 앞에서 처리한다.
	const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr)
	{
		Movement->bOrientRotationToMovement = false;
	}

	// 타겟 결정과 추적 태스크는 소유 클라(또는 리슨 서버 호스트)에서만 처리한다 — 태스크는 카메라·몸체 추적과 재탐색 입력 폴링의 로컬 어포던스다.
	// 서버는 소유 클라의 SetLockOnTarget RPC로 복제된 값만 보유하고, 발사체·스냅 등 소비처가 그 값을 읽는다.
	if (!IsLocallyControlled())
	{
		return;
	}

	TArray<AActor*> Candidates;
	GatherCandidates(Candidates);

	// 후보가 거리순이라 첫 지점이 가장 가깝다.
	USceneComponent* TargetComponent = nullptr;
	for (AActor* Candidate : Candidates)
	{
		TargetComponent = UWxLockOnPointComponent::ResolveLockOnTarget(Candidate);
		if (TargetComponent)
		{
			break;
		}
	}

	const AActor* Avatar = GetAvatarActorFromActorInfo();
	UWxLockOnComponent* LockOnComp = Avatar ? Avatar->FindComponentByClass<UWxLockOnComponent>() : nullptr;
	if (!TargetComponent || !LockOnComp)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 대상은 컴포넌트 하나가 들고 두 태스크는 매 틱 그 값을 읽는다 — 이후 교체·해제는 컴포넌트에만 쓴다.
	LockOnComponent = LockOnComp;
	LockOnComp->SetLockOnTarget(TargetComponent);

	ListenForDodgeRotation();
	if (!IsDodgeActive())
	{
		StartRotateToTargetTask();
	}

	UWxAbilityTask_LockOnCamera* LockOnTask = UWxAbilityTask_LockOnCamera::CreateTask(this, CameraInterpSpeed, CameraPitchOffset, MaxDistance, RetargetLookThreshold);
	LockOnTask->OnTargetLost.AddDynamic(this, &UWxAbility_LockOn::HandleTargetLost);
	LockOnTask->OnRetargetRequested.AddDynamic(this, &UWxAbility_LockOn::HandleRetargetRequested);
	LockOnTask->ReadyForActivation();
}

void UWxAbility_LockOn::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 대상을 비워 락온을 해제한다. Reticle과 Nameplate는 NameplateManager가 이 값을 매 틱 읽어 정리한다.
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (UWxLockOnComponent* LockOnComp = LockOnComponent.Get())
		{
			LockOnComp->SetLockOnTarget(nullptr);
		}
		LockOnComponent = nullptr;
		StopRotateToTargetTask();

		// 평상시 회전 모드는 폰마다 다를 수 있으므로 무브먼트 아키타입(폰 BP·C++ 생성자 기본값)에서 읽어 되돌린다.
		const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
		UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
		if (const UCharacterMovementComponent* MovementDefaults = Movement ? Cast<UCharacterMovementComponent>(Movement->GetArchetype()) : nullptr)
		{
			Movement->bOrientRotationToMovement = MovementDefaults->bOrientRotationToMovement;
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UWxAbility_LockOn::HandleDodgeTagAdded()
{
	StopRotateToTargetTask();
}

void UWxAbility_LockOn::HandleDodgeTagRemoved()
{
	StartRotateToTargetTask();
}

void UWxAbility_LockOn::ListenForDodgeRotation()
{
	UAbilityTask_WaitGameplayTagAdded* AddedTask = UAbilityTask_WaitGameplayTagAdded::WaitGameplayTagAdd(this, WxGameplayTags::Ability_Dodge, nullptr, false);
	AddedTask->Added.AddDynamic(this, &UWxAbility_LockOn::HandleDodgeTagAdded);
	AddedTask->ReadyForActivation();

	UAbilityTask_WaitGameplayTagRemoved* RemovedTask = UAbilityTask_WaitGameplayTagRemoved::WaitGameplayTagRemove(this, WxGameplayTags::Ability_Dodge, nullptr, false);
	RemovedTask->Removed.AddDynamic(this, &UWxAbility_LockOn::HandleDodgeTagRemoved);
	RemovedTask->ReadyForActivation();
}

void UWxAbility_LockOn::StartRotateToTargetTask()
{
	if (RotateToTargetTask)
	{
		return;
	}

	RotateToTargetTask = UWxAbilityTask_RotateToTarget::CreateTask(this, CharacterInterpSpeed);
	RotateToTargetTask->ReadyForActivation();
}

void UWxAbility_LockOn::StopRotateToTargetTask()
{
	if (RotateToTargetTask)
	{
		RotateToTargetTask->EndTask();
		RotateToTargetTask = nullptr;
	}
}

bool UWxAbility_LockOn::IsDodgeActive() const
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	return ASC && ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Dodge);
}

void UWxAbility_LockOn::HandleTargetLost()
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	UWxLockOnComponent* LockOnComp = LockOnComponent.Get();
	if (bRetargetOnTargetLost && IsLocallyControlled() && Avatar && LockOnComp)
	{
		// 락온 대상은 컴포넌트지만 후보 비교/제외는 액터 단위이므로 소유 액터로 환원한다.
		const USceneComponent* LostComponent = LockOnComp->GetLockOnTarget();
		const AActor* LostTarget = LostComponent ? LostComponent->GetOwner() : nullptr;
		const FVector AvatarLocation = Avatar->GetActorLocation();
		const float MaxDistanceSquared = MaxDistance * MaxDistance;

		TArray<AActor*> Candidates;
		GatherCandidates(Candidates);
		for (AActor* Candidate : Candidates)
		{
			if (!Candidate || Candidate == LostTarget)
			{
				continue;
			}

			// 락온 유지 범위(MaxDistance)를 벗어난 후보로 갈아타면 다음 틱에 즉시 다시 잃으므로 제외한다.
			if (FVector::DistSquared(AvatarLocation, Candidate->GetActorLocation()) > MaxDistanceSquared)
			{
				continue;
			}

			// ResolveLockOnTarget이 죽은 대상 등 불가 지점을 이미 거른다.
			USceneComponent* TargetComponent = UWxLockOnPointComponent::ResolveLockOnTarget(Candidate);
			if (!TargetComponent)
			{
				continue;
			}

			// 컴포넌트에만 설정하면 태스크가 다음 틱에 새 대상을 읽어 추적을 잇는다(권위 반영은 서버 RPC).
			LockOnComp->SetLockOnTarget(TargetComponent);
			return;
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_LockOn::HandleRetargetRequested(FVector2D ScreenDirection)
{
	APlayerController* PC = CurrentActorInfo ? Cast<APlayerController>(CurrentActorInfo->PlayerController.Get()) : nullptr;
	UWxLockOnComponent* LockOnComp = LockOnComponent.Get();
	if (!PC || !LockOnComp)
	{
		return;
	}

	const USceneComponent* CurrentComponent = LockOnComp->GetLockOnTarget();

	int32 ViewportX = 0;
	int32 ViewportY = 0;
	PC->GetViewportSize(ViewportX, ViewportY);
	FVector2D OriginScreen(ViewportX * 0.5f, ViewportY * 0.5f);
	if (CurrentComponent)
	{
		FVector2D CurrentScreen;
		if (PC->ProjectWorldLocationToScreen(CurrentComponent->GetComponentLocation(), CurrentScreen))
		{
			OriginScreen = CurrentScreen;
		}
	}

	// 현재 타겟 액터도 후보에 남긴다 — 같은 적의 다른 부위로도 전환할 수 있게 비교 단위를 지점으로 둔다.
	TArray<AActor*> Candidates;
	GatherCandidates(Candidates);

	USceneComponent* BestTargetComponent = nullptr;
	float BestAlignment = RetargetMinAlignment;
	TArray<USceneComponent*> CandidatePoints;
	for (AActor* Candidate : Candidates)
	{
		if (!Candidate)
		{
			continue;
		}

		// 지점이 없는 후보는 빈 배열이라 자연히 건너뛴다.
		UWxLockOnPointComponent::GatherLockOnPoints(Candidate, CandidatePoints);
		for (USceneComponent* CandidateComponent : CandidatePoints)
		{
			if (!CandidateComponent || CandidateComponent == CurrentComponent)
			{
				continue;
			}

			// 실제 보이는 위치로 비교한다 — 카메라 뒤의 후보는 화면 좌표가 없어 제외된다.
			FVector2D CandidateScreen;
			if (!PC->ProjectWorldLocationToScreen(CandidateComponent->GetComponentLocation(), CandidateScreen))
			{
				continue;
			}

			// 화면 좌표는 Y가 아래로 증가하므로 입력 공간(+Y=위)에 맞춰 Y를 뒤집어 정렬도를 비교한다.
			const FVector2D ToCandidate = FVector2D(CandidateScreen.X - OriginScreen.X, OriginScreen.Y - CandidateScreen.Y).GetSafeNormal();
			if (ToCandidate.IsNearlyZero())
			{
				continue;
			}

			const float Alignment = FVector2D::DotProduct(ToCandidate, ScreenDirection);
			if (Alignment > BestAlignment)
			{
				BestAlignment = Alignment;
				BestTargetComponent = CandidateComponent;
			}
		}
	}

	if (BestTargetComponent)
	{
		LockOnComp->SetLockOnTarget(BestTargetComponent);
	}
}

void UWxAbility_LockOn::GatherCandidates(TArray<AActor*>& OutCandidates) const
{
	OutCandidates.Reset();

	UTargetingSubsystem* TargetingSubsystem = UTargetingSubsystem::Get(GetWorld());
	if (!TargetingPreset || !TargetingSubsystem)
	{
		return;
	}

	FTargetingSourceContext SourceContext;
	SourceContext.SourceActor = GetOwningActorFromActorInfo();
	FTargetingRequestHandle RequestHandle = UTargetingSubsystem::MakeTargetRequestHandle(TargetingPreset, SourceContext);
	TargetingSubsystem->ExecuteTargetingRequestWithHandle(RequestHandle);
	TargetingSubsystem->GetTargetingResultsActors(RequestHandle, OutCandidates);
	UTargetingSubsystem::ReleaseTargetRequestHandle(RequestHandle);
}
