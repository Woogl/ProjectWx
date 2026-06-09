// Copyright Woogle. All Rights Reserved.

#include "WxPatrolComponent.h"

#include "GameFramework/Actor.h"

UWxPatrolComponent* UWxPatrolComponent::FindPatrolComponent(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	// 스포너가 스폰 시 Owner 로 자신을 지정하고(빙의 전 보장) 자신에게 부착한다. 그 액터에서 정찰 컴포넌트를 찾는다.
	// Owner 를 먼저 보는 이유: 부착은 FinishSpawning 이후라 빙의 시점엔 아직 없지만, Owner 는 그 전에 세팅돼 있다.
	if (const AActor* Owner = Actor->GetOwner())
	{
		if (UWxPatrolComponent* Found = Owner->FindComponentByClass<UWxPatrolComponent>())
		{
			return Found;
		}
	}

	if (const AActor* AttachParent = Actor->GetAttachParentActor())
	{
		return AttachParent->FindComponentByClass<UWxPatrolComponent>();
	}

	return nullptr;
}

int32 UWxPatrolComponent::GetNumPoints() const
{
	return GetNumberOfSplinePoints();
}

FVector UWxPatrolComponent::GetPointLocation(int32 Index) const
{
	return GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World);
}

bool UWxPatrolComponent::GetNextIndex(int32 CurrentIndex, int32& InOutDirection, int32& OutNextIndex) const
{
	const int32 NumPoints = GetNumberOfSplinePoints();

	// 빈 경로 또는 단일 지점: 진행할 다음 지점이 없다.
	if (NumPoints <= 1)
	{
		OutNextIndex = CurrentIndex;
		return false;
	}

	switch (MoveMode)
	{
	case EWxPatrolMoveMode::Loop:
		OutNextIndex = (CurrentIndex + 1) % NumPoints;
		return true;

	case EWxPatrolMoveMode::PingPong:
		if (CurrentIndex + InOutDirection < 0 || CurrentIndex + InOutDirection >= NumPoints)
		{
			InOutDirection = -InOutDirection;
		}
		OutNextIndex = CurrentIndex + InOutDirection;
		return true;

	case EWxPatrolMoveMode::Once:
		if (CurrentIndex + 1 >= NumPoints)
		{
			OutNextIndex = CurrentIndex;
			return false;
		}
		OutNextIndex = CurrentIndex + 1;
		return true;
	}

	OutNextIndex = CurrentIndex;
	return false;
}

#if WITH_EDITOR
void UWxPatrolComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 에디터에서 MoveMode 를 바꾸면 스플라인 닫힘 상태도 즉시 따라가게 한다.
	SetClosedLoop(MoveMode == EWxPatrolMoveMode::Loop);
}
#endif

void UWxPatrolComponent::OnRegister()
{
	Super::OnRegister();

	// Loop 모드면 마지막 포인트가 첫 포인트로 이어지도록 스플라인을 닫는다.
	SetClosedLoop(MoveMode == EWxPatrolMoveMode::Loop);
}
