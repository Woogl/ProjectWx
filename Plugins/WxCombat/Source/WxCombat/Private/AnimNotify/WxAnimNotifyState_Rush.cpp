// Copyright Woogle. All Rights Reserved.

#include "AnimNotify/WxAnimNotifyState_Rush.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimNotifyQueue.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Minion/WxMinionSubsystem.h"
#include "MotionWarpingComponent.h"
#include "Targeting/WxLockOnComponent.h"
#include "Targeting/WxRootMotionModifier_Rush.h"
#include "WxGameplayTags.h"

UWxAnimNotifyState_Rush::UWxAnimNotifyState_Rush()
{
	bIsNativeBranchingPoint = true;
}

void UWxAnimNotifyState_Rush::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload)
{
	const FAnimNotifyEventReference Reference(Payload.NotifyEvent, Payload.SequenceAsset);
	NotifyBegin(Payload.SkelMeshComponent, Payload.SequenceAsset, Payload.NotifyEvent ? Payload.NotifyEvent->GetDuration() : 0.f, Reference);
}

void UWxAnimNotifyState_Rush::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& Payload)
{
	// 정상 종료 프레임의 루트 모션까지 적용한 뒤 modifier가 정리한다.
	if (!Payload.bReachedEnd)
	{
		Super::BranchingPointNotifyEnd(Payload);
	}
}

AActor* UWxAnimNotifyState_Rush::FindTarget(APawn& Avatar) const
{
	if (TargetSource == EWxRushTarget::LockOnTarget)
	{
		return UWxLockOnComponent::ResolveLockOnTargetActor(&Avatar);
	}
	if (TargetSource == EWxRushTarget::Master)
	{
		return UWxMinionSubsystem::GetMaster(Avatar);
	}
	if (Avatar.HasAuthority())
	{
		const UWxMinionSubsystem* Subsystem = Avatar.GetWorld()->GetSubsystem<UWxMinionSubsystem>();
		return Subsystem ? Subsystem->FindActiveMinion(Avatar) : nullptr;
	}

	// 오너 예측 실행에는 서버 로스터가 없으므로 복제된 소환 관계로 찾는다.
	for (TActorIterator<APawn> It(Avatar.GetWorld()); It; ++It)
	{
		if (!It->IsActorBeingDestroyed() && UWxMinionSubsystem::GetMaster(**It) == &Avatar)
		{
			const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(*It);
			if (ASC && !ASC->HasMatchingGameplayTag(WxGameplayTags::Ability_Death))
			{
				return *It;
			}
		}
	}
	return nullptr;
}

void UWxAnimNotifyState_Rush::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	APawn* Avatar = MeshComp ? Cast<APawn>(MeshComp->GetOwner()) : nullptr;
	const FAnimNotifyEvent* Event = EventReference.GetNotify();
	UMotionWarpingComponent* Warping = Avatar ? Avatar->FindComponentByClass<UMotionWarpingComponent>() : nullptr;
	if (!Warping || !Event || !Avatar->GetWorld()->IsGameWorld())
	{
		return;
	}
	AActor* Other = FindTarget(*Avatar);
	if (!Other)
	{
		return;
	}
	UWxRootMotionModifier_Rush* Modifier = NewObject<UWxRootMotionModifier_Rush>(Warping);
	Modifier->Animation = Animation;
	Modifier->StartTime = Event->GetTriggerTime();
	Modifier->EndTime = Event->GetEndTriggerTime();
	Modifier->WarpTargetName = TEXT("Rush");
	Modifier->bWarpTranslation = true;
	Modifier->bIgnoreZAxis = true;
	Modifier->bWarpRotation = true;
	if (!Modifier->InitializeRush(*Other, *this, StopDistance, IgnoreCollisions))
	{
		return;
	}
	Warping->AddModifier(Modifier);
}

void UWxAnimNotifyState_Rush::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	UMotionWarpingComponent* Warping = MeshComp && MeshComp->GetOwner() ? MeshComp->GetOwner()->FindComponentByClass<UMotionWarpingComponent>() : nullptr;
	if (!Warping)
	{
		return;
	}
	for (URootMotionModifier* Modifier : Warping->GetModifiers())
	{
		if (UWxRootMotionModifier_Rush* Rush = Cast<UWxRootMotionModifier_Rush>(Modifier); Rush && Rush->MatchesNotify(*this))
		{
			Rush->CancelRush();
		}
	}
}
