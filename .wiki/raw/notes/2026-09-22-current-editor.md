---
title: "편집기 모듈 등록과 공개 도구 계약 조사"
source: "MANUAL"
type: notes
ingested: 2026-09-22
tags: [wx, static-review, editor]
summary: "편집기 모듈 등록과 공개 도구 계약 조사의 저장소 원문 발췌와 파일별 SHA-256. 정적 확인 범위이며 실행 검증이 아니다."
revision: fe8c943f49401326e1007fedd78a937c9e66db47
---

# 편집기 모듈 등록과 공개 도구 계약 조사

조사일: 2026-09-22. 기준 HEAD: `fe8c943f49401326e1007fedd78a937c9e66db47` + 현재 작업 트리. 아래 파일의 선택된 계약·실행 경로를 조사했다. SHA-256은 파일 전체 바이트의 식별값이며 파일 전체의 결함 검토를 뜻하지 않는다. 코드 원문은 해당 저장소 경로가 정본이다. 발췌 전후 생략 부분과 바이너리 에셋·실행 결과는 이 기록에 포함하지 않는다.

## Source/WxEditor/WxEditor.Build.cs
- [저장소 원문](<../../../Source/WxEditor/WxEditor.Build.cs>)
- SHA-256: `2864b8c69081ae4f78d381053d677590be56485713ba2207a086fc827d514231`
```text
...
18: 		});
19: 
20: 		PrivateDependencyModuleNames.AddRange(new string[]
21: 		{
22: 			"GameplayAbilities",
23: 			"GameplayTags",
24: 			"GameplayStateTreeModule",
25: 			"Slate",
26: 			"SlateCore",
27: 			"PropertyEditor",
28: 			"StateTreeEditorModule",
29: 			"UniversalObjectLocator",
30: 			"UnrealEd",
31: 			"WxCore",
32: 			"WxInventory",
33: 			"WxUI",
34: 			"WxWorld",
35: 		});
36: 	}
37: }
```
## Source/WxEditor/WxEditor.cpp
- [저장소 원문](<../../../Source/WxEditor/WxEditor.cpp>)
- SHA-256: `ede4e874af6afe684f7b605151350b1932f2da7024510f8140b33afab1a5cff4`
```text
...
36: }
37: 
38: void FWxEditorModule::StartupModule()
39: {
40: 	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(WxEditorModule::PropertyEditorModuleName);
41: 
42: 	ActorLocatorIdentifier = MakeShared<FWxActorLocatorTypeIdentifier>();
43: 	PropertyModule.RegisterCustomPropertyTypeLayout(
44: 		FUniversalObjectLocator::StaticStruct()->GetFName(),
45: 		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FWxActorLocatorCustomization::MakeInstance),
46: 		ActorLocatorIdentifier);
47: 
48: 	DataTableRowHandleIdentifier = MakeShared<FWxDataTableRowHandleTypeIdentifier>();
49: 	PropertyModule.RegisterCustomPropertyTypeLayout(
50: 		FDataTableRowHandle::StaticStruct()->GetFName(),
51: 		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FWxDataTableRowHandleCustomization::MakeInstance),
52: 		DataTableRowHandleIdentifier);
53: 
54: 	PropertyModule.RegisterCustomPropertyTypeLayout(
55: 		FWxStateTreeComponentName::StaticStruct()->GetFName(),
56: 		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FWxStateTreeComponentNameCustomization::MakeInstance));
...
75: 	PropertyModule.NotifyCustomizationModuleChanged();
76: 
77: 	UThumbnailManager::Get().RegisterCustomRenderer(
78: 		UWxItemDefinition::StaticClass(),
79: 		UWxItemDefinitionThumbnailRenderer::StaticClass());
80: 
81: 	// IWxUIData 를 든 BP 썸네일을 그 아이콘으로 렌더링하기 위해, 엔진이 ini 로 등록한 기본 Blueprint 렌더러를 파생 렌더러로 교체한다.
82: 	// RegisterCustomRenderer 는 동일 클래스 중복 등록을 거부하므로 기존 등록을 먼저 해제해야 한다.
83: 	UThumbnailManager::Get().UnregisterCustomRenderer(UBlueprint::StaticClass());
84: 	UThumbnailManager::Get().RegisterCustomRenderer(
85: 		UBlueprint::StaticClass(),
86: 		UWxUIDataThumbnailRenderer::StaticClass());
87: 
88: 	// 엔진은 UStateTreeComponent 자리를 비워 두었고, 에디터가 클래스 사슬을 거슬러 찾으므로 장치의 파생 컴포넌트까지 덮인다.
89: 	if (GUnrealEd)
90: 	{
91: 		GUnrealEd->RegisterComponentVisualizer(UStateTreeComponent::StaticClass()->GetFName(), MakeShared<FWxDeviceLinkVisualizer>());
92: 	}
93: }
94: 
95: void FWxEditorModule::ShutdownModule()
96: {
97: 	if (GUnrealEd)
98: 	{
99: 		GUnrealEd->UnregisterComponentVisualizer(UStateTreeComponent::StaticClass()->GetFName());
100: 	}
101: 
102: 	if (FModuleManager::Get().IsModuleLoaded(WxEditorModule::PropertyEditorModuleName))
103: 	{
104: 		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(WxEditorModule::PropertyEditorModuleName);
105: 		if (ActorLocatorIdentifier.IsValid())
106: 		{
107: 			PropertyModule.UnregisterCustomPropertyTypeLayout(TEXT("UniversalObjectLocator"), ActorLocatorIdentifier);
108: 			ActorLocatorIdentifier.Reset();
109: 		}
110: 		if (DataTableRowHandleIdentifier.IsValid())
111: 		{
112: 			PropertyModule.UnregisterCustomPropertyTypeLayout(TEXT("DataTableRowHandle"), DataTableRowHandleIdentifier);
113: 			DataTableRowHandleIdentifier.Reset();
```
## Plugins/WxToolset/WxToolset.uplugin
- [저장소 원문](<../../../Plugins/WxToolset/WxToolset.uplugin>)
- SHA-256: `7ceeae76f9baa61dd4d15854068a0efb0bfa40f178a6bda8308f5021c9a42fd9`
```text
...
11: 		{
12: 			"Name": "WxToolset",
13: 			"Type": "Editor",
14: 			"LoadingPhase": "PostEngineInit",
15: 			"TargetAllowList": [ "Editor" ]
16: 		}
17: 	],
18: 	"Plugins": [
19: 		{
20: 			"Name": "StateTree",
21: 			"Enabled": true
22: 		},
23: 		{
24: 			"Name": "GameplayStateTree",
25: 			"Enabled": true
26: 		},
27: 		{
28: 			"Name": "ToolsetRegistry",
29: 			"Enabled": true
30: 		}
31: 	]
```
## Plugins/WxToolset/Source/WxToolset/Private/WxToolsetModule.cpp
- [저장소 원문](<../../../Plugins/WxToolset/Source/WxToolset/Private/WxToolsetModule.cpp>)
- SHA-256: `3e83dc6313dcc74a39907e9212edbaa01059bca97fa7a2248bd272f59c2ef88f`
```text
...
11: DEFINE_LOG_CATEGORY(LogWxToolset);
12: 
13: void FWxToolsetModule::StartupModule()
14: {
15: 	UToolsetRegistry::RegisterToolsetClass(UWxAnimMontageToolset::StaticClass());
16: 	UToolsetRegistry::RegisterToolsetClass(UWxBlueprintToolset::StaticClass());
17: 	UToolsetRegistry::RegisterToolsetClass(UWxStateTreeToolset::StaticClass());
18: }
19: 
20: void FWxToolsetModule::ShutdownModule()
21: {
22: 	UToolsetRegistry::UnregisterToolsetClass(UWxAnimMontageToolset::StaticClass());
23: 	UToolsetRegistry::UnregisterToolsetClass(UWxBlueprintToolset::StaticClass());
24: 	UToolsetRegistry::UnregisterToolsetClass(UWxStateTreeToolset::StaticClass());
25: }
26: 
27: IMPLEMENT_MODULE(FWxToolsetModule, WxToolset)
```
## Plugins/WxToolset/Source/WxToolset/Private/WxBlueprintToolset.h
- [저장소 원문](<../../../Plugins/WxToolset/Source/WxToolset/Private/WxBlueprintToolset.h>)
- SHA-256: `4014b4f4d7c99ddfe15ca3d007b88df7f648b416ebe3c78cb04f7d2bc0f5ab0b`
```text
...
25: 	 */
26: 	UFUNCTION(meta = (AICallable), Category = "Wx")
27: 	static bool SetVariableMeta(UBlueprint* Blueprint, FName VarName, const FString& MetaJson);
28: 
29: 	/**
30: 	 * 반환 형식: {"키":"값", ...}. 메타가 없으면 빈 오브젝트.
31: 	 */
32: 	UFUNCTION(meta = (AICallable), Category = "Wx")
33: 	static FString GetVariableMeta(UBlueprint* Blueprint, FName VarName);
34: };
```
## Plugins/WxToolset/Source/WxToolset/Private/WxAnimMontageToolset.h
- [저장소 원문](<../../../Plugins/WxToolset/Source/WxToolset/Private/WxAnimMontageToolset.h>)
- SHA-256: `0ae427a9d503240cdf31bcfa9a00832bcbd60909cca5fd6e0159abfed13e9279`
```text
...
23: 	 */
24: 	UFUNCTION(meta = (AICallable), Category = "Wx")
25: 	static FString DescribeMontage(UAnimMontage* Montage);
26: 
27: 	/**
28: 	 * 원본의 슬롯 세그먼트와 섹션 구성을 대상에 옮기고, 각 섹션의 다음 섹션 링크를 비워 한 방향만 재생되게 한다.
29: 	 * 노티파이와 블렌드·재생 속도 같은 대상 고유 설정은 그대로 둔다.
30: 	 */
31: 	UFUNCTION(meta = (AICallable), Category = "Wx")
32: 	static bool MirrorMontageStructure(UAnimMontage* Source, UAnimMontage* Target);
33: 
34: 	/**
35: 	 * 기준 섹션의 노티파이를 나머지 섹션에 같은 상대 위치로 복제하고 복제한 개수를 돌려준다.
36: 	 * 노티파이 오브젝트를 통째로 복제하므로 설정된 프로퍼티가 그대로 따라간다.
37: 	 * 이미 노티파이가 있는 섹션은 건너뛰므로 다시 돌려도 중복되지 않는다.
38: 	 */
39: 	UFUNCTION(meta = (AICallable), Category = "Wx")
40: 	static int32 ReplicateNotifiesToSections(UAnimMontage* Montage, FName SourceSectionName);
41: 
42: 	/**
43: 	 * 섹션 시작 기준 오프셋(초)에 단발 노티파이를 하나 추가한다.
44: 	 * 트랙은 인덱스로 지목하며 그 인덱스까지 없는 트랙은 만든다.
45: 	 */
46: 	UFUNCTION(meta = (AICallable), Category = "Wx")
47: 	static bool AddNotify(UAnimMontage* Montage, TSubclassOf<UAnimNotify> NotifyClass, FName SectionName, float OffsetInSection, int32 TrackIndex);
48: 
49: 	UFUNCTION(meta = (AICallable), Category = "Wx")
50: 	static bool SaveMontage(UAnimMontage* Montage);
51: };
```
## Plugins/WxToolset/Source/WxToolset/Private/WxStateTreeToolset.h
- [저장소 원문](<../../../Plugins/WxToolset/Source/WxToolset/Private/WxStateTreeToolset.h>)
- SHA-256: `08cbf0f26488dbfc08cfc8e990fd7c54ac291ed8b5006b6053e9881bd11ff26b`
```text
...
26: 	 */
27: 	UFUNCTION(meta = (AICallable), Category = "Wx")
28: 	static UStateTree* CreateStateTree(const FString& PackagePath, const FString& AssetName);
29: 
30: 	/**
31: 	 * 루트 파라미터 백의 소스 ID(바인딩 소스로 쓰는 GUID)와 파라미터 목록을 JSON 으로 돌려준다.
32: 	 * 반환 형식: {"rootParametersId":"GUID","parameters":[{"name":...,"id":"GUID","type":"Text|Float|Struct|...","container":"None|Array","valueTypeObject":"/Script/... 또는 빈 문자열"}]}
33: 	 */
34: 	UFUNCTION(meta = (AICallable), Category = "Wx")
35: 	static FString GetRootParameters(UStateTree* StateTree);
36: 
37: 	/**
38: 	 * @param Type EPropertyBagPropertyType 이름. 예: "Text", "Float", "Bool", "Int32", "Struct", "Object", "SoftObject"
39: 	 * @param ValueTypePath Type 이 "Struct" 면 값 구조체 경로(예: "/Script/Engine.DataTableRowHandle"), Object/SoftObject/Class/SoftClass 면 값 클래스 경로(예: "/Script/Engine.Actor").
40: 	 * @param MetaJson 선택. 생성 프로퍼티에 붙일 메타 {"키":"값", ...}. 예: {"AllowedLocators":"Actor"} 는 UOL 파라미터에 전용 액터 픽커를 띄우며, 링크 상태 오버라이드 행까지 전파된다. 빈 문자열이면 메타 없음.
41: 	 * @return 추가된 파라미터의 ID(GUID 문자열).
42: 	 */
43: 	UFUNCTION(meta = (AICallable), Category = "Wx")
44: 	static FString AddRootParameter(UStateTree* StateTree, FName Name, const FString& Type, const FString& ValueTypePath, bool bArray, const FString& MetaJson);
45: 
46: 	/**
47: 	 * 기존 루트 파라미터의 메타를 갈아 끼운다. 파라미터를 지웠다 다시 만들지 않으므로 ID·값·바인딩이 유지된다.
48: 	 * @param MetaJson 기입할 메타 {"키":"값", ...}. 기존 메타는 이것으로 교체되며, 빈 문자열이면 메타를 모두 지운다.
49: 	 *   예: {"RowType":"/Script/WxDialogue.WxDialogueTableRow"} 는 DataTableRowHandle 파라미터의 테이블 픽커를 그 행 구조체를 쓰는 테이블로 제한한다.
50: 	 */
51: 	UFUNCTION(meta = (AICallable), Category = "Wx")
52: 	static bool SetRootParameterMeta(UStateTree* StateTree, FName Name, const FString& MetaJson);
53: 
54: 	/**
55: 	 * 그 파라미터를 소스로 쓰던 바인딩은 함께 지워지지 않으므로 RemoveBinding 으로 별도 정리한다.
56: 	 */
57: 	UFUNCTION(meta = (AICallable), Category = "Wx")
58: 	static bool RemoveRootParameter(UStateTree* StateTree, FName Name);
59: 
60: 	/**
61: 	 * 루트 파라미터 백의 값(기본값)을 JSON 으로 기입한다.
62: 	 * @param ValuesJson {"파라미터명": 값, ...}.
63: 	 *   값 규약 — Text/숫자/bool 은 JSON 원시값, 오브젝트·소프트 참조는 경로 문자열(레벨 액터 예: "/Game/Maps/LV_X.LV_X:PersistentLevel.액터명"), 배열은 그 값들의 JSON 배열, DataTableRowHandle 은 {"DataTable":"/Game/...경로","RowName":"행이름"}.
64: 	 */
65: 	UFUNCTION(meta = (AICallable), Category = "Wx")
66: 	static bool SetRootParameterValues(UStateTree* StateTree, const FString& ValuesJson);
67: 
68: 	/**
69: 	 * 프로퍼티 바인딩을 추가한다. 소스가 루트 파라미터면 SourceStructId 에 GetRootParameters 의 rootParametersId 를 넣는다.
70: 	 * @param SourceStructId 소스 구조체 GUID(루트 파라미터 ID 또는 노드 ID).
71: 	 * @param SourcePath 소스 프로퍼티 경로. 예: "Npcs" (구조체 전체 복사면 빈 문자열 불가 — 프로퍼티명까지 쓴다)
72: 	 * @param TargetStructId 타깃 노드의 ID(GUID).
73: 	 * @param TargetPath 타깃 인스턴스 데이터의 프로퍼티 경로. 예: "Target", "StartRow"
74: 	 */
75: 	UFUNCTION(meta = (AICallable), Category = "Wx")
76: 	static bool AddBinding(UStateTree* StateTree, const FString& SourceStructId, const FString& SourcePath, const FString& TargetStructId, const FString& TargetPath);
77: 
78: 	/**
79: 	 * 지정 타깃 경로의 바인딩을 제거한다(경로 포함 일치).
80: 	 * @param TargetStructId 타깃 노드의 ID(GUID).
81: 	 */
82: 	UFUNCTION(meta = (AICallable), Category = "Wx")
83: 	static bool RemoveBinding(UStateTree* StateTree, const FString& TargetStructId, const FString& TargetPath);
84: 
85: 	/**
86: 	 * 에셋의 모든 프로퍼티 바인딩을 JSON 배열로 돌려준다.
87: 	 * 반환 형식: [{"sourceId":"GUID","sourcePath":"...","targetId":"GUID","targetPath":"..."}]
88: 	 */
89: 	UFUNCTION(meta = (AICallable), Category = "Wx")
90: 	static FString GetBindings(UStateTree* StateTree);
91: 
92: 	/**
93: 	 * 상태를 LinkedAsset 타입으로 전환하고 다른 StateTree 에셋을 링크한다.
94: 	 * 상태의 기존 태스크는 제거되고, 파라미터 백이 링크 에셋의 루트 파라미터 레이아웃으로 동기화된다.
```
## Plugins/BoxComponentVisualizer/Source/BoxComponentVisualizerEditor/Private/BoxComponentVisualizerEditorModule.cpp
- [저장소 원문](<../../../Plugins/BoxComponentVisualizer/Source/BoxComponentVisualizerEditor/Private/BoxComponentVisualizerEditorModule.cpp>)
- SHA-256: `95dd89ec6cadf40839913c26bd1420d07b66f4f2eaab4af70d715338d2bc2ac6`
```text
...
11: IMPLEMENT_MODULE(FBoxComponentVisualizerEditorModule, BoxComponentVisualizerEditor)
12: 
13: void FBoxComponentVisualizerEditorModule::StartupModule()
14: {
15: 	// 엔진은 UBoxComponent 자리를 비워 두었고, 에디터가 클래스 사슬을 거슬러 찾으므로 파생 박스까지 함께 덮인다.
16: 	if (GUnrealEd)
17: 	{
18: 		GUnrealEd->RegisterComponentVisualizer(UBoxComponent::StaticClass()->GetFName(), MakeShared<FBoxComponentVisualizer>());
19: 	}
20: }
21: 
22: void FBoxComponentVisualizerEditorModule::ShutdownModule()
23: {
24: 	if (GUnrealEd)
25: 	{
26: 		GUnrealEd->UnregisterComponentVisualizer(UBoxComponent::StaticClass()->GetFName());
27: 	}
28: }
```
