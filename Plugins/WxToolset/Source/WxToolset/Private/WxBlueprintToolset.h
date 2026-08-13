// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "WxBlueprintToolset.generated.h"

class UBlueprint;

/**
 * 블루프린트 저작 MCP 도구 모음.
 * 기존 MCP 표면(BlueprintTools 등)이 닿지 못하는 지점만 뚫는다 — 변수 메타.
 */
UCLASS(BlueprintType, Hidden)
class UWxBlueprintToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * 블루프린트 변수에 메타를 기입한다. 넘긴 키만 덮어쓰므로 카테고리·툴팁 같은 기존 메타는 남는다.
	 * 변수 서술자에 저장되며 컴파일 때 프로퍼티로 전달된다 — 생성자에서 프로퍼티에 직접 넣는 방식은 컴파일마다 지워지므로 쓸 수 없다.
	 * @param VarName 대상 변수 이름.
	 * @param MetaJson 기입할 메타 {"키":"값", ...}.
	 *   예: {"RowType":"/Script/WxInventory.WxRewardTableRow"} 는 DataTableRowHandle 변수의 테이블 픽커를 그 행 구조체를 쓰는 테이블로 제한한다.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool SetVariableMeta(UBlueprint* Blueprint, FName VarName, const FString& MetaJson);

	/**
	 * 블루프린트 변수의 메타를 JSON 으로 돌려준다.
	 * 반환 형식: {"키":"값", ...}. 메타가 없으면 빈 오브젝트.
	 * @param VarName 대상 변수 이름.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static FString GetVariableMeta(UBlueprint* Blueprint, FName VarName);
};
