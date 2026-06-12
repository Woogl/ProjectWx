// Copyright Woogle. All Rights Reserved.

#include "AbilitySystem/Ability/WxAbility_LockOn.h"
#include "AbilitySystem/Task/WxAbilityTask_LockOnTarget.h"
#include "AbilitySystemBlueprintLibrary.h"
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
	// AssetTag 의도적 미설정: Ability 태그가 없어야 HitReact/Guard 등의
	// CancelAbilitiesWithTag(Ability)에 의해 락온이 해제되지 않는다.
	ActivationBlockedTags.AddTag(WxGameplayTags::State_Dead);
	ActivationOwnedTags.AddTag(WxGameplayTags::State_LockOn);
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

	// 락온 중에는 이동 방향 회전을 끈다. 캐릭터를 타겟으로 향하게 하는 회전은 태스크가 부드럽게 보간한다. EndAbility에서 복구.
	// autonomous proxy 의 회전 정합을 위해 서버에서도 꺼야 하므로 IsLocallyControlled 게이트 앞에서 처리한다.
	if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
	{
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	}

	// 타겟 결정과 추적 태스크는 소유 클라(또는 리슨 서버 호스트)에서만 처리한다. 태스크는 카메라/몸체 추적·재탐색 입력 폴링·레티클의 로컬 어포던스다.
	// 서버(리모트 플레이어)는 소유 클라의 SetLockOnTarget RPC 로 복제된 LockOnTarget 만 보유하며, 발사체/스냅 등 소비처가 그 값을 읽는다.
	if (!IsLocallyControlled())
	{
		return;
	}

	// TargetingSubsystem으로 동기 타겟 탐색. 결과는 프리셋이 거리순으로 정렬하므로 첫 번째가 가장 가까운 타겟.
	TArray<AActor*> Candidates;
	GatherCandidates(Candidates);

	// 락온 지점(UWxLockOnPointComponent)이 있는 액터만 락온 대상이 된다. 후보는 거리순이므로 지점을 가진 가장 가까운 액터를 택한다.
	// 대상·카메라/캐릭터 시선·레티클·호밍이 모두 이 컴포넌트 위치를 따라가므로, 조준 부위를 바꾸려면 지점 배치만 옮기면 된다.
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

	// 락온 대상 등록. 소유 클라는 로컬 즉시 반영 + 서버 RPC, 리슨 호스트는 권위 직접 반영(컴포넌트가 복제 처리).
	if (UWxLockOnComponent* LockOnComp = UWxLockOnComponent::FindComponent(GetOwningActorFromActorInfo()))
	{
		LockOnComp->SetLockOnTarget(TargetComponent);
	}

	// 락온 태스크 생성
	LockOnTask = UWxAbilityTask_LockOnTarget::CreateTask(this, TargetComponent, CameraInterpSpeed, CameraPitchOffset, MaxDistance, CharacterInterpSpeed, ReticleWidgetClass, LookAction, RetargetLookThreshold);
	LockOnTask->OnTargetLost.AddDynamic(this, &UWxAbility_LockOn::HandleTargetLost);
	LockOnTask->OnRetargetRequested.AddDynamic(this, &UWxAbility_LockOn::HandleRetargetRequested);
	LockOnTask->ReadyForActivation();
}

void UWxAbility_LockOn::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Super::EndAbility 가 태스크를 해제하기 전에 타겟을 먼저 비운다. 그래야 아직 살아있는 태스크가
	// 컴포넌트의 null 변경 브로드캐스트를 받아 레티클을 즉시 정리한다.
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		if (UWxLockOnComponent* LockOnComp = UWxLockOnComponent::FindComponent(GetOwningActorFromActorInfo()))
		{
			LockOnComp->SetLockOnTarget(nullptr);
		}

		// 락온 해제 시 OrientToMovement 복구. 이동 시 RotationRate로 부드럽게 이동 방향으로 돌아온다.
		if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
		{
			Character->GetCharacterMovement()->bOrientRotationToMovement = true;
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	LockOnTask = nullptr;
}

void UWxAbility_LockOn::HandleTargetLost()
{
	// 재탐색이 켜져 있으면 잃은 대상을 제외하고 살아있는 가장 가까운 적으로 갈아탄다. 후보가 없을 때만 락온을 해제한다.
	// 타겟 결정은 활성화와 동일하게 소유 클라(또는 리슨 호스트)에서만 한다. 서버는 SetLockOnTarget RPC 로 복제된다.
	AActor* Avatar = GetOwningActorFromActorInfo();
	UWxLockOnComponent* LockOnComp = UWxLockOnComponent::FindComponent(Avatar);
	if (bRetargetOnTargetLost && IsLocallyControlled() && Avatar && LockOnComp)
	{
		// 락온 대상은 컴포넌트지만 후보 비교/제외는 액터 단위이므로 소유 액터로 환원한다.
		const USceneComponent* LostComponent = LockOnComp->GetLockOnTarget();
		const AActor* LostTarget = LostComponent ? LostComponent->GetOwner() : nullptr;
		const FVector AvatarLocation = Avatar->GetActorLocation();
		const float MaxDistanceSquared = MaxDistance * MaxDistance;

		// 후보는 프리셋이 거리순 정렬하므로 첫 유효 후보가 가장 가까운 다른 적이다.
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

			// 사망 직후 프리셋이 시체를 아직 거르지 못할 수 있으므로 죽은 후보는 건너뛴다.
			const UAbilitySystemComponent* CandidateASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Candidate);
			if (CandidateASC && CandidateASC->HasMatchingGameplayTag(WxGameplayTags::State_Dead))
			{
				continue;
			}

			// 락온 지점이 없는 후보는 락온 대상이 될 수 없으므로 건너뛴다.
			USceneComponent* TargetComponent = UWxLockOnPointComponent::ResolveLockOnTarget(Candidate);
			if (!TargetComponent)
			{
				continue;
			}

			// 컴포넌트에만 설정하면 태스크가 OnLockOnTargetChanged 로 즉시 추적을 이어간다(서버 RPC 로 권위 반영).
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

	UWxLockOnComponent* LockOnComp = UWxLockOnComponent::FindComponent(Avatar);
	const USceneComponent* CurrentComponent = LockOnComp ? LockOnComp->GetLockOnTarget() : nullptr;
	const AActor* CurrentTarget = CurrentComponent ? CurrentComponent->GetOwner() : nullptr;

	// 시선 입력 방향(화면 기준 2D)을 카메라 평면 위의 월드 방향으로 변환한다.
	const FRotationMatrix ControlRotMatrix(PC->GetControlRotation());
	const FVector CamForward = ControlRotMatrix.GetUnitAxis(EAxis::X);
	const FVector CamRight = ControlRotMatrix.GetUnitAxis(EAxis::Y);
	const FVector CamUp = ControlRotMatrix.GetUnitAxis(EAxis::Z);
	const FVector DesiredDir = (CamRight * ScreenDirection.X + CamUp * ScreenDirection.Y).GetSafeNormal();
	if (DesiredDir.IsNearlyZero())
	{
		return;
	}

	// 후보들 중 현재 타겟을 제외하고, 시선이 가리키는 방향에 가장 잘 정렬된 적을 고른다.
	TArray<AActor*> Candidates;
	GatherCandidates(Candidates);

	const FVector AvatarLocation = Avatar->GetActorLocation();
	USceneComponent* BestTargetComponent = nullptr;
	float BestAlignment = RetargetMinAlignment;
	for (AActor* Candidate : Candidates)
	{
		if (!Candidate || Candidate == CurrentTarget)
		{
			continue;
		}

		// 락온 지점이 없는 후보는 대상이 될 수 없으므로 제외한다(없는데 선택되면 대상이 nullptr로 비워진다).
		USceneComponent* CandidateComponent = UWxLockOnPointComponent::ResolveLockOnTarget(Candidate);
		if (!CandidateComponent)
		{
			continue;
		}

		// 후보 방향에서 카메라 정면 성분을 제거해 화면 평면에 투영한 뒤 시선 방향과의 정렬도를 비교한다.
		const FVector ToTarget = Candidate->GetActorLocation() - AvatarLocation;
		const FVector ProjectedDir = (ToTarget - FVector::DotProduct(ToTarget, CamForward) * CamForward).GetSafeNormal();
		const float Alignment = FVector::DotProduct(ProjectedDir, DesiredDir);
		if (Alignment > BestAlignment)
		{
			BestAlignment = Alignment;
			BestTargetComponent = CandidateComponent;
		}
	}

	if (BestTargetComponent && LockOnComp)
	{
		// 컴포넌트에만 설정한다. 로컬 예측 브로드캐스트가 태스크를 즉시 갱신하고(응답성), 서버 RPC 로 권위 반영 후 전 머신에 복제된다.
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
