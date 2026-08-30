// Copyright Woogle. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "WxStateTreeToolset.generated.h"

class UStateTree;
class UStateTreeState;

/**
 * 기존 MCP 표면(ObjectTools 등)이 닿지 못하는 지점만 뚫는다 — 루트 파라미터 백 정의, 프로퍼티 바인딩(EditorBindings, 편집 플래그 없는 UPROPERTY), 링크 상태 전환·파라미터 오버라이드, 프로그래매틱 컴파일, 스키마 고정 에셋 생성.
 * 상태·태스크·전이의 일반 편집은 기존 ObjectTools.set_properties 를 그대로 쓴다.
 */
UCLASS(BlueprintType, Hidden)
class UWxStateTreeToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * StateTreeComponentSchema(ContextActorClass=Actor) 로 고정된 일반 UStateTree 에셋을 생성한다.
	 * @param PackagePath 에셋을 만들 폴더 경로. 예: "/Game/Quest/Steps"
	 * @return 생성된 StateTree 에셋. 실패 시 스크립트 에러.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static UStateTree* CreateStateTree(const FString& PackagePath, const FString& AssetName);

	/**
	 * 루트 파라미터 백의 소스 ID(바인딩 소스로 쓰는 GUID)와 파라미터 목록을 JSON 으로 돌려준다.
	 * 반환 형식: {"rootParametersId":"GUID","parameters":[{"name":...,"id":"GUID","type":"Text|Float|Struct|...","container":"None|Array","valueTypeObject":"/Script/... 또는 빈 문자열"}]}
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static FString GetRootParameters(UStateTree* StateTree);

	/**
	 * @param Type EPropertyBagPropertyType 이름. 예: "Text", "Float", "Bool", "Int32", "Struct", "Object", "SoftObject"
	 * @param ValueTypePath Type 이 "Struct" 면 값 구조체 경로(예: "/Script/Engine.DataTableRowHandle"), Object/SoftObject/Class/SoftClass 면 값 클래스 경로(예: "/Script/Engine.Actor").
	 * @param bArray true 면 배열 컨테이너로 추가한다.
	 * @param MetaJson 선택. 생성 프로퍼티에 붙일 메타 {"키":"값", ...}. 예: {"AllowedLocators":"Actor"} 는 UOL 파라미터에 전용 액터 픽커를 띄우며, 링크 상태 오버라이드 행까지 전파된다. 빈 문자열이면 메타 없음.
	 * @return 추가된 파라미터의 ID(GUID 문자열).
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static FString AddRootParameter(UStateTree* StateTree, FName Name, const FString& Type, const FString& ValueTypePath, bool bArray, const FString& MetaJson);

	/**
	 * 기존 루트 파라미터의 메타를 갈아 끼운다. 파라미터를 지웠다 다시 만들지 않으므로 ID·값·바인딩이 유지된다.
	 * @param MetaJson 기입할 메타 {"키":"값", ...}. 기존 메타는 이것으로 교체되며, 빈 문자열이면 메타를 모두 지운다.
	 *   예: {"RowType":"/Script/WxDialogue.WxDialogueTableRow"} 는 DataTableRowHandle 파라미터의 테이블 픽커를 그 행 구조체를 쓰는 테이블로 제한한다.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool SetRootParameterMeta(UStateTree* StateTree, FName Name, const FString& MetaJson);

	/**
	 * 그 파라미터를 소스로 쓰던 바인딩은 함께 지워지지 않으므로 RemoveBinding 으로 별도 정리한다.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool RemoveRootParameter(UStateTree* StateTree, FName Name);

	/**
	 * 루트 파라미터 백의 값(기본값)을 JSON 으로 기입한다.
	 * @param ValuesJson {"파라미터명": 값, ...}.
	 *   값 규약 — Text/숫자/bool 은 JSON 원시값, 오브젝트·소프트 참조는 경로 문자열(레벨 액터 예: "/Game/Maps/LV_X.LV_X:PersistentLevel.액터명"), 배열은 그 값들의 JSON 배열, DataTableRowHandle 은 {"DataTable":"/Game/...경로","RowName":"행이름"}.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool SetRootParameterValues(UStateTree* StateTree, const FString& ValuesJson);

	/**
	 * 프로퍼티 바인딩을 추가한다. 소스가 루트 파라미터면 SourceStructId 에 GetRootParameters 의 rootParametersId 를 넣는다.
	 * @param SourceStructId 소스 구조체 GUID(루트 파라미터 ID 또는 노드 iD).
	 * @param SourcePath 소스 프로퍼티 경로. 예: "Npcs" (구조체 전체 복사면 빈 문자열 불가 — 프로퍼티명까지 쓴다)
	 * @param TargetStructId 타깃 노드의 iD(GUID).
	 * @param TargetPath 타깃 인스턴스 데이터의 프로퍼티 경로. 예: "Targets", "DialogueRow"
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool AddBinding(UStateTree* StateTree, const FString& SourceStructId, const FString& SourcePath, const FString& TargetStructId, const FString& TargetPath);

	/**
	 * 지정 타깃 경로의 바인딩을 제거한다(경로 포함 일치).
	 * @param TargetStructId 타깃 노드의 iD(GUID).
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool RemoveBinding(UStateTree* StateTree, const FString& TargetStructId, const FString& TargetPath);

	/**
	 * 에셋의 모든 프로퍼티 바인딩을 JSON 배열로 돌려준다.
	 * 반환 형식: [{"sourceId":"GUID","sourcePath":"...","targetId":"GUID","targetPath":"..."}]
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static FString GetBindings(UStateTree* StateTree);

	/**
	 * 상태를 LinkedAsset 타입으로 전환하고 다른 StateTree 에셋을 링크한다.
	 * 상태의 기존 태스크는 제거되고, 파라미터 백이 링크 에셋의 루트 파라미터 레이아웃으로 동기화된다.
	 * @param State 전환할 상태. 예: "/Game/.../ST_X.ST_X:StateTreeEditorData_0.StateTreeState_0.StateTreeState_1"
	 * @param LinkedAsset 링크할 StateTree 에셋(스키마가 호환돼야 컴파일이 통과한다).
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool LinkStateToAsset(UStateTreeState* State, UStateTree* LinkedAsset);

	/**
	 * 링크 상태의 파라미터 값을 기입하고 해당 파라미터를 오버라이드로 마킹한다.
	 * 먼저 LinkStateToAsset 으로 레이아웃이 동기화돼 있어야 한다.
	 * @param ValuesJson SetRootParameterValues 와 같은 값 규약의 {"파라미터명": 값, ...}.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool SetStateParameterValues(UStateTreeState* State, const FString& ValuesJson);

	/**
	 * 링크 상태의 파라미터 오버라이드를 해제하고 값을 링크 에셋 기본값으로 되돌린다(바인딩으로 값을 받는 파라미터의 리터럴 잔재 제거용).
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool ClearStateParameterOverride(UStateTreeState* State, FName Name);

	/**
	 * 임의 오브젝트(컴포넌트·액터 등)의 FStateTreeReference 프로퍼티에 ST 에셋을 지정하고 파라미터 레이아웃을 동기화한다.
	 * @param PropertyName 대상 FStateTreeReference UPROPERTY 이름. 예: "Quest"
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool SetReferenceStateTree(UObject* Object, FName PropertyName, UStateTree* StateTree);

	/**
	 * 임의 오브젝트(컴포넌트·액터 등)의 FStateTreeReference 프로퍼티에 파라미터 값을 기입하고 오버라이드로 마킹한다.
	 * 레퍼런스에 에셋이 먼저 지정돼 있어야 하며(SetReferenceStateTree), 레이아웃이 어긋나 있으면 기입 전에 에셋 기준으로 동기화한다.
	 * @param PropertyName 대상 FStateTreeReference UPROPERTY 이름. 예: "Quest"
	 * @param ValuesJson SetRootParameterValues 와 같은 값 규약의 {"파라미터명": 값, ...}.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static bool SetReferenceParameterValues(UObject* Object, FName PropertyName, const FString& ValuesJson);

	/**
	 * StateTree 를 컴파일하고 결과와 컴파일러 로그를 돌려준다. 더티 상태와 무관하게 항상 컴파일한다.
	 * 저장은 하지 않으므로 성공 후 AssetTools.save_assets 를 따로 호출한다.
	 */
	UFUNCTION(meta = (AICallable), Category = "Wx")
	static FString CompileStateTree(UStateTree* StateTree);
};
