// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "WxPatrolComponent.generated.h"

/** 정찰 경로의 순회 방식. */
UENUM(BlueprintType)
enum class EWxPatrolMoveMode : uint8
{
	/** 마지막 지점에 도달하면 첫 지점으로 돌아가 순환한다. */
	Loop,

	/** 끝에 도달하면 진행 방향을 뒤집어 되짚어 온다(왕복). */
	PingPong,

	/** 마지막 지점에 도달하면 정찰을 종료한다. */
	Once
};

/**
 * 정찰 경로를 정의하는 스플라인 컴포넌트. 스플라인 포인트들이 정찰 지점이 된다.
 *
 * AWxSpawner 인스턴스에 추가하면, 스폰된 적이 빙의 시점에 이 컴포넌트를 찾아 경로를 따라 정찰한다.
 * 컨트롤러가 빙의 시 포인트를 월드 좌표 배열로 캐시하므로, 본 컴포넌트가 이후 제거되어도 진행 중인 정찰엔 영향이 없다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXWORLD_API UWxPatrolComponent : public USplineComponent
{
	GENERATED_BODY()

public:
	/** 스플라인 포인트들의 월드 좌표 목록. 컨트롤러가 빙의 시 한 번 캐시한다. */
	TArray<FVector> GetWorldPoints() const;

	EWxPatrolMoveMode GetMoveMode() const;

protected:
	/** 정찰 지점 순회 방식. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	EWxPatrolMoveMode MoveMode = EWxPatrolMoveMode::Loop;
};
