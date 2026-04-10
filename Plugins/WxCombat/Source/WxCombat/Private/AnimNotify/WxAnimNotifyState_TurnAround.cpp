// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_TurnAround.h"
#include "Targeting/WxLockOnComponent.h"
#include "MotionWarpingComponent.h"
#include "RootMotionModifier.h"
#include "RootMotionModifier_SkewWarp.h"
#include "TargetingSystem/TargetingSubsystem.h"

const FName UWxAnimNotifyState_TurnAround::DefaultWarpTargetName = TEXT("TurnAround");

void UWxAnimNotifyState_TurnAround::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (!MeshComp || !TargetingPreset)
    {
        return;
    }

    AActor* Owner = MeshComp->GetOwner();
    if (!Owner)
    {
        return;
    }

    UMotionWarpingComponent* MotionWarpingComp = Owner->FindComponentByClass<UMotionWarpingComponent>();
    if (!MotionWarpingComp)
    {
        return;
    }

    // 타겟 탐색: 락온 대상 우선
    AActor* FoundTarget = nullptr;

    if (UWxLockOnComponent* LockOnComp = UWxLockOnComponent::FindComponent(Owner))
    {
        FoundTarget = LockOnComp->GetLockOnTarget();
    }

    // 락온 대상이 없으면 TargetingSubsystem으로 탐색
    if (!FoundTarget)
    {
        UTargetingSubsystem* TargetingSubsystem = UTargetingSubsystem::Get(Owner->GetWorld());
        if (!TargetingSubsystem)
        {
            return;
        }

        FTargetingSourceContext SourceContext;
        SourceContext.SourceActor = Owner;
        SourceContext.InstigatorActor = Owner;

        FTargetingRequestHandle Handle = TargetingSubsystem->MakeTargetRequestHandle(TargetingPreset, SourceContext);
        TargetingSubsystem->ExecuteTargetingRequestWithHandle(Handle);

        TArray<AActor*> Targets;
        TargetingSubsystem->GetTargetingResultsActors(Handle, Targets);
        TargetingSubsystem->ReleaseTargetRequestHandle(Handle);

        if (Targets.Num() > 0)
        {
            FoundTarget = Targets[0];
        }
    }

    if (!FoundTarget)
    {
        return;
    }

    // 타겟 방향 Rotation 계산 (Yaw만 사용)
    FVector Direction = FoundTarget->GetActorLocation() - Owner->GetActorLocation();
    Direction.Z = 0.0;
    if (Direction.IsNearlyZero())
    {
        return;
    }

    MotionWarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(DefaultWarpTargetName, FoundTarget->GetActorLocation(), Direction.Rotation());
}

void UWxAnimNotifyState_TurnAround::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if (!MeshComp)
    {
        return;
    }

    AActor* Owner = MeshComp->GetOwner();
    if (!Owner)
    {
        return;
    }

    if (UMotionWarpingComponent* MotionWarpingComp = Owner->FindComponentByClass<UMotionWarpingComponent>())
    {
        MotionWarpingComp->RemoveWarpTarget(DefaultWarpTargetName);
    }
}

URootMotionModifier* UWxAnimNotifyState_TurnAround::AddRootMotionModifier_Implementation(UMotionWarpingComponent* MotionWarpingComp, const UAnimSequenceBase* Animation, float StartTime, float EndTime) const
{
    ApplyDefaultWarpSettings();
    return Super::AddRootMotionModifier_Implementation(MotionWarpingComp, Animation, StartTime, EndTime);
}

FString UWxAnimNotifyState_TurnAround::GetNotifyName_Implementation() const
{
    return TEXT("Turn Around");
}

#if WITH_EDITOR
void UWxAnimNotifyState_TurnAround::ValidateAssociatedAssets()
{
    ApplyDefaultWarpSettings();
    Super::ValidateAssociatedAssets();
}
#endif

void UWxAnimNotifyState_TurnAround::ApplyDefaultWarpSettings() const
{
    if (URootMotionModifier_SkewWarp* SkewWarp = Cast<URootMotionModifier_SkewWarp>(RootMotionModifier))
    {
        SkewWarp->WarpTargetName = DefaultWarpTargetName;
        SkewWarp->bWarpTranslation = false;
        SkewWarp->bWarpRotation = true;
        SkewWarp->RotationType = EMotionWarpRotationType::Facing;
    }
}