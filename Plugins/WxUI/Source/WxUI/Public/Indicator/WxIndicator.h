// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WxIndicator.generated.h"

class UWidgetComponent;
class UWxViewModel_Indicator;

/**
 * 대상을 가리키는 화면 인디케이터 하나를 들고 다니는 액터.
 * 대상이 화면 밖이거나 카메라 뒤면 여백을 둔 화면 가장자리로 당겨 방향을 가리킨다.
 *
 * 대상 액터에 위젯 컴포넌트를 직접 붙이지 않고 독립 액터로 두는 이유는, 대상이 언로드된 동안에도 기록 좌표를 계속 가리켜야 하기 때문이다.
 * 대상이 로드돼 있는 동안만 그 액터에 부착해 따라다닌다.
 *
 * 무엇을 어떻게 그릴지(위젯 클래스·아이콘)는 전부 BP 서브클래스의 위젯 컴포넌트에 저작한다 — 본 클래스는 위치만 책임진다.
 * 보는 사람마다 다른 로컬 표시라 복제하지 않는다. 띄우는 쪽이 표시할 머신에서 스폰한다.
 */
UCLASS()
class WXUI_API AWxIndicator : public AActor
{
	GENERATED_BODY()

public:
	AWxIndicator();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 스폰 직후 1회. 앵커는 스폰 위치이며, 여기서 받은 높이만큼 위를 가리킨다. */
	void Initialize(float InZOffset);

	/** 대상에 부착해 따라다니게 한다. 유효한 대상만 넘긴다 — 부착 해제는 대상이 사라질 때 엔진이 대신 한다. */
	void SetTarget(AActor* InTarget);

	/** 부착 대상이 살아 있는지. 띄운 쪽이 대상을 다시 해석할 때가 됐는지 판단하는 데 쓴다. */
	bool HasTarget() const;

private:
	/** 위젯이 만들어져 있으면 뷰모델을 묶는다. 데디 서버·Slate 없는 실행에서는 위젯이 없는 게 정상이라 조용히 지나간다. */
	void BindViewModel();

	/** 뷰를 얻지 못하면(월드 전환 등) 위젯을 숨긴다. */
	void UpdateProjection();

	/** 당겼으면 true — 위젯이 화면 밖임을 알 수 있다. */
	bool ClampToScreenEdge(const FVector2D& ClampRectMin, const FVector2D& ClampRectMax, FVector2D& InOutPixelPosition) const;

	/** 루트가 곧 표시 위치라 매 틱 덮어써진다. 대상 위치는 여기가 아니라 AnchorLocation 이 쥔다. */
	UPROPERTY(VisibleAnywhere, Category = "Wx|Indicator")
	TObjectPtr<UWidgetComponent> IndicatorWidget;

	/**
	 * 클램프된 인디케이터와 화면 가장자리 사이에 둘 여백(슬레이트 단위).
	 * 위젯의 중심 좌표를 당기므로 "아이콘 절반 + 여백" 만큼이 필요하다 — 아이콘을 크게 바꾸면 이 값도 함께 키운다.
	 */
	UPROPERTY(EditAnywhere, Category = "Wx|Indicator")
	float ScreenMargin = 48.f;

	/** 대상 발밑이 아니라 머리 위를 가리키게 한다. */
	float ZOffset = 0.f;

	/** 부착이 풀린 동안 가리킬 좌표. 부착 중에는 대상 위치로 매 틱 갱신된다. */
	FVector AnchorLocation = FVector::ZeroVector;

	UPROPERTY()
	TObjectPtr<UWxViewModel_Indicator> ViewModel;
};
