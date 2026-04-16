// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "WxPromptWidgetComponent.generated.h"

/**
 * 상호작용 프롬프트 전용 위젯 컴포넌트.
 * Screen Space, Desired Size 렌더링, 충돌 없음, 초기 비가시 상태를 기본값으로 설정한다.
 * 위젯 클래스가 IWxPromptWidgetInterface를 구현하면 PromptText를 자동 전달한다.
 */
UCLASS(ClassGroup = "Wx", meta = (BlueprintSpawnableComponent))
class WXWORLD_API UWxPromptWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UWxPromptWidgetComponent();

	/** 프롬프트 텍스트를 갱신한다. 위젯이 이미 생성되어 있으면 즉시 반영한다. */
	void SetPromptText(const FText& InText);

protected:
	virtual void InitWidget() override;

	UPROPERTY(EditAnywhere, Category = "Wx", meta = (MultiLine = true))
	FText PromptText;

private:
	void ApplyPromptTextToWidget();
};
