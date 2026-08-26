// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ControllerComponent.h"
#include "WxIndicatorManagerComponent.generated.h"

class USceneComponent;
class UWxIndicatorDescriptor;

DECLARE_MULTICAST_DELEGATE(FWxOnIndicatorsUpdated);
DECLARE_MULTICAST_DELEGATE_OneParam(FWxOnIndicatorManagerReady, UWxIndicatorManagerComponent* /*Manager*/);

/**
 * 로컬 플레이어가 화면에 띄울 인디케이터 목록을 들고, 매 틱 그 화면 좌표를 계산해 발행하는 컨트롤러 컴포넌트.
 * 표시는 보는 사람마다 다른 로컬 사건이라 월드가 아니라 컨트롤러에 매달리며, 화면 좌표의 원천인 카메라·뷰포트도 여기서만 볼 수 있다.
 * 어떤 위젯이 그리는지는 알지 않는다 — 뷰모델이 갱신 통지를 받아 표시 필드로 옮기고, 위젯은 HUD 에 배치돼 그 필드에 바인딩한다.
 *
 * 부착은 코드가 아니라 GameMode 가 고른 Experience 에셋의 주입 목록으로 한다(컨트롤러는 본 클래스를 모른다).
 * 목록에는 사이드 구분이 없으므로 원격 사본(데디 서버가 들고 있는 PC)에서 등록을 거부하는 것은 본 클래스의 책임이다.
 */
UCLASS()
class WXUI_API UWxIndicatorManagerComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UWxIndicatorManagerComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * 오너가 로컬 컨트롤러가 아니면 표시될 수 없으므로 발급하지 않는다(null 반환).
	 * 컴포넌트가 비었거나 언로드·파괴로 사라지면 좌표를 대신 가리킨다 — 컴포넌트를 줄 때도 좌표를 함께 받는 이유다.
	 */
	UWxIndicatorDescriptor* AddIndicator(USceneComponent* InTargetComponent, const FVector& InWorldLocation, const FVector& InWorldOffset);

	/** 목록에 없는 등록증은 무시한다. */
	void RemoveIndicator(UWxIndicatorDescriptor* Indicator);

	const TArray<TObjectPtr<UWxIndicatorDescriptor>>& GetIndicators() const;

	/**
	 * 등록증의 투영 결과가 갱신되었다는 통지. 매 틱과 등록·해제 시점에 발행된다.
	 * 등록이 비면 틱이 멈추므로, 마지막 해제에서도 한 번 발행해 표시 측이 숨김으로 수렴하게 한다.
	 */
	FWxOnIndicatorsUpdated OnIndicatorsUpdated;

	/**
	 * 매니저가 쓸 수 있게 될 때마다 발행된다.
	 * 관찰자(HUD 뷰모델)가 매니저보다 먼저 존재할 수 있어 인스턴스가 아니라 클래스 차원에 둔다 — 구독자는 소유 액터로 자기 것인지 가린다.
	 */
	static FWxOnIndicatorManagerReady OnAnyManagerReady;

private:
	/** 뷰를 얻지 못하면(월드 전환 등) 전부 투영 없음으로 되돌린다. */
	void UpdateProjections();

	void UpdateTickEnabled();

	UPROPERTY()
	TArray<TObjectPtr<UWxIndicatorDescriptor>> Indicators;
};
