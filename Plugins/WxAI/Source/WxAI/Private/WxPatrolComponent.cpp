// Copyright Woogle. All Rights Reserved.

#include "WxPatrolComponent.h"

#include "Components/ArrowComponent.h"
#include "GameFramework/Controller.h"

UWxPatrolComponent::UWxPatrolComponent()
{
#if WITH_EDITORONLY_DATA
	DirectionArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("DirectionArrow"));
	if (DirectionArrow)
	{
		DirectionArrow->SetupAttachment(this);
		DirectionArrow->SetIsVisualizationComponent(true);
		DirectionArrow->SetArrowColor(FLinearColor(1.0f, 0.85f, 0.1f));
		DirectionArrow->bIsScreenSizeScaled = true;
		DirectionArrow->SetHiddenInGame(true);
	}
#endif
}

UWxPatrolComponent* UWxPatrolComponent::FindPatrolComponent(const AController* Controller)
{
	// 정찰 경로는 항상 스폰 주체(AWxSpawner)에 붙고, 그 스폰 주체는 AWxEnemyController 가 자기 Owner 로 물고 있다.
	const AActor* Spawner = Controller ? Controller->GetOwner() : nullptr;

	return Spawner ? Spawner->FindComponentByClass<UWxPatrolComponent>() : nullptr;
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

	ConfigureSpline();
}
#endif

void UWxPatrolComponent::OnRegister()
{
	Super::OnRegister();

	ConfigureSpline();
}

void UWxPatrolComponent::ConfigureSpline()
{
	SetClosedLoop(MoveMode == EWxPatrolMoveMode::Loop, /*bUpdateSpline*/ false);

	// 정찰은 지점 사이를 내비게이션으로 직선 이동하므로, 곡선 보간을 끄고 포인트를 직선으로 잇는다(에디터 표시도 실제 경로와 일치).
	const int32 NumPoints = GetNumberOfSplinePoints();
	for (int32 Index = 0; Index < NumPoints; ++Index)
	{
		SetSplinePointType(Index, ESplinePointType::Linear, /*bUpdateSpline*/ false);
	}

	UpdateSpline();

#if WITH_EDITORONLY_DATA
	// 모드와 무관하게 정찰은 0번 → 1번 지점으로 시작하므로, 그 방향을 화살표로 표시한다.
	if (DirectionArrow && NumPoints >= 2)
	{
		const FVector Start = GetLocationAtSplinePoint(0, ESplineCoordinateSpace::World);
		const FVector Next = GetLocationAtSplinePoint(1, ESplineCoordinateSpace::World);
		DirectionArrow->SetWorldLocationAndRotation(Start, (Next - Start).Rotation());
	}
#endif
}
