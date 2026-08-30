// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_LockOn.h"
#include "AbilitySystem/Task/WxAbilityTask_LockOnTarget.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "Targeting/WxLockOnManagerComponent.h"
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
		SavedOrientRotationToMovement = Movement->bOrientRotationToMovement;
		Movement->bOrientRotationToMovement = false;
	}

	// 타겟 결정과 추적 태스크는 소유 클라(또는 리슨 서버 호스트)에서만 처리한다 — 태스크는 카메라·몸체 추적, 재탐색 입력 폴링, 레티클의 로컬 어포던스다.
	// 서버는 소유 클라의 SetLockOnTarget RPC로 복제된 값만 보유하고, 발사체·스냅 등 소비처가 그 값을 읽는다.
	if (!IsLocallyControlled())
	{
		return;
	}

	TArray<AActor*> Candidates;
	GatherCandidates(Candidates);

	// 락온 지점(UWxLockOnPointComponent)이 있는 액터만 대상이 되며, 후보가 거리순이라 첫 지점이 가장 가깝다.
	USceneComponent* TargetComponent = nullptr;
	for (AActor* Candidate : Candidates)
	{
		TargetComponent = UWxLockOnPointComponent::ResolveLockOnTarget(Candidate);
		if (TargetComponent)
		{
			break;
		}
	}
	if (!TargetComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 소유 클라는 로컬 즉시 반영 + 서버 RPC, 리슨 호스트는 권위 직접 반영(복제는 컴포넌트 몫).
	if (UWxLockOnManagerComponent* LockOnComp = UWxLockOnManagerComponent::FindComponent(GetOwningActorFromActorInfo()))
	{
		LockOnComp->SetLockOnTarget(TargetComponent);
	}

	LockOnTask = UWxAbilityTask_LockOnTarget::CreateTask(this, TargetComponent, CameraInterpSpeed, CameraPitchOffset, MaxDistance, CharacterInterpSpeed, ReticleWidgetClass, RetargetLookThreshold);
	LockOnTask->OnTargetLost.AddDynamic(this, &UWxAbility_LockOn::HandleTargetLost);
	LockOnTask->OnRetargetRequested.AddDynamic(this, &UWxAbility_LockOn::HandleRetargetRequested);
	LockOnTask->ReadyForActivation();
}

void UWxAbility_LockOn::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Super::EndAbility 가 태스크를 해제하기 전에 타겟을 먼저 비운다.
	// 그래야 아직 살아있는 태스크가 컴포넌트의 null 변경 브로드캐스트를 받아 레티클을 즉시 정리한다.
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (UWxLockOnManagerComponent* LockOnComp = UWxLockOnManagerComponent::FindComponent(GetOwningActorFromActorInfo()))
		{
			LockOnComp->SetLockOnTarget(nullptr);
		}

		if (SavedOrientRotationToMovement.IsSet())
		{
			const ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
			if (UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr)
			{
				Movement->bOrientRotationToMovement = SavedOrientRotationToMovement.GetValue();
			}

			SavedOrientRotationToMovement.Reset();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	LockOnTask = nullptr;
}

void UWxAbility_LockOn::HandleTargetLost()
{
	AActor* Avatar = GetOwningActorFromActorInfo();
	UWxLockOnManagerComponent* LockOnComp = UWxLockOnManagerComponent::FindComponent(Avatar);
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

			// 컴포넌트에만 설정하면 태스크가 OnLockOnTargetChanged로 즉시 추적을 잇는다(권위 반영은 서버 RPC).
			LockOnComp->SetLockOnTarget(TargetComponent);
			return;
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UWxAbility_LockOn::HandleRetargetRequested(FVector2D ScreenDirection)
{
	if (!LockOnTask)
	{
		return;
	}

	APlayerController* PC = CurrentActorInfo ? Cast<APlayerController>(CurrentActorInfo->PlayerController.Get()) : nullptr;
	AActor* Avatar = GetOwningActorFromActorInfo();
	if (!PC || !Avatar)
	{
		return;
	}

	UWxLockOnManagerComponent* LockOnComp = UWxLockOnManagerComponent::FindComponent(Avatar);
	const USceneComponent* CurrentComponent = LockOnComp ? LockOnComp->GetLockOnTarget() : nullptr;

	// 비교 원점은 현재 락온 지점의 화면 좌표(유저가 보고 있는 레티클 위치).
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

	if (BestTargetComponent && LockOnComp)
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
	TargetingSubsystem->ExecuteTargetingRequestWithHandle(RequestHandle, FTargetingRequestDelegate());

	FTargetingDefaultResultsSet& ResultsSet = FTargetingDefaultResultsSet::FindOrAdd(RequestHandle);
	for (const FTargetingDefaultResultData& Result : ResultsSet.TargetResults)
	{
		if (AActor* Actor = Result.HitResult.GetActor())
		{
			OutCandidates.Add(Actor);
		}
	}

	UTargetingSubsystem::ReleaseTargetRequestHandle(RequestHandle);
}
