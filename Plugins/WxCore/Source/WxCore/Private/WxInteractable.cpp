// Copyright Woogle. All Rights Reserved.

#include "WxInteractable.h"

#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

IWxInteractable* IWxInteractable::Find(const UActorComponent* Mesh)
{
	AActor* Owner = Mesh ? Mesh->GetOwner() : nullptr;
	if (!Owner)
	{
		return nullptr;
	}

	// 액터가 직접 구현했으면 그것이 답이다(NPC·픽업·적처럼 상호작용이 액터 고유 동작인 경우).
	if (IWxInteractable* OwnerImplementation = Cast<IWxInteractable>(Owner))
	{
		return OwnerImplementation;
	}

	// 아니면 컴포넌트가 계약을 든다(기믹). 이 갈래 덕에 호스트 액터는 C++ 없이 순수 BP 로 둘 수 있다.
	return Cast<IWxInteractable>(Owner->FindComponentByInterface(UWxInteractable::StaticClass()));
}

bool IWxInteractable::IsMeshInRange(const UPrimitiveComponent* Mesh, const FVector& Origin, float Radius)
{
	if (!Mesh)
	{
		return false;
	}

	// 쿼리 콜리전이 꺼져 있으면 아래 테스트가 무조건 false 라 상호작용이 경고 한 줄 없이 통째로 사라진다.
	// 폴백을 두면 판정 기준이 대상마다 갈리므로, 동작은 그대로 두고 전제가 깨진 사실만 개발 빌드에서 드러낸다.
	ensureMsgf(Mesh->IsQueryCollisionEnabled(),
		TEXT("%s 의 %s 에 쿼리 콜리전이 꺼져 있어 상호작용 사거리 판정이 항상 실패한다(스켈레탈이면 피직스 애셋도 필요)."),
		*GetNameSafe(Mesh->GetOwner()), *Mesh->GetName());

	// 콜리전 형상에 구를 던져 겹치는지로 잰다 — 스켈레탈 메시는 오버라이드가 피직스 애셋의 모든 바디를 훑는다.
	// 채널이 아니라 바디에 직접 던지는 테스트라 콜리전 응답·프로파일은 보지 않는다. 쿼리 콜리전이 켜져 있기만 하면 된다.
	return Mesh->OverlapComponent(Origin, FQuat::Identity, FCollisionShape::MakeSphere(Radius));
}

bool IWxInteractable::CanBeInteractedBy(const AActor* Interactor, const UActorComponent* Source) const
{
	return true;
}
