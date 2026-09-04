// Copyright Woogle. All Rights Reserved.

#include "Targeting/WxRootMotionModifier_SnapToTarget.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "MotionWarpingComponent.h"
#include "Targeting/WxLockOnComponent.h"
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

	ApplySnapTarget();
}

void UWxRootMotionModifier_SnapToTarget::Update(const FMotionWarpingUpdateContext& Context)
{
	if (GetState() == ERootMotionModifierState::Active)
	{
		// 부모 SkewWarp는 창 끝까지 반드시 도달시키는 마감형 보정이라 매 프레임 "남은 거리 / 남은 시간" 속도를 상한 없이 요구한다.
		// 대상이 죽으면 시체가 루트 모션으로 밀리고 래그돌로 캡슐까지 사라져 워프 타겟이 흔들리는데, 창 끝 몇 프레임에 겹치면 그 요구 속도가 그대로 튄다.
		if (!IsSnapTargetAlive())
		{
			if (UMotionWarpingComponent* MotionWarpingComp = GetOwnerComponent())
			{
				MotionWarpingComp->RemoveWarpTarget(WarpTargetName);
			}
		}
		// 지정 대상이 창 도중에 도착하거나 바뀌면 회전을 그쪽으로 옮긴다 — 폴백으로 먼저 돈 머신이 권위 값에 수렴하는 경로다.
		// 이동 역할은 재조준하지 않는다. 부모가 남은 시간에 남은 거리를 반드시 메우므로, 창 도중에 대상을 바꾸면 그 프레임에 캐릭터가 튄다.
		// 지정 대상이 없는 프레임에는 아무것도 하지 않아 폴백 상태에서 프리셋을 매 틱 재질의하지 않는다.
		else if (!bWarpTranslation)
		{
			const AActor* DesignatedTarget = UWxLockOnComponent::ResolveLockOnTargetActor(GetActorOwner());
			if (DesignatedTarget && DesignatedTarget != SnapTarget.Get())
			{
				ApplySnapTarget();
			}
		}
	}

	Super::Update(Context);
}

void UWxRootMotionModifier_SnapToTarget::ApplySnapTarget()
{
	AActor* Owner = GetActorOwner();
	UMotionWarpingComponent* MotionWarpingComp = GetOwnerComponent();
	if (!Owner || !MotionWarpingComp)
	{
		return;
	}

	AActor* DesignatedTarget = UWxLockOnComponent::ResolveLockOnTargetActor(Owner);

	// 위치를 옮기는 역할은 전 머신이 같은 값을 읽는 지정 대상에만 건다 — 로컬 프리셋 폴백으로 밀면 머신마다 다른 곳에 선다.
	// 대상이 없으면 범위 판정도 볼 것이 없으므로 쿼리 앞에서 끊는다.
	if (bWarpTranslation && !DesignatedTarget)
	{
		MotionWarpingComp->RemoveWarpTarget(WarpTargetName);
		return;
	}

	// TargetingPreset 쿼리 결과가 곧 스냅 가능 범위이자, 지정 대상이 없을 때 쓸 폴백 후보다.
	// 범위를 볼 이동 역할이거나 폴백이 필요할 때만 돌린다 — 지정 대상을 아는 회전 역할에는 쓸 데가 없다.
	TArray<AActor*> TargetingResults;
	if (TargetingPreset && (bWarpTranslation || !DesignatedTarget))
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

	// 접근은 스냅 가능 범위 안에서만 건다.
	if (bWarpTranslation && TargetingPreset && !TargetingResults.Contains(DesignatedTarget))
	{
		MotionWarpingComp->RemoveWarpTarget(WarpTargetName);
		return;
	}

	// 회전 역할은 지정 대상이 없어도 폴백으로 돈다. 늦게 도착한 지정 대상은 Update 가 다시 잡는다.
	AActor* FacingTarget = DesignatedTarget;
	if (!FacingTarget && TargetingResults.Num() > 0)
	{
		FacingTarget = TargetingResults[0];
	}

	// 워프 타겟은 이름으로 컴포넌트에 남고 몽타주가 끝나도 엔진이 지우지 않으므로, 새로 등록하지 않으면 직전 공격이 남긴 타겟을 그대로 집어 쓴다.
	// 여기서 자기 워프 타겟을 지우면 부모 Warp::Update 가 타겟 부재를 감지해 modifier 를 끄고 순정 루트 모션으로 재생한다.
	USceneComponent* TargetComponent = FacingTarget ? FacingTarget->GetRootComponent() : nullptr;
	if (!TargetComponent)
	{
		MotionWarpingComp->RemoveWarpTarget(WarpTargetName);
		return;
	}

	SnapTarget = FacingTarget;

	// 접근·회전 모두 수평(yaw) 전용이며, 작은 높이차는 SkewWarp의 bIgnoreZAxis와 캡슐 step-up이 흡수한다.
	MotionWarpingComp->AddOrUpdateWarpTargetFromComponent(WarpTargetName, TargetComponent, NAME_None, true, EWarpTargetLocationOffsetDirection::VectorFromTargetToOwner, LocationOffset);
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
