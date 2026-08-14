// Copyright Woogle. All Rights Reserved.

#include "WxInteractable.h"

#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

IWxInteractable* IWxInteractable::Find(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	if (IWxInteractable* ActorImplementation = Cast<IWxInteractable>(Actor))
	{
		return ActorImplementation;
	}

	return Cast<IWxInteractable>(Actor->FindComponentByInterface(UWxInteractable::StaticClass()));
}

IWxInteractable* IWxInteractable::Find(const UActorComponent* Mesh)
{
	return Mesh ? Find(Mesh->GetOwner()) : nullptr;
}

bool IWxInteractable::IsMeshInRange(const UPrimitiveComponent* Mesh, const FVector& Origin, float Radius)
{
	if (!Mesh)
	{
		return false;
	}

	// 폴백을 두면 판정 기준이 대상마다 갈리므로, 동작은 그대로 두고 전제가 깨진 사실만 개발 빌드에서 드러낸다.
	ensureMsgf(Mesh->IsQueryCollisionEnabled(),
		TEXT("%s 의 %s 에 쿼리 콜리전이 꺼져 있어 상호작용 사거리 판정이 항상 실패한다(스켈레탈이면 피직스 애셋도 필요)."),
		*GetNameSafe(Mesh->GetOwner()), *Mesh->GetName());

	// 스켈레탈 메시는 OverlapComponent 오버라이드가 피직스 애셋의 모든 바디를 훑는다.
	return Mesh->OverlapComponent(Origin, FQuat::Identity, FCollisionShape::MakeSphere(Radius));
}

void IWxInteractable::SetInteractionEnabled(bool bEnabled)
{
}

bool IWxInteractable::CanBeInteractedBy(const AActor* Interactor, const UActorComponent* Source) const
{
	return true;
}
