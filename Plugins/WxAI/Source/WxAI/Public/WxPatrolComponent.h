// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "WxPatrolComponent.generated.h"

class AActor;
class UArrowComponent;

/** 정찰 경로의 순회 방식. */
UENUM(BlueprintType)
enum class EWxPatrolMoveMode : uint8
{
	/** 끝에 도달하면 진행 방향을 뒤집어 되짚어 온다(왕복). */
	PingPong,

	/** 마지막 지점에 도달하면 첫 지점으로 돌아가 순환한다. */
	Loop,

	/** 마지막 지점에 도달하면 정찰을 종료한다. */
	Once
};

/**
 * 정찰 경로를 정의하는 스플라인 컴포넌트. 스플라인 포인트들이 정찰 지점이 된다.
 *
 * 순수 경로 데이터( + MoveMode 순회 규칙)만 제공하고 상태를 갖지 않는다.
 * 진행 커서는 BT 태스크(UWxBTTask_Patrol)가 폰별로 소유하므로, 같은 경로를 여러 폰이 재사용하거나 적이 리스폰돼도 안전하다.
 * AWxSpawner 인스턴스에 추가하면, 스폰된 적이 FindPatrolComponent(액터) 로 본 컴포넌트를 찾아 이 경로를 따라 정찰한다.
 */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXAI_API UWxPatrolComponent : public USplineComponent
{
	GENERATED_BODY()

public:
	UWxPatrolComponent();

	/** 액터의 Owner(또는 부착 부모)에 붙은 정찰 컴포넌트를 찾는다. 없으면 null. 스포너가 스폰 시 Owner 로 자신을 지정한다. */
	static UWxPatrolComponent* FindPatrolComponent(const AActor* Actor);

	/** 정찰 지점 개수. */
	int32 GetNumPoints() const;

	/** Index 정찰 지점의 월드 좌표. */
	FVector GetPointLocation(int32 Index) const;

	/**
	 * MoveMode 규칙으로 CurrentIndex 의 다음 정찰 지점을 계산한다.
	 * 계속 정찰할 수 있으면 OutNextIndex 를 채우고 true 를, Once 로 마지막 지점에 도달했거나 진행할 지점이 없으면 false 를 반환한다.
	 * InOutDirection 은 PingPong 진행 방향(+1/-1)으로, 호출자가 폰별로 보관하며 본 함수가 끝점에서 뒤집는다.
	 */
	bool GetNextIndex(int32 CurrentIndex, int32& InOutDirection, int32& OutNextIndex) const;

#if WITH_EDITOR
	/** 에디터에서 MoveMode 를 바꾸면 스플라인 닫힘 상태도 즉시 따라가게 한다. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	/** 등록 시 MoveMode 에 맞춘 스플라인 설정을 적용한다. */
	virtual void OnRegister() override;

	/** 정찰 지점 순회 방식. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	EWxPatrolMoveMode MoveMode = EWxPatrolMoveMode::PingPong;

private:
	/** MoveMode 에 맞춰 스플라인 닫힘 상태를 맞추고, 모든 포인트를 직선으로 이어 곡선 보간을 제거한다. 에디터에선 최초 진행 방향 화살표도 갱신한다. */
	void ConfigureSpline();

#if WITH_EDITORONLY_DATA
	/** 에디터에서 최초 정찰 진행 방향(0→1 지점)을 보여주는 화살표. */
	UPROPERTY()
	TObjectPtr<UArrowComponent> DirectionArrow;
#endif
};
