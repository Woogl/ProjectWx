// Copyright Woogle. All Rights Reserved.

#include "Animation/WxAnimNotify_AbilityEvent.h"
#include "AbilitySystem/WxAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	UWxAbilitySystemComponent* GetSignalReceiver(USkeletalMeshComponent* MeshComp)
	{
		return Cast<UWxAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp ? MeshComp->GetOwner() : nullptr));
	}

	FBranchingPointNotifyPayload MakePayload(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
	{
		const UE::Anim::FAnimNotifyMontageInstanceContext* Context = EventReference.GetContextData<UE::Anim::FAnimNotifyMontageInstanceContext>();
		// 엔진 페이로드의 이벤트 포인터는 비const지만 수신자는 읽기만 한다. 신호는 이 호출 안에서 동기로 소비된다.
		return FBranchingPointNotifyPayload(MeshComp, Animation, const_cast<FAnimNotifyEvent*>(EventReference.GetNotify()), Context ? Context->MontageInstanceID : INDEX_NONE);
	}
}

void UWxAnimNotify_AbilityEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	FBranchingPointNotifyPayload Payload = MakePayload(MeshComp, Animation, EventReference);
	BranchingPointNotify(Payload);
}

void UWxAnimNotify_AbilityEvent::BranchingPointNotify(FBranchingPointNotifyPayload& Payload)
{
	if (UWxAbilitySystemComponent* ASC = GetSignalReceiver(Payload.SkelMeshComponent))
	{
		ASC->OnMontageNotify.Broadcast(this, Payload);
	}
}

void UWxAnimNotifyState_AbilityEvent::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	FBranchingPointNotifyPayload Payload = MakePayload(MeshComp, Animation, EventReference);
	BranchingPointNotifyBegin(Payload);
}

void UWxAnimNotifyState_AbilityEvent::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	FBranchingPointNotifyPayload Payload = MakePayload(MeshComp, Animation, EventReference);
	BranchingPointNotifyEnd(Payload);
}

void UWxAnimNotifyState_AbilityEvent::BranchingPointNotifyBegin(FBranchingPointNotifyPayload& Payload)
{
	if (UWxAbilitySystemComponent* ASC = GetSignalReceiver(Payload.SkelMeshComponent))
	{
		ASC->OnMontageNotifyState.Broadcast(this, Payload, true);
	}
}

void UWxAnimNotifyState_AbilityEvent::BranchingPointNotifyEnd(FBranchingPointNotifyPayload& Payload)
{
	if (UWxAbilitySystemComponent* ASC = GetSignalReceiver(Payload.SkelMeshComponent))
	{
		ASC->OnMontageNotifyState.Broadcast(this, Payload, false);
	}
}
