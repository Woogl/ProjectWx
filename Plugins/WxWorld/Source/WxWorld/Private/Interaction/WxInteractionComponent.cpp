// Copyright Woogle. All Rights Reserved.

#include "Interaction/WxInteractionComponent.h"

#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Net/UnrealNetwork.h"
#include "WxCollisionChannels.h"

UWxInteractionComponent::UWxInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// OnInteracted 자체는 서버 전용이라 RPC가 없지만, 상호작용 어빌리티의 TargetData(FWxAbilityTargetData_Interaction)가 이 컴포넌트 포인터를 PackageMap으로 클라→서버 직렬화한다.
	// 동적 스폰 액터(픽업·적)의 컴포넌트도 net-addressable 하려면 복제가 필요하다.
	SetIsReplicatedByDefault(true);

	InteractionText = FText::FromString(TEXT("Interact"));
}

void UWxInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	// 볼륨 미지정 시 부착 부모 프리미티브를 자동 채택한다. 대부분의 인터랙션 컴포넌트는 대상 메시에 직접 부착되므로 별도 지정이 필요 없다.
	// 부착 부모가 프리미티브가 아닌 경우(예: 엘리베이터 플랫폼은 SceneComponent 에 부착)에만 소유자가 SetCollisionVolume 으로 명시한다.
	if (!CollisionVolume)
	{
		CollisionVolume = Cast<UPrimitiveComponent>(GetAttachParent());
	}

	if (!CollisionVolume)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: CollisionVolume 미지정 — 상호작용 스캔에 잡히지 않는다."), *GetPathName());
	}

	// 볼륨의 WxInteractable 응답을 현재 활성 상태에 맞춰 초기 세팅한다(메시의 다른 콜리전은 건드리지 않는다).
	ApplyInteractionCollision();
}

void UWxInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWxInteractionComponent, bInteractionEnabled);
}

void UWxInteractionComponent::SetCollisionVolume(UPrimitiveComponent* InVolume)
{
	CollisionVolume = InVolume;
}

UPrimitiveComponent* UWxInteractionComponent::GetCollisionVolume() const
{
	return CollisionVolume;
}

FVector UWxInteractionComponent::GetInteractionLocation() const
{
	return CollisionVolume ? CollisionVolume->GetComponentLocation() : GetComponentLocation();
}

float UWxInteractionComponent::GetInteractionReachRadius() const
{
	return CollisionVolume ? CollisionVolume->Bounds.SphereRadius : 0.f;
}

UWxInteractionComponent* UWxInteractionComponent::FindByCollisionVolume(const UPrimitiveComponent* Volume)
{
	if (!Volume)
	{
		return nullptr;
	}

	const AActor* Owner = Volume->GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	// 소유 액터에서 이 볼륨을 참조하는 상호작용 컴포넌트를 찾는다(액터당 인터랙션 컴포넌트 수는 소수).
	TArray<UWxInteractionComponent*> Components;
	Owner->GetComponents(Components);
	for (UWxInteractionComponent* Component : Components)
	{
		if (Component->CollisionVolume == Volume)
		{
			return Component;
		}
	}
	return nullptr;
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
	// 볼륨 미지정 시(예: BeginPlay 이전 조기 OnRep) 안전하게 무시한다. BeginPlay 에서 다시 적용된다.
	if (!CollisionVolume)
	{
		return;
	}

	// 볼륨의 WxInteractable 응답만 토글한다. 메시의 CollisionEnabled·ObjectType·다른 응답은 건드리지 않아 본래 콜리전이 보존된다.
	// 비활성(Ignore) 볼륨은 스캐너의 OverlapMultiByChannel 에 잡히지 않아 스캔에서 자연 탈락한다.
	CollisionVolume->SetCollisionResponseToChannel(WxCollision::WxInteractable, bInteractionEnabled ? ECR_Overlap : ECR_Ignore);

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

FText UWxInteractionComponent::GetInteractionText() const
{
	return InteractionText;
}

void UWxInteractionComponent::SetHighlightEnabled(bool bNewEnabled)
{
	// 강조 대상은 소유 액터가 명시 지정한다(SetHighlightTarget / BP). 미지정이면 강조하지 않는다.
	if (!bUseHighlight || !HighlightTarget)
	{
		return;
	}

	HighlightTarget->SetRenderCustomDepth(bNewEnabled);
	if (bNewEnabled)
	{
		HighlightTarget->SetCustomDepthStencilValue(HighlightStencilValue);
	}
}

void UWxInteractionComponent::SetHighlightTarget(UMeshComponent* InTarget)
{
	HighlightTarget = InTarget;
}

void UWxInteractionComponent::SetUseHighlight(bool bNewUseHighlight)
{
	if (bUseHighlight == bNewUseHighlight)
	{
		return;
	}

	// 게이트를 닫기 전에 이미 켜진 외곽선을 끈다(닫힌 뒤에는 SetHighlightEnabled 가 early-return 이라 끌 수 없다).
	if (!bNewUseHighlight)
	{
		SetHighlightEnabled(false);
	}

	bUseHighlight = bNewUseHighlight;
}

void UWxInteractionComponent::SetUseInteractMontage(bool bNewUseInteractMontage)
{
	bUseInteractMontage = bNewUseInteractMontage;
}

bool UWxInteractionComponent::GetUseInteractMontage() const
{
	return bUseInteractMontage;
}
