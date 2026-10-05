# 블루프린트 그래프·위젯·MVVM

## 그래프 DSL

- `BlueprintTools.read_graph_dsl`·`write_graph_dsl`로 이벤트 그래프를 S-표현식으로 읽고 쓴다. 문법은 `get_graph_dsl_docs`에 있다.
- **`write_graph_dsl`은 그래프를 갈아 끼우지 않고 이벤트 단위로 병합한다.** 같은 이벤트는 새 본문으로 바뀌지만 DSL에 없는 이벤트와 고아 노드는 남아 컴파일을 깨뜨린다. 남은 것은 `find_nodes`(title:"") → `get_node_infos`(type_id로 식별) → `delete_node`로 지운다.
- **읽고 그대로 되쓰면 조용히 틀린 그래프가 될 수 있다.** `read_graph_dsl`은 출력이 여럿인 노드의 핀 선택, FormatText 명명 인자, 텍스트 로컬라이즈 키, CreateEvent 바인딩을 잃는다. 되쓰면 0번 출력이 연결되고 타입이 다르면 변환 노드까지 끼어든다. 되쓰기 전에 `get_node_infos`로 실제 연결을 보고 `(bind (a b) ...)`로 직접 적는다.
- 노드 타입은 `find_node_types`(`context_pins:[]` 필수), 핀 이름은 `get_node_type_pins`로 찾는다. `get_node_type_pins`는 그래프에 임시 노드를 남기니 정리한다.
- 타입이 다른 핀(String→Name 등)을 이으면 변환 노드가 자동으로 들어간다.
- `delete_node`는 K2 노드만 지운다. 주석 노드는 BP 에디터 그래프 영역을 클릭하고 `Ctrl+A`·`Delete`로 지운다([slate-pie.md](slate-pie.md)). 함수 진입 노드는 엔진이 삭제를 막는다.
- `compile_blueprint`는 패키지를 더티로 만들지 않는다. 저장은 `SavePackages`로 한다.
- enum 멤버 변수와 변수 메타는 `WxToolset.WxBlueprintToolset`으로 만든다.

## 함수를 커스텀 이벤트로 바꾸기

- MCP로는 타입 있는 파라미터를 가진 커스텀 이벤트를 만들 수 없다. DSL의 `(event Name (Param))`은 이름만 받고, `add_function_param`은 함수·디스패처 그래프 전용이다.
- 유일한 길은 엔진 메뉴다. 함수 진입 노드 우클릭 → Organization → "Convert Function to Event"를 SlateInspector로 누른다. 같은 이름의 이벤트가 생기고 파라미터 핀이 타입째 옮겨지며 호출부도 유효하다. 반환값이 있거나 오버라이드인 함수는 메뉴가 없다.
- 파라미터 있는 이벤트를 새로 만들 때는 `add_function_graph` → `add_function_param` → `write_graph_dsl`로 본문을 쓴 뒤 위 메뉴로 바꾼다. 같은 이름의 이벤트를 방금 지웠다면 한 번 컴파일해 이름을 비운 뒤 만든다. 아니면 `_0`이 붙는다.
- UI 조작 요령:
  - 에셋 에디터가 앞에 있어야 클릭이 먹는다. 매번 `EditorAppToolset.OpenEditorForAsset`으로 앞으로 가져온다.
  - 함수 그래프는 My Blueprint 트리 항목을 더블클릭해 연다. 탭 자체 클릭은 안 먹는다. 트리 항목 ref는 갱신마다 바뀌니 직전에 다시 스냅샷한다.
  - 위젯 BP는 Designer 모드로 열린다. 우상단 `Graph` 체크박스로 바꾼 뒤 진행한다.
  - 그래프 노드는 스냅샷에서 제목 없는 `image`로 잡힌다. `get_node_infos`로 노드 사이 그래프 좌표 차이를 구하고, 화면 좌표 차이가 `줌 배율 × 그 차이`인 쌍을 찾는다. 줌 라벨과 배율은 1:1=1.0, -1=0.875, -2=0.75, -3=0.675, -4=0.5다.
  - 컨텍스트 메뉴는 새 최상위 창으로 뜬다. `WaitFor("Convert Function to Event")`로 맞는 노드를 눌렀는지 보고, 아니면 Escape 후 다음 후보로 간다.

## 위젯(UMG)

- `UMGToolSet`: CreateWidgetBlueprint·AddWidget·ToggleWidgetAsVariable·CompileWidgetBlueprint·GetWidgets·RemoveWidget·RenameWidget. `RenameWidget`은 바인딩된 OnClicked 이벤트 노드도 함께 고친다.
- 슬롯·위젯 속성과 버튼 라벨 같은 인스턴스 값은 `ObjectTools`로 쓴다.
- NamedSlot을 가진 부모 WBP를 지우거나 자식을 재부모화하면 자식의 `NamedSlotBindings`에 고아 위젯이 남아 디자이너에서 `ensure(WidgetTree)`가 난다. 계층 패널에 보이지 않으니 `GetWidgets`에서 parent와 namedSlotHost가 둘 다 None인데 루트가 아닌 위젯을 찾아 `RemoveWidget`으로 지운다. `ensure`는 세션당 한 번만 보고되니 확인은 에디터 재시작 후 로그로 한다.

## MVVM

- WBP 에셋 경로는 CDO로 바뀌어 확장에 닿지 않는다. 뷰는 서브오브젝트 경로 `WBP.WBP:MVVMWidgetBlueprintExtension_View_0.MVVMBlueprintView_0`로 직접 열고 `bindings`·`availableViewModels`를 쓴다. 리졸버는 클래스 경로로 인스턴스드 신설이 된다.
- MVVM 확장이 없는 위젯에는 확장을 새로 만들 수 없다. MVVM 위젯이 필요하면 기존 MVVM WBP를 복제해 고친다.
- 변환 함수, 소스·인자 경로, 이벤트 목적지는 `WxToolset.WxMVVMToolset`으로 쓴다.
- 이벤트 행은 `EventPath`·`DestinationPath`가 VisibleAnywhere라 MCP로 만들 수 없다. 바인딩 패널을 SlateInspector로 조작한다. 위젯 대상 바인딩을 추가하고, 왼쪽 셀의 연필 아이콘으로 델리게이트(On Clicked)를 고르면 하단에 Event 버튼이 생긴다. 그걸 누르면 이벤트 행이 되고, 목적지는 `SetEventDestination`으로 정한다.
