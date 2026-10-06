// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "WxMVVMToolset.generated.h"

class UMVVMBlueprintView;
class UWidgetBlueprint;
struct FMVVMBlueprintPropertyPath;

/**
 * 기존 MCP 표면(ObjectTools 등)이 닿지 못하는 지점만 뚫는다 — MVVM 바인딩 경로·변환 함수·삭제와 이벤트 행·목적지·인자.
 * 변환 객체의 함수·인자 경로와 이벤트 경로는 편집 플래그가 없어 set_properties 로 쓸 수 없고, 래퍼 그래프도 에디터 서브시스템이 만들어야 한다.
 */
UCLASS(BlueprintType, Hidden)
class UWxMVVMToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * MVVM 이벤트 행을 새로 만들고 소스 델리게이트를 지정한다. 목적지는 SetEventDestination, 목적지 함수 인자는 SetEventArgumentPath 로 잇는다.
	 * 엔진 이벤트 바인딩은 델리게이트 인자를 목적지로 넘기지 못한다. 목적지에 넘길 값은 뷰모델·위젯 프로퍼티로 두고 인자 경로로 읽게 한다.
	 * 이벤트는 뷰 초기화 때 실행되지 않으므로, 획득 알림처럼 한 번만 반응해야 하는 신호에 상태 바인딩 대신 쓴다.
	 * @param EventPath "뷰모델이름.델리게이트" 또는 "위젯이름.델리게이트". 끝이 BlueprintAssignable 멀티캐스트 델리게이트여야 한다.
	 * @return 새 이벤트의 events 배열 인덱스. 실패하면 -1.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static int32 AddEvent(UWidgetBlueprint* WidgetBlueprint, const FString& EventPath);

	/**
	 * MVVM 이벤트의 목적지를 호출 가능한 함수로 바꾼다. 래퍼 그래프도 에디터 서브시스템에서 갱신한다.
	 * @param EventIndex MVVMBlueprintView 의 events 배열 인덱스.
	 * @param DestinationPath "Self.함수", "뷰모델이름.함수" 또는 "위젯이름.함수"(예: "AcquiredItemList.AddItem").
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool SetEventDestination(UWidgetBlueprint* WidgetBlueprint, int32 EventIndex, const FString& DestinationPath);

	/**
	 * MVVM 이벤트 목적지 함수의 입력 인자 하나를 프로퍼티 경로에 잇는다. 이벤트가 발화할 때 그 값을 읽어 넘긴다. 목적지를 먼저 정해 둔다.
	 * @param ArgumentName 목적지 함수의 입력 파라미터 이름(예: ListView.AddItem 의 "Item").
	 * @param SourcePath "뷰모델이름.필드[.필드...]", "위젯이름.필드" 또는 "Self.필드".
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool SetEventArgumentPath(UWidgetBlueprint* WidgetBlueprint, int32 EventIndex, FName ArgumentName, const FString& SourcePath);

	/**
	 * 바인딩의 Source→Destination 변환 함수를 지정하고 인자마다 소스 경로를 연결한다. 기존 소스 경로는 변환 함수로 대체된다.
	 * @param BindingId MVVMBlueprintView.bindings[].bindingId (예: "775DFD51-4984-B2EC-0E09-9683AF2F123C").
	 * @param FunctionPath BlueprintFunctionLibrary 의 정적 BlueprintPure 함수 또는 위젯 자신의 Pure·const 함수 경로. 예: "/Script/Engine.KismetTextLibrary:Conv_IntToText"
	 * @param ArgumentsJson {"입력파라미터이름":"경로", ...}. 경로는 "뷰모델이름.필드[.필드...]" 또는 "Self.필드[.필드...]".
	 *   예: {"Value":"WxViewModel_Item.TotalCount"}
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool SetBindingConversionFunction(UWidgetBlueprint* WidgetBlueprint, const FString& BindingId, const FString& FunctionPath, const FString& ArgumentsJson);

	/**
	 * 바인딩의 소스 경로나 Source→Destination 변환 함수 인자 하나의 경로만 바꾼다. 나머지 인자의 경로·기본값은 유지된다.
	 * @param ArgumentName 비우면 바인딩 자체의 소스 경로다. 이때 엔진 에디터와 같이 기존 변환 함수는 제거된다.
	 * @param SourcePath "뷰모델이름.필드[.필드...]" 또는 "Self.필드[.필드...]".
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool SetBindingSourcePath(UWidgetBlueprint* WidgetBlueprint, const FString& BindingId, FName ArgumentName, const FString& SourcePath);

	/**
	 * 바인딩 행 하나를 에디터 서브시스템으로 지운다(바인딩 패널의 삭제와 같다).
	 * @param BindingId MVVMBlueprintView.bindings[].bindingId.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool RemoveBinding(UWidgetBlueprint* WidgetBlueprint, const FString& BindingId);

private:
	/** "소스.필드[.필드...]" 를 뷰 기준 경로로 해석한다. 소스는 Self, 뷰모델 이름, 위젯 이름 순으로 찾는다. 실패하면 스크립트 오류를 올린다. */
	static bool ResolvePropertyPath(const UWidgetBlueprint* WidgetBlueprint, const UMVVMBlueprintView* View, const FString& PathString, FMVVMBlueprintPropertyPath& OutPath);
};
