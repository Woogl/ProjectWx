// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxInteractionComponent.h"

#include "Components/MeshComponent.h"
#include "WxCollisionChannels.h"

UWxInteractionComponent::UWxInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// OnInteracted 자체는 서버 전용이라 RPC가 없지만, 상호작용 어빌리티의 TargetData(FWxAbilityTargetData_Interaction)가
	// 이 컴포넌트 포인터를 PackageMap으로 클라→서버 직렬화한다. 동적 스폰 액터(픽업·적)의 컴포넌트도 net-addressable 하려면 복제가 필요하다.
	SetIsReplicatedByDefault(true);

	bInteractionEnabled = true;
	
	// 플레이어 측 스캐너가 OverlapMultiByObjectType 으로 수집하는 수동 쿼리 볼륨이다. 오버랩 이벤트는 쓰지 않는다.
	InitSphereRadius(10.f);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(WxCollision::WxInteractable);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetGenerateOverlapEvents(false);

	InteractionText = FText::FromString(TEXT("Interact"));
}

void UWxInteractionComponent::TryInteract(AActor* InstigatorActor)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !bInteractionEnabled)
	{
		return;
	}

	// 서버 권한에서만 fire한다. 클라 비주얼은 각 대상의 복제 상태(기믹 State, 픽업 Destroy 등)로 수렴하므로 클라 전파는 불필요하다.
	OnInteracted.Broadcast(InstigatorActor);
}

void UWxInteractionComponent::SetInteractionEnabled(bool bEnabled)
{
	if (bInteractionEnabled == bEnabled)
	{
		return;
	}

	bInteractionEnabled = bEnabled;

	// 쿼리 가능 여부만 토글한다. 비활성 컴포넌트는 ObjectType 쿼리에 잡히지 않아 스캔에서 자연 탈락한다.
	SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

FWxOnInteractedSignature& UWxInteractionComponent::GetOnInteractedDelegate()
{
	return OnInteracted;
}

void UWxInteractionComponent::SetInteractionText(const FText& InText)
{
	InteractionText = InText;
}

void UWxInteractionComponent::SetHighlightEnabled(bool bNewEnabled)
{
	// 강조 대상은 소유 액터가 명시 지정한다(SetHighlightTarget / BP). 미지정이면 강조하지 않는다.
	if (!bEnableHighlight || !HighlightTarget)
	{
		return;
	}

	HighlightTarget->SetRenderCustomDepth(bNewEnabled);
	if (bNewEnabled)
	{
		HighlightTarget->SetCustomDepthStencilValue(HighlightStencilValue);
	}
}
