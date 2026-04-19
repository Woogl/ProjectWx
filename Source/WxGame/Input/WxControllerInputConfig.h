// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "WxControllerInputConfig.generated.h"

class UInputMappingContext;
class UInputAction;
class UWxActivatableWidget;

/** 입력으로 열 수 있는 메뉴 위젯 하나의 선언 (InputAction → LayerTag에 WidgetClass 푸시) */
USTRUCT(BlueprintType)
struct FWxMenuInputBinding
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftClassPtr<UWxActivatableWidget> WidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "UI.Layer"))
	FGameplayTag LayerTag;
};

/**
 * 플레이어 컨트롤러용 입력 설정.
 * 메뉴 토글 등 UI 레벨 입력만 포함.
 */
UCLASS()
class WXGAME_API UWxControllerInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 플레이어 IMC. BeginPlay에서 Subsystem에 추가됨 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TObjectPtr<UInputMappingContext> MappingContext;

	/** 메뉴 입력 바인딩 설정. 키 입력 시 지정 레이어에 위젯 푸시. 새 메뉴 추가 시 여기에 한 항목만 추가 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wx|Input")
	TArray<FWxMenuInputBinding> MenuInputBindings;
};
