// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVM/WxViewModel.h"
#include "View/MVVMViewModelContextResolver.h"

#include "WxViewModel_Indicator.generated.h"

class APlayerController;
class UMVVMView;
class UUserWidget;
class UWxIndicatorDescriptor;
class UWxIndicatorManagerComponent;

/**
 * 화면 인디케이터 하나의 표시를 노출하는 뷰모델.
 * 위젯은 자기가 화면 어디에 놓이는지 계산하지 않는다 — 매니저가 계산한 값을 그대로 받아 위치·표시·외형 바인딩으로 소비한다.
 *
 * 인디케이터 위젯은 HUD 에 배치돼 상시 존재하고 bHasIndicator 로 표시만 갈린다.
 * 매니저는 Experience 주입이라 위젯보다 늦게 도착할 수 있고, 리졸버가 돌려준 인스턴스는 뷰가 교체할 수 없다.
 * 그래서 인스턴스는 고정한 채 도착 신호를 받아 내부 상태(Initialize)만 갈아끼운다 — UWxViewModel_InteractionList 와 같은 구조다.
 */
UCLASS()
class WXUI_API UWxViewModel_Indicator : public UWxViewModel
{
	GENERATED_BODY()

public:
	/** 매니저가 이미 붙어 있으면 즉시 연결하고, 아니면 도착 신호를 기다린다. */
	void StartObserving(APlayerController* PC, int32 InIndicatorIndex);

	void Initialize(UWxIndicatorManagerComponent* InManager);

	virtual void Deinitialize() override;
	virtual void BeginDestroy() override;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Indicator")
	bool bHasIndicator = false;

	/** 전체화면 캔버스에 정렬 (0.5, 0.5) 로 둔 위젯의 렌더 트랜슬레이션에 바인딩한다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Indicator")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	/** 카메라에서 대상까지의 거리(m). 1m 단위로만 갱신된다. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Indicator")
	float DistanceMeters = 0.f;

	/** 대상이 화면 밖이라 가장자리에 붙었는지. 화면 밖 외형(화살표 등) 전환용. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Wx|Indicator")
	bool bClamped = false;

private:
	void HandleManagerReady(UWxIndicatorManagerComponent* Manager);

	void HandleIndicatorsUpdated();

	/** 도착 신호 구독을 해제한다. 연결 성공 시와 소멸 시 모두 여기로 모은다. */
	void StopObserving();

	/** 등록증(없으면 null)의 값을 표시 필드로 옮긴다. */
	void ApplyIndicator(const UWxIndicatorDescriptor* Indicator);

	TWeakObjectPtr<APlayerController> ObservedController;

	FDelegateHandle ManagerReadyHandle;

	TWeakObjectPtr<UWxIndicatorManagerComponent> CachedManager;

	/** 이 뷰모델이 표시할 등록 순번. 리졸버가 주입한다. */
	int32 IndicatorIndex = 0;
};

/**
 * VM_Indicator 용 View Bindings Resolver.
 * WBP 의 View Bindings 에서 Creation Type = Resolver 로 본 클래스를 선택한다.
 */
UCLASS(EditInlineNew, CollapseCategories)
class WXUI_API UWxViewModelResolver_Indicator : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()

public:
	virtual UObject* CreateInstance(const UClass* ExpectedType, const UUserWidget* UserWidget, const UMVVMView* View) const override;

	/** 이 위젯이 표시할 인디케이터의 등록 순번. */
	UPROPERTY(EditAnywhere, Category = "Wx|Indicator")
	int32 IndicatorIndex = 0;
};
