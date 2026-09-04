// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "WxPatrolComponent.generated.h"

class APawn;
class UArrowComponent;

UENUM(BlueprintType)
enum class EWxPatrolMoveMode : uint8
{
	PingPong,

	Loop,

	Once
};

/**
 * 스플라인 포인트들이 정찰 지점이 된다.
 *
 * 순수 경로 데이터( + MoveMode 순회 규칙)만 제공하고 상태를 갖지 않는다.
 * 진행 커서는 BT 태스크(UWxBTTask_Patrol)가 폰별로 소유하므로, 같은 경로를 여러 폰이 재사용하거나 적이 리스폰돼도 안전하다.
 *
 * 적이 부착된 액터에 추가한다 — 스포너로 스폰된 적은 그 스포너에, 직접 배치한 적은 레벨에서 부착해 둔 액터에 붙은 경로를 따른다.
 * 경로를 든 액터가 런타임에 움직이는 경우는 지원하지 않는다(정찰 지점이 함께 끌려간다).
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXAI_API UWxPatrolComponent : public USplineComponent
{
	GENERATED_BODY()

public:
	UWxPatrolComponent();

	/** Pawn 이 따를 정찰 경로. 부착 부모의 것을 쓰며, 없으면 그 적은 정찰하지 않는다. */
	static UWxPatrolComponent* FindPatrolComponent(const APawn* Pawn);

	int32 GetNumPoints() const;

	/** 중단된 정찰을 어디서 재개할지는 순회 규칙에 달렸으므로, 호출자가 모드를 직접 본다. */
	EWxPatrolMoveMode GetMoveMode() const;

	/** Index 정찰 지점의 월드 좌표. */
	FVector GetPointLocation(int32 Index) const;

	/**
	 * MoveMode 규칙으로 CurrentIndex 의 다음 정찰 지점을 계산한다.
	 * 계속 정찰할 수 있으면 OutNextIndex 를 채우고 true 를, Once 로 마지막 지점에 도달했거나 진행할 지점이 없으면 false 를 반환한다.
	 * InOutDirection 은 PingPong 진행 방향(+1/-1)으로, 호출자가 폰별로 보관하며 본 함수가 끝점에서 뒤집는다.
	 */
	bool GetNextIndex(int32 CurrentIndex, int32& InOutDirection, int32& OutNextIndex) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void OnRegister() override;

	UPROPERTY(EditAnywhere, Category = "Wx")
	EWxPatrolMoveMode MoveMode = EWxPatrolMoveMode::PingPong;

private:
	void ConfigureSpline();

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UArrowComponent> DirectionArrow;
#endif
};
