// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WxIndicatorDescriptor.generated.h"

class USceneComponent;
class UWxIndicatorManagerComponent;

/**
 * 화면 인디케이터 1개의 등록증.
 * 매니저가 발급하고 뷰모델이 소비한다 — 등록한 쪽은 이걸 들고 있다가 해제할 때만 쓴다.
 *
 * 발급 시 정해진 대상·오프셋은 이후 바뀌지 않고, 화면 좌표·거리·클램프 여부만 매 틱 매니저가 덮어 쓴다.
 */
UCLASS()
class WXUI_API UWxIndicatorDescriptor : public UObject
{
	GENERATED_BODY()

public:
	/** 매니저가 발급 직후 1회 호출한다. */
	void Initialize(UWxIndicatorManagerComponent* InOwningManager, USceneComponent* InTargetComponent, const FVector& InWorldLocation, const FVector& InWorldOffset);

	/** 추종 대상이 없거나 이미 사라졌으면 nullptr — 그때는 폴백 좌표가 투영 원점이다. */
	USceneComponent* GetTargetComponent() const;

	FVector GetWorldLocation() const;

	FVector GetWorldOffset() const;

	void SetProjection(const FVector2D& InScreenPosition, float InDistanceMeters, bool bInClamped);

	/** 뷰를 얻지 못해 투영할 수 없음을 기록한다. 표시 측은 이 상태를 숨김으로 읽는다. */
	void ClearProjection();

	bool IsProjected() const;

	FVector2D GetScreenPosition() const;

	float GetDistanceMeters() const;

	bool IsClamped() const;

	/** 등록한 쪽이 매니저를 다시 찾지 않아도 되게, 발급한 매니저에서 스스로 빠진다. */
	void Unregister();

private:
	/** 이 컴포넌트의 월드 위치가 투영 원점이다. 대상을 소유하지 않으므로 약참조이고, 언로드·파괴되면 비어 아래 좌표로 내려앉는다. */
	UPROPERTY()
	TWeakObjectPtr<USceneComponent> TargetComponent;

	/** 추종 대상이 없거나 사라진 동안의 투영 원점. 컴포넌트를 따라다니는 등록증에도 항상 채워 둔다. */
	UPROPERTY()
	FVector WorldLocation = FVector::ZeroVector;

	/** 투영 원점에 더할 월드 오프셋. 대상 발밑이 아니라 머리 위를 가리키게 한다. */
	UPROPERTY()
	FVector WorldOffset = FVector::ZeroVector;

	UPROPERTY()
	TWeakObjectPtr<UWxIndicatorManagerComponent> OwningManager;

	/** 화면 좌표(뷰포트 로컬 단위). 화면 밖 대상은 가장자리로 당겨진 좌표가 들어온다. */
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	/** 카메라에서 대상까지의 거리. 매 프레임 표시가 떨리지 않도록 1m 단위로 반올림해 들어온다. */
	float DistanceMeters = 0.f;

	/** 대상이 화면 밖이라 가장자리에 붙었는지. */
	bool bClamped = false;

	bool bProjected = false;
};
