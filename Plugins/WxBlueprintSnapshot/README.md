# WxBlueprintSnapshot

Blueprint 저장 시 CDO delta, SCS 트리, 변수 선언, 그래프 의사코드를 JSON으로 추출하는 UE5 에디터 플러그인. BP 히스토리 추적 및 AI 스캐폴딩 용도.

## 개요

- **Engine**: Unreal Engine 5.7
- **Module type**: Editor (PostEngineInit)
- **Dependencies**: ModelViewViewModel
- **출력 경로**: `Plugins/WxBlueprintSnapshot/Snapshots/<PackagePath>.json`
  (경로가 240자 초과 시 해시 폴더로 폴백)

## 동작 방식

1. `UPackage::PackageSavedWithContextEvent` 구독
2. 저장된 패키지 내 Blueprint를 필터링 후 큐잉
3. Ticker가 프레임당 1건씩 처리 (에디터 스파이크 방지)
4. 이전 JSON과 동일하면 write skip
5. ReadOnly 파일은 자동 해제 후 덮어쓰기

## 설정

`Project Settings > Plugins > Wx Blueprint Snapshot`

| 항목 | 기본값 | 설명 |
|---|---|---|
| `bEnabled` | true | 전체 기능 on/off |
| `IncludeDirectories` | [] | 대상 BP 폴더 (비어있으면 전체). Content 브라우저에서 선택 |
| `ExcludeDirectories` | [] | 제외 BP 폴더 |
| `bIncludeComponents` | true | SCS 트리 포함 |
| `bIncludeVariables` | true | NewVariables 포함 |
| `bIncludeInterfaces` | true | ImplementedInterfaces 포함 |
| `bIncludeWidgetTree` | true | WBP WidgetTree 포함 |
| `bIncludeMvvm` | true | MVVM 바인딩 포함 |
| `bIncludeGraphs` | true | 그래프 의사코드 포함 |

---

## ✅ 지원됨

### 대상 Blueprint
- `BPTYPE_Normal` (Actor/Object BP)
- `UWidgetBlueprint` (WBP — 위젯 트리 + MVVM 추가 추출)

### CDO Delta (classDefaults)
- `CPF_Edit | BlueprintVisible | BlueprintAssignable` 속성
- Bool, Enum, Byte, 정수/실수, String/Name/Text, SoftObject
- Object/Class 참조 (경로 문자열)
- **Instanced subobject 재귀 delta** (`CPF_InstancedReference`, `CLASS_DefaultToInstanced`)
- Struct (재귀, 빈 struct는 드롭)
- Array, Set
- Map (문자열 키는 object, 그 외는 `[{key, value}]` 배열)
- `Identical()` 실패 시 `ExportText` 텍스트 비교 fallback

### SCS (components)
- `SimpleConstructionScript` 전체 노드 트리
- `componentClass`, `attachParent`, `attachSocket`
- 각 컴포넌트 템플릿의 CDO delta

### 그래프 (의사코드)
- Event, CustomEvent, FunctionEntry 진입점
- `if/else`, `Sequence`, `VariableSet`, `CallFunction`, `DynamicCast`, `MacroInstance`(ForEach/ForLoop/While), `Return`
- Knot 스킵, 사이클 감지 (`goto NodeName`)

### Widget Blueprint
- `WidgetTree` 전체 순회 (루트/부모/슬롯)
- 위젯·슬롯 CDO delta
- 이름 충돌 시 `#2`, `#3` 접미사 부여
- MVVM View extension: ViewModel 컨텍스트, 바인딩(정렬됨), conversion function 경로

### 메타
- `parentClass`, `blueprintPath`
- `NewVariables` (type + 기본값)
- `ImplementedInterfaces`

### 실행/파일 I/O
- PostSaveContext 필터링 (procedural/cook/autosave 제외)
- Ticker 기반 프레임당 1건 처리
- JSON 키 정렬 (git diff 친화)
- 변경 없으면 write skip
- ReadOnly 파일 자동 해제
- Windows `MAX_PATH` 폴백 (240자 초과 시 해시 폴더)
- Include/Exclude 디렉터리 필터

---

## ❌ 지원되지 않음 / 제한

### Blueprint 종류
- **`BPTYPE_Interface`** — skip
- **`BPTYPE_MacroLibrary`** — skip
- **`BPTYPE_FunctionLibrary`** — skip
- **`UEditorUtilityBlueprint`** — skip
- **Animation Blueprint** — AnimGraph 스테이트머신/Pose 노드는 미지원 (K2Node가 아니므로 fallback title만)
- **Control Rig, Metasound, Niagara** 등 커스텀 그래프 기반 에셋 — 미지원
- **Data-only BP, Gameplay Ability Blueprint** — Normal BP로 취급되어 CDO delta까지는 동작. 고유 서브시스템 해석은 없음

### 속성 직렬화
- **Transient / EditorOnly / Deprecated** 속성 무시 (의도된 것)
- **Non-Edit / Non-BP** 속성 무시 (C++ 내부 상태)
- **Delegate, MulticastDelegate** 속성 — fallback `ExportText` 문자열만
- **FieldPath, Optional** 속성 — fallback 문자열
- **Array/Set** 요소 diff 없음 — 전체 덤프 (요소 재정렬 시 전체가 diff에 찍힘)
- **Map** — 요소 diff 없음
- **Text Property**는 `ToString()`만 저장 → 로컬라이제이션 키/네임스페이스 손실

### 그래프 의사코드
- **핀 위치/주석/색상** — 기록 안 함
- **타이밍 노드** (Delay, Timeline, Gate, MultiGate) — fallback title만
- **Event Dispatcher** 호출 — `CallFunction`으로 나오나 dispatcher 특성 구분 안 됨
- **Latent action** (Async Task, BlueprintAsyncAction) — 진입/완료 핀 분기 해석 없음
- **로컬 변수** — 함수 본문 로컬 변수 선언 기록 안 함
- **수학 노드 체인** — K2Node_CallFunction으로 장황하게 전개
- **Pure 함수 의존** — 실행 체인 기반이라 pure 노드는 데이터 입력 시점에 inline 전개 (중복 시 중복 렌더)
- **매크로 내부 본문** — 호출 라인만, 내부 그래프는 펼치지 않음
- **Comment Node** — 기록 안 함

### WBP
- **Named Slots** 내용 — 주입 시점이 런타임이라 CDO에서는 비어있음
- **Widget Animation** — 미지원
- **UMG Legacy Property Binding** — MVVM만 지원, 구식 바인딩은 무시

### MVVM
- **Conversion function 내부 로직** — path만
- **MVVM View Event** — 바인딩만, 이벤트 연결 미지원
- **Field Notify** 플래그 — 기록 안 함

### SCS
- **AddedActorComponents (Inherited Component Handler)** — 상속 컴포넌트 커스터마이즈 미지원

### 실행/저장
- **커맨드렛**에서 동작 안 함
- **PIE / Cook / Procedural save**에서 동작 안 함
- **Autosave**에서 동작 안 함
- **런타임 빌드 포함 안 됨** — 에디터 전용
- **대용량 BP**: 프레임당 1건 처리이므로 큐가 쌓이면 지연

### 컴파일 상태
- `BS_Dirty`, `BS_Error`, `BS_Unknown` BP는 skip (컴파일 먼저 필요)

---

## 요약

**타깃**: Actor BP, Object BP, Widget BP의 **구조/설정/로직 윤곽**을 git-friendly JSON으로 기록.

**비타깃**: 그래프 비주얼 디버깅, AnimBP/ControlRig, 정확한 픽셀 수준의 BP 재현, 런타임.
