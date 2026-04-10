// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_MoveTo.h"
#include "Targeting/WxLockOnComponent.h"
#include "MotionWarpingComponent.h"
#include "RootMotionModifier.h"
#include "RootMotionModifier_SkewWarp.h"
#include "TargetingSystem/TargetingSubsystem.h"

const FName UWxAnimNotifyState_MoveTo::DefaultWarpTargetName = TEXT("MoveTo");

void UWxAnimNotifyState_MoveTo::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
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

    // 타겟 방향 계산 (수평면만)
    FVector OwnerLocation = Owner->GetActorLocation();
    FVector TargetLocation = FoundTarget->GetActorLocation();
    FVector Direction = TargetLocation - OwnerLocation;
    Direction.Z = 0.0;

    if (Direction.IsNearlyZero())
    {
        return;
    }

    // MinDistance만큼 앞에서 멈추도록 워프 타겟 위치 조정
    const float CurrentDistance = Direction.Size();
    FVector WarpLocation;
    if (CurrentDistance <= MinDistance)
    {
        // 이미 충분히 가까우면 현재 위치를 유지
        WarpLocation = OwnerLocation;
    }
    else
    {
        FVector DirectionNorm = Direction / CurrentDistance;
        WarpLocation = OwnerLocation + DirectionNorm * (CurrentDistance - MinDistance);
    }
    WarpLocation.Z = OwnerLocation.Z;

    MotionWarpingComp->AddOrUpdateWarpTargetFromLocationAndRotation(DefaultWarpTargetName, WarpLocation, Direction.Rotation());
}

void UWxAnimNotifyState_MoveTo::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
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

URootMotionModifier* UWxAnimNotifyState_MoveTo::AddRootMotionModifier_Implementation(UMotionWarpingComponent* MotionWarpingComp, const UAnimSequenceBase* Animation, float StartTime, float EndTime) const
{
    ApplyDefaultWarpSettings();
    return Super::AddRootMotionModifier_Implementation(MotionWarpingComp, Animation, StartTime, EndTime);
}

FString UWxAnimNotifyState_MoveTo::GetNotifyName_Implementation() const
{
    return TEXT("Move To");
}

#if WITH_EDITOR
void UWxAnimNotifyState_MoveTo::ValidateAssociatedAssets()
{
    ApplyDefaultWarpSettings();
    Super::ValidateAssociatedAssets();
}
#endif

void UWxAnimNotifyState_MoveTo::ApplyDefaultWarpSettings() const
{
    if (URootMotionModifier_SkewWarp* SkewWarp = Cast<URootMotionModifier_SkewWarp>(RootMotionModifier))
    {
        SkewWarp->WarpTargetName = DefaultWarpTargetName;
        SkewWarp->bWarpTranslation = true;
        SkewWarp->bWarpRotation = true;
        SkewWarp->RotationType = EMotionWarpRotationType::Facing;
    }
}
