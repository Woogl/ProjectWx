// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "WxNameplateComponent.generated.h"

class UAbilitySystemComponent;

/** 카메라 거리에 따라 위젯 스케일을 자동 조절하여 원근 효과를 적용한다. */
UCLASS(ClassGroup = (Wx), meta = (BlueprintSpawnableComponent))
class WXUI_API UWxNameplateComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UWxNameplateComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	 * 주입받은 표시 데이터와 ASC 를 묶은 UWxViewModel_Character 를 생성해 MVVM View 에 바인딩한다.
	 * WxUI 는 구체 캐릭터 타입을 알지 못하므로 표시 데이터는 소비 측이 주입한다.
	 *
	 * InASC 는 필수다 — 없으면 경고만 남기고 아무것도 하지 않는다.
	 * 데디케이티드 서버나 Slate 없는 실행에서는 표시할 위젯 자체가 없으므로 조용히 무시한다.
	 */
	void InitializeViewModels(UAbilitySystemComponent* InASC, const FText& InCharacterName, const TSoftObjectPtr<UObject>& InPortrait);

protected:
	/** 이 거리에서 위젯 스케일이 1.0이 된다. */
	UPROPERTY(EditAnywhere, Category = "Wx")
	float ReferenceDistance = 1000.f;

	UPROPERTY(EditAnywhere, Category = "Wx")
	float MinScale = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Wx")
	float MaxScale = 1.f;

private:
	/**
	 * 값이 같아도 SetRenderScale 은 위젯을 무효화하므로 변화가 있을 때만 부른다.
	 * 음수 초기값은 "아직 한 번도 적용하지 않음"을 뜻해 첫 틱이 반드시 반영되게 한다.
	 */
	float LastRenderScale = -1.f;
};
