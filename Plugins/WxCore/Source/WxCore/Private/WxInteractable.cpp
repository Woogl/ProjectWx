// Copyright Woogle. All Rights Reserved.

#include "WxInteractable.h"

#include "CollisionShape.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

IWxInteractable* IWxInteractable::Find(const UActorComponent* Mesh)
{
	return Mesh ? Cast<IWxInteractable>(Mesh->GetOwner()) : nullptr;
}

bool IWxInteractable::IsMeshInRange(const UPrimitiveComponent* Mesh, const FVector& Origin, float Radius)
{
	// 콜리전 형상에 구를 던져 겹치는지로 잰다 — 스켈레탈 메시는 오버라이드가 피직스 애셋의 모든 바디를 훑는다.
	// 채널이 아니라 바디에 직접 던지는 테스트라 콜리전 응답·프로파일은 보지 않는다. 쿼리 콜리전이 켜져 있기만 하면 된다.
	return Mesh && Mesh->OverlapComponent(Origin, FQuat::Identity, FCollisionShape::MakeSphere(Radius));
}
