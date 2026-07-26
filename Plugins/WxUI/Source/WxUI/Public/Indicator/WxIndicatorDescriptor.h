// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WxIndicatorDescriptor.generated.h"

class UWxIndicatorManagerComponent;
class UWxIndicatorWidget;

/**
 * 화면 인디케이터 1개의 등록증.
 * 매니저가 발급하고 캔버스가 소비한다 — 등록한 쪽은 이걸 들고 있다가 해제할 때만 쓴다.
 *
 * 발급 시 정해진 대상·오프셋·위젯 클래스는 이후 바뀌지 않는다. 표시 위치는 상태가 아니라 매 프레임 대상에서 다시 계산되는 값이라 여기 남지 않으며,
 * 캔버스가 쓰는 슬롯·위젯 같은 표시 측 기록도 캔버스가 자기 슬롯에 들고 있어 등록증은 순수 데이터로 남는다.
 */
UCLASS()
class WXUI_API UWxIndicatorDescriptor : public UObject
{
	GENERATED_BODY()

public:
	/** 매니저가 발급 직후 1회 호출한다. */
	void Initialize(UWxIndicatorManagerComponent* InOwningManager, USceneComponent* InTargetComponent, const TSoftClassPtr<UWxIndicatorWidget>& InIndicatorWidgetClass, const FVector& InWorldOffset);

	USceneComponent* GetTargetComponent() const { return TargetComponent; }

	FVector GetWorldOffset() const { return WorldOffset; }

	TSoftClassPtr<UWxIndicatorWidget> GetIndicatorWidgetClass() const { return IndicatorWidgetClass; }

	/** 대상이 아직 살아있는지. 대상이 파괴되면 캔버스가 표시를 접는다(등록증 제거는 등록한 쪽의 몫이다). */
	bool HasValidTarget() const { return IsValid(TargetComponent); }

	/** 발급한 매니저에서 스스로 빠진다. 등록한 쪽이 매니저를 다시 찾지 않아도 되게 한다. */
	void Unregister();

private:
	/** 가리킬 대상. 이 컴포넌트의 월드 위치가 투영 원점이다. */
	UPROPERTY()
	TObjectPtr<USceneComponent> TargetComponent;

	/** 투영 원점에 더할 월드 오프셋. 대상 발밑이 아니라 머리 위를 가리키게 한다. */
	UPROPERTY()
	FVector WorldOffset = FVector::ZeroVector;

	/** 화면에 띄울 위젯 클래스. 캔버스가 비동기 로드해 풀에서 인스턴스를 얻는다. */
	UPROPERTY()
	TSoftClassPtr<UWxIndicatorWidget> IndicatorWidgetClass;

	UPROPERTY()
	TWeakObjectPtr<UWxIndicatorManagerComponent> OwningManager;
};
