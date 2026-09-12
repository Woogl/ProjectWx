// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "WxNameplateComponent.generated.h"

class APawn;
class UMVVMView;
class UWxViewModel_Character;

/** 캐릭터 VM을 주입하고 거리 기반 표시와 스케일을 적용한다. 태그 가시성은 WBP 바인딩이 담당한다. */
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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
	void BindViewModel();
	void ReleaseViewModel();
	void UpdatePresentation(const APawn* ViewerPawn);

	UPROPERTY(Transient)
	TObjectPtr<UWxViewModel_Character> CharacterViewModel;

	TWeakObjectPtr<UMVVMView> BoundView;
	FName BoundSourceName;
	bool bBindingErrorReported = false;
};
