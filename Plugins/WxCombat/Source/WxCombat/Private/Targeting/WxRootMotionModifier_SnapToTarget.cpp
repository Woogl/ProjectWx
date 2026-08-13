// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxRootMotionModifier_SnapToTarget.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "MotionWarpingComponent.h"
#include "Targeting/WxLockOnManagerComponent.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "WxGameplayTags.h"

void UWxRootMotionModifier_SnapToTarget::OnStateChanged(ERootMotionModifierState LastState)
{
	Super::OnStateChanged(LastState);

	// Active로 막 진입한 프레임에만 스냅 타겟을 판정해 워프 타겟을 등록한다.
	if (LastState == ERootMotionModifierState::Active || GetState() != ERootMotionModifierState::Active)
	{
		return;
	}

	AActor* Owner = GetActorOwner();
	UMotionWarpingComponent* MotionWarpingComp = GetOwnerComponent();
	if (!Owner || !MotionWarpingComp)
	{
		return;
	}

	// 락온 대상이 최우선이며, 부위 컴포넌트에서 소유 액터를 환원해 쓴다.
	AActor* LockOnTarget = nullptr;
	if (UWxLockOnManagerComponent* LockOnComp = UWxLockOnManagerComponent::FindComponent(Owner))
	{
		if (const USceneComponent* LockOnTargetComponent = LockOnComp->GetLockOnTarget())
		{
			LockOnTarget = LockOnTargetComponent->GetOwner();
		}
	}

	// TargetingPreset 쿼리 결과가 곧 스냅 가능 범위다.
	// 락온 대상이 결과에 있으면 위치 스냅까지 허용하고, 밖이면 회전만 적용한다.
	TArray<AActor*> TargetingResults;
	if (TargetingPreset)
	{
		if (UTargetingSubsystem* TargetingSubsystem = UTargetingSubsystem::Get(Owner->GetWorld()))
		{
			FTargetingSourceContext SourceContext;
			SourceContext.SourceActor = Owner;
			SourceContext.InstigatorActor = Owner;

			FTargetingRequestHandle Handle = TargetingSubsystem->MakeTargetRequestHandle(TargetingPreset, SourceContext);
			TargetingSubsystem->ExecuteTargetingRequestWithHandle(Handle);
			TargetingSubsystem->GetTargetingResultsActors(Handle, TargetingResults);
			TargetingSubsystem->ReleaseTargetRequestHandle(Handle);
		}
	}

	AActor* FacingTarget = LockOnTarget;
	if (!FacingTarget && TargetingResults.Num() > 0)
	{
		FacingTarget = TargetingResults[0];
	}

	// 워프 타겟은 이름으로 컴포넌트에 남고 대상 컴포넌트를 계속 추종한다. 몽타주가 끝나도 엔진은 지우지 않는다.
	// 따라서 대상을 못 찾아 새로 등록하지 않으면 직전 공격이 남긴 타겟을 그대로 집어 써, 이미 죽은 대상을 계속 응시한다.
	// 여기서 자기 워프 타겟을 지우면 부모 Warp::Update 가 타겟 부재를 감지해 modifier 를 끄고 순정 루트 모션으로 재생한다.
	USceneComponent* TargetComponent = FacingTarget ? FacingTarget->GetRootComponent() : nullptr;
	if (!TargetComponent)
	{
		MotionWarpingComp->RemoveWarpTarget(WarpTargetName);
		return;
	}

	SnapTarget = FacingTarget;

	const bool bFacingTargetIsLockOn = (FacingTarget == LockOnTarget);

	// Preset이 없으면 판정 근거가 없으므로 허용한다.
	const bool bTargetInSnapRange = !TargetingPreset || TargetingResults.Contains(FacingTarget);

	// 위치 워프 디싱크는 클라가 예측하는 플레이어 폰에서만 문제라, 그 경우만 복제되는 락온 대상으로 제한한다.
	// 서버 권위로만 도는 AI 등은 폴백 위치 스냅을 유지하며, IsPlayerControlled는 소유 클라·서버 양쪽에서 일관된다.
	const APawn* OwnerPawn = Cast<APawn>(Owner);
	const bool bRequireLockOnForTranslation = OwnerPawn && OwnerPawn->IsPlayerControlled();
	const bool bShouldWarpTranslation = bWarpTranslation && bTargetInSnapRange && (!bRequireLockOnForTranslation || bFacingTargetIsLockOn);

	bWarpTranslation = bShouldWarpTranslation;

	// 컴포넌트 추종으로 등록해 워프 중 대상 이동을 매 프레임 추적한다.
	// 접근·회전 모두 수평(yaw) 전용이며, 작은 높이차는 SkewWarp의 bIgnoreZAxis와 캡슐 step-up이 흡수한다.
	MotionWarpingComp->AddOrUpdateWarpTargetFromComponent(WarpTargetName, TargetComponent, NAME_None, true, EWarpTargetLocationOffsetDirection::VectorFromTargetToOwner, LocationOffset);
}

void UWxRootMotionModifier_SnapToTarget::Update(const FMotionWarpingUpdateContext& Context)
{
	// 부모 SkewWarp는 창 끝까지 반드시 도달시키는 마감형 보정이라 매 프레임 "남은 거리 / 남은 시간" 속도를 상한 없이 요구한다.
	// 대상이 죽으면 시체가 루트 모션으로 밀리고 래그돌로 캡슐까지 사라져 워프 타겟이 흔들리는데, 창 끝 몇 프레임에 겹치면 그 요구 속도가 그대로 튄다.
	// 자격을 잃은 즉시 타겟을 거두면 Super가 부재를 감지해 modifier를 끄므로 남은 구간은 순정 루트 모션으로 재생된다.
	if (GetState() == ERootMotionModifierState::Active && !IsSnapTargetAlive())
	{
		if (UMotionWarpingComponent* MotionWarpingComp = GetOwnerComponent())
		{
			MotionWarpingComp->RemoveWarpTarget(WarpTargetName);
		}
	}

	Super::Update(Context);
}

bool UWxRootMotionModifier_SnapToTarget::IsSnapTargetAlive() const
{
	const UAbilitySystemComponent* TargetAbilitySystem = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SnapTarget.Get());
	if (!TargetAbilitySystem)
	{
		return true;
	}

	return !TargetAbilitySystem->HasMatchingGameplayTag(WxGameplayTags::Ability_Death);
}
