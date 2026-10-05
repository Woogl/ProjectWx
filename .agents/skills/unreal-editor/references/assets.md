# 에셋 프로퍼티 편집

## ObjectTools 기본

- 읽기: `ObjectTools.get_properties(instance={refPath}, properties=["camelCase이름"])`. 이름은 먼저 `list_properties`로 확인한다. 위젯·슬롯처럼 클래스마다 이름이 다른 대상은 특히 그렇다.
- 쓰기: `ObjectTools.set_properties(instance, values)`. `values`는 객체가 아니라 **JSON 문자열**이다.
- 편집 플래그 없는 UPROPERTY와 `VisibleAnywhere`(EditConst)는 읽거나 쓸 수 없다. 이런 곳이 `WxToolset`을 만든 이유다.
- 런타임 오브젝트도 경로로 읽고 쓴다. PIE 액터는 `…/UEDPIE_0_맵.맵:PersistentLevel.액터` 경로이고, 액터 서브오브젝트는 `<액터 경로>.컴포넌트명`이다. 존재 확인은 `get_class`가 간편하다.

## 배열

배열 쓰기는 거절되거나 조용히 틀리기 쉽다. 쓴 뒤에는 반드시 다시 읽는다.

- **크기 변경과 기존 원소 값 변경을 한 번에 못 한다.** "ArrayAdd/ArrayRemove: ambiguous" 또는 "elements changed alongside the size change"로 통째로 거절된다.
  - 늘릴 때: 기존 원소를 조회 결과 **그대로(모든 필드)** 두고 새 원소만 붙여 한 번 쓴다. 값 수정은 같은 크기로 두 번째 호출에서 한다.
  - 줄일 때: 남길 원소를 원본 그대로 넣어 크기만 줄이고, 값 수정은 그다음에 한다.
- 오브젝트 참조 배열은 기존 원소까지 전부 `{"refPath":...}` 표기로 통일해야 통과한다. 문자열 경로가 섞이면 "변경"으로 판정돼 거절된다.
- 인스턴스드 오브젝트 배열은 원소를 **클래스 경로 문자열**로 넣으면 서브오브젝트가 생긴다(`{instance:...}` 표기는 null이 들어간다). 생긴 서브오브젝트는 `ASSET.ASSET:SubObj_0` 경로로 직접 편집한다.
- 같은 크기일 때 빈 dict `{}`는 그 원소를 바꾸지 않는다는 뜻이다.
- 기존 원소의 `_structType` 교체는 무시된다. 타입을 바꾸려면 `[]`로 비운 뒤 다시 채운다.
- 빈 배열에서 N개로 늘리며 값까지 넣는 것과 `[]`로 비우는 것은 된다.

## 블루프린트 CDO

- `BlueprintTools.get_default_object`로 CDO(`Default__X_C`) 경로를 얻는다. BP 에셋 경로를 `ObjectTools`에 넘기면 CDO로 바뀐다.
- 컴포넌트 템플릿은 `…Default__X_C:컴포넌트명`, 차일드 액터 템플릿은 `BP.BP_C:컴포넌트_GEN_VARIABLE.<템플릿명>_CAT`이다.
- CDO·템플릿을 고친 뒤에는 **`BlueprintTools.compile_blueprint`가 필수**다. 컴파일하지 않으면 조회 값은 바뀌어도 스폰된 인스턴스에는 반영되지 않는다. 컴파일 뒤 `SavePackages`로 저장한다.
- 신규 BP: `BlueprintTools.create(asset_type=부모 클래스)` → CDO 편집 → 컴파일 → 저장.

## 데이터 에셋·테이블

- 데이터 에셋 생성: `DataAssetTools.create(folder_path, asset_name, asset_type={refPath:"/Script/모듈.클래스"})`.
- DataTable: `DataTableTools.create(schema=행 구조체)` / `add_rows` / `set_rows` / `get_rows`. `values`는 JSON 문자열이고 프로퍼티는 camelCase다. FText는 평문으로 넣으면 키가 자동으로 생긴다.
- 헤드리스 커맨드릿(`-run=pythonscript`)은 게임 모듈 로드에 자주 실패하니 라이브 에디터를 쓴다.
- 읽기 전용 `.uasset`은 저장할 수 없다. `Set-ItemProperty <파일> -Name IsReadOnly -Value $false`로 푼 뒤 저장한다.
- `AssetTools.update_metadata_tags`의 `remove_tags`는 항상 실패한다. 메타 태그를 저장 트리거로 쓰지 않는다.

## 몽타주

- 노티파이(`Notifies`·`AnimNotifyTracks`)와 섹션은 `ObjectTools`로 읽지도 쓰지도 못한다. `WxToolset.WxAnimMontageToolset`을 쓴다.

## 머티리얼

- `MaterialTools`(create_material·add_expression·connect_expressions·connect_to_output·recompile)와 `ObjectTools`로 만든다. Custom HLSL 노드로 절차적 형상도 된다.
- Custom 노드 입력 배열은 늘리기가 실패할 수 있다. 입력을 하나로 묶어(패킹) 우회한다.
- Custom 노드의 추가 출력(AdditionalOutputs)으로 노멀을 내면 NaN이 생긴다. 출력마다 Custom 노드를 따로 둔다.
- 결과 확인은 `EditorAppToolset.CaptureAssetImage`(썸네일 base64 PNG)로 한다.

## Niagara

- `NiagaraToolset_System`(CreateNiagaraSystem·AddEmitter·AddModule·AddRenderer·SetStackInputData·AddUserVariables·SetRendererData·SetModuleEnabled·GetSystemCompileState)을 쓴다.
- 스택 위치는 `StackItemReference{system, emitterName, scriptName, moduleName, rendererIndex, inputNameStack}`이다. 입력값은 Local 값이나 Linked(`User.X`, `Particles.NormalizedAge` 등)로 준다.
- 시스템·이미터 생성에는 **템플릿이 필수**다. `/Niagara/DefaultAssets/DefaultSystem`에는 Fountain 이미터가 딸려 오니 `RemoveEmitter`로 지운다. 스프라이트 버스트 템플릿은 `/Niagara/DefaultAssets/Templates/Emitters/SimpleSpriteBurst`다.
- Niagara 시스템은 `CaptureAssetImage`를 지원하지 않는다.
