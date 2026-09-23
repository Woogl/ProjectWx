// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "WxMVVMToolset.generated.h"

class UWidgetBlueprint;

/**
 * 기존 MCP 표면(ObjectTools 등)이 닿지 못하는 지점만 뚫는다 — MVVM 바인딩 변환 함수와 이벤트 목적지.
 * 변환 객체의 함수·인자 경로는 편집 플래그가 없어 set_properties 로 쓸 수 없고, 래퍼 그래프도 에디터 서브시스템이 만들어야 한다.
 */
UCLASS(BlueprintType, Hidden)
class UWxMVVMToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/** MVVM 이벤트의 목적지를 위젯 자신의 함수로 바꾼다. 래퍼 그래프도 에디터 서브시스템에서 갱신한다. */
	UFUNCTION(BlueprintCallable, meta = (AICallable), Category = "Wx")
	static bool SetEventDestinationWidgetFunction(UWidgetBlueprint* WidgetBlueprint, int32 EventIndex, FName FunctionName);

	/**
	 * 바인딩의 Source→Destination 변환 함수를 지정하고 인자마다 소스 경로를 연결한다. 기존 소스 경로는 변환 함수로 대체된다.
	 * @param BindingId MVVMBlueprintView.bindings[].bindingId (예: "775DFD51-4984-B2EC-0E09-9683AF2F123C").
	 * @param FunctionPath BlueprintFunctionLibrary 의 정적 BlueprintPure 함수 또는 위젯 자신의 Pure·const 함수 경로. 예: "/Script/Engine.KismetTextLibrary:Conv_IntToText"
	 * @param ArgumentsJson {"입력파라미터이름":"경로", ...}. 경로는 "뷰모델이름.필드[.필드...]" 또는 "Self.필드[.필드...]".
	 *   예: {"Value":"WxViewModel_Item.TotalCount"}
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool SetBindingConversionFunction(UWidgetBlueprint* WidgetBlueprint, const FString& BindingId, const FString& FunctionPath, const FString& ArgumentsJson);
};
