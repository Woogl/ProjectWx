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

UWxAnimNotifyState_AbilityEvent::UWxAnimNotifyState_AbilityEvent()
{
	// Queued 구간은 같은 몽타주를 다시 재생하면 앞 인스턴스와 합쳐져 시작·끝이 빠지고 저프레임에 스킵될 수 있어, 인스턴스별로 동기 실행되는 분기점으로 고정한다.
	bIsNativeBranchingPoint = true;
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
