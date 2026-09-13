// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "WxNameplateComponent.generated.h"

class APawn;

/** 공유 Character VM을 위젯에 연결하고 거리 기반 표시와 스케일을 처리한다. 태그 가시성은 WBP의 MVVM이 처리한다. */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXUI_API UWxNameplateComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UWxNameplateComponent();

	virtual void InitWidget() override;
	virtual void SetWidget(UUserWidget* InWidget) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0", Units = "cm"))
	float ReferenceDistance = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0"))
	float MinScale = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0"))
	float MaxScale = 1.f;

	/** 0이면 거리와 관계없이 숨긴다. */
	UPROPERTY(EditAnywhere, Category = "Wx", meta = (ClampMin = "0", Units = "cm"))
	float MaxVisibilityDistance = 3000.f;

private:
	friend class FWxNameplatePresentationTest;

	void BindViewModel();
	void UpdatePresentation(const APawn* ViewerPawn);
};
