// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxInteractionComponent.h"

#include "Components/MeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "WxCollisionChannels.h"

UWxInteractionComponent::UWxInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// OnInteracted 자체는 서버 전용이라 RPC가 없지만, 상호작용 어빌리티의 TargetData(FWxAbilityTargetData_Interaction)가
	// 이 컴포넌트 포인터를 PackageMap으로 클라→서버 직렬화한다. 동적 스폰 액터(픽업·적)의 컴포넌트도 net-addressable 하려면 복제가 필요하다.
	SetIsReplicatedByDefault(true);

	// 플레이어 측 스캐너가 OverlapMultiByObjectType 으로 수집하는 수동 쿼리 볼륨이다. 오버랩 이벤트는 쓰지 않는다.
	InitSphereRadius(10.f);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(WxCollision::WxInteractable);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetGenerateOverlapEvents(false);

	InteractionText = FText::FromString(TEXT("Interact"));
}

void UWxInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWxInteractionComponent, bInteractionEnabled);
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

	// 권위·비권위 모두 즉시 로컬 적용한다(gimmick 은 복제 State 로 ST 가 양측에서 SetInteractionEnabled 를 구동하므로 즉시성이 필요).
	// 권위 쓰기는 복제되어 서버 전용 구동(enemy 어포던스)도 OnRep 으로 원격 클라에 반영된다.
	ApplyInteractionCollision();
}

void UWxInteractionComponent::OnRep_InteractionEnabled()
{
	// 권위에서 토글된 활성 상태를 클라 쿼리 콜리전에 반영한다 — 서버 전용으로 구동되는 소유자도 이 경로로 원격 클라의 스캔 포함/탈락이 맞춰진다.
	ApplyInteractionCollision();
}

void UWxInteractionComponent::ApplyInteractionCollision()
{
	// 쿼리 가능 여부만 토글한다. 비활성 컴포넌트는 ObjectType 쿼리에 잡히지 않아 스캔에서 자연 탈락한다.
	SetCollisionEnabled(bInteractionEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

	// 비활성화 즉시 외곽선을 끈다 — 레지스트리 다음 스캔까지의 잔상을 막는다. 활성화 시 강조 ON 은 레지스트리가 선택 대상만 결정한다.
	if (!bInteractionEnabled)
	{
		SetHighlightEnabled(false);
	}
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
