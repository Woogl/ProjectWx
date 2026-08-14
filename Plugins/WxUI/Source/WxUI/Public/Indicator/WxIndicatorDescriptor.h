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
 * 발급 시 정해진 대상·오프셋은 이후 바뀌지 않는다. 화면 좌표·거리·클램프 여부는 매 틱 대상에서 다시 계산되는 값이라
 * 매니저가 여기에 덮어 쓰며, 뷰모델은 그 값을 읽어 표시 필드로 발행한다.
 */
UCLASS()
class WXUI_API UWxIndicatorDescriptor : public UObject
{
	GENERATED_BODY()

public:
	/** 매니저가 발급 직후 1회 호출한다. */
	void Initialize(UWxIndicatorManagerComponent* InOwningManager, USceneComponent* InTargetComponent, const FVector& InWorldOffset);

	USceneComponent* GetTargetComponent() const;

	FVector GetWorldOffset() const;

	/** 매니저가 매 틱 투영 결과를 기록한다. */
	void SetProjection(const FVector2D& InScreenPosition, float InDistanceMeters, bool bInClamped);

	/** 대상이 무효하거나 투영을 얻지 못했음을 기록한다. 표시 측은 이 상태를 숨김으로 읽는다. */
	void ClearProjection();

	bool IsProjected() const;

	FVector2D GetScreenPosition() const;

	float GetDistanceMeters() const;

	bool IsClamped() const;

	/** 발급한 매니저에서 스스로 빠진다. 등록한 쪽이 매니저를 다시 찾지 않아도 되게 한다. */
	void Unregister();

private:
	/** 이 컴포넌트의 월드 위치가 투영 원점이다. */
	UPROPERTY()
	TObjectPtr<USceneComponent> TargetComponent;

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

	/** 이번 틱에 투영이 성립했는지. */
	bool bProjected = false;
};
