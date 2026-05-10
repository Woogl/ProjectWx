# WxBlueprintSnapshot

블루프린트 에셋을 저장할 때마다 그 안의 내용(기본값, 컴포넌트, 변수, 함수, 그래프 로직)을 JSON 파일로 자동 기록합니다.

## 이 플러그인으로 할 수 있는 것

> **💡 핵심 가치**  
> BP 를 텍스트 자산처럼 다룰 수 있게 만들어 git / grep / blame / diff / AI 같은 텍스트 기반 도구를 BP 에도 적용할 수 있게 합니다.

### 1. 블루프린트 변경 추적
`.uasset` 은 바이너리라 source control 에서 diff 가 제한됩니다.  
이 플러그인을 사용하면 BP 의 작업 내용을 JSON 파일로도 함께 커밋할 수 있습니다.
- **히스토리 추적** — "누가 언제 변수 값을 바꿨는지", "이 컴포넌트 추가한 사람" 같은 추적이 즉시 가능
- **에디터 없이 PR 리뷰** — 코드 리뷰어가 UE 컴파일 환경 없이도 line-level 코멘트로 BP 변경 토론

### 2. AI 협업
AI 코드 어시스턴트와 결합 효율이 높습니다.
- **토큰 효율** — 스크린샷 대비 1/10 이하 토큰
- **단일 컨텍스트** — 의사 코드 + 변수 + MVVM 바인딩 + 컴포넌트 트리가 한 파일에
- **분석 정확도** — OCR 오인 없이 노드 이름/연결을 정확히 전달

### 3. 블루프린트 일괄 스캔
언리얼 에디터를 켜지 않고도 전체 프로젝트 BP 를 텍스트 도구로 스캔할 수 있습니다.
- **Deprecated API 스캔** — BP Function Library 함수 제거 전에 BP 내 사용처 스캔
- **Tick 이 켜진 Actor BP 찾기** — `bCanEverTick` 검색
- **특정 함수 호출하는 BP** — `eventGraph` / `functions` 의사 코드 grep

### 4. UMG 특화
WBP 는 디자이너 / 기획자 / 프로그래머가 동시에 건드리는 영역이라 변경 영향과 리뷰 부담이 큽니다.  
이 플러그인은 WBP 의 위젯 트리와 MVVM 바인딩까지 JSON 으로 추출합니다.
- **변경 영향 가시성** — 디자이너의 위젯 변경이 프로그래머 코드(MVVM 바인딩 / 위젯 참조)에 미치는 영향, 또는 그 반대 방향을 즉시 검토 가능
- **ViewModel 변경 영향 분석** — 프로퍼티 삭제/리네임 시 영향받는 WBP 를 grep 으로 검출
- **MVVM 바인딩 추적** — "어느 ViewModel 의 어느 프로퍼티를 누가 어디에 바인딩했는가" 를 즉시 파악

### 5. BP 삭제 시 스냅샷 자동 정리
콘텐츠 브라우저에서 BP 에셋을 삭제하면 대응되는 JSON 파일도 자동으로 삭제됩니다.

---

## Quick Start

1. 이 플러그인 전체를 프로젝트의 Plugins 폴더에 추가.
2. `Project Settings > Wx > WxBlueprintSnapshot`에서 BP에서 추출할 데이터 선택.
3. 블루프린트를 저장 (Ctrl+S).  
4. `Plugins/WxBlueprintSnapshot/Snapshots/<BP 패키지 경로>.json` 파일이 생성/업데이트됨.

---

## 어떤 블루프린트가 대상인가?

| 블루프린트 종류 | 지원 | 비고 |
|---|---|---|
| UObject Blueprint | ✅ | Character, Actor, GameplayAbility 등 UObject를 상속한 모든 BP |
| Widget Blueprint (UMG) | ✅ | 위젯 트리 + MVVM 바인딩까지 추출 |
| Animation Blueprint | ⚠️ | AnimGraph 스테이트머신은 지원 안됨 |
| Data Asset | ❌ | 블루프린트가 아닌 UObject 에셋 |
| Data Table / Curve Table | ❌ | CSV/JSON Export 사용 권장 |
| Blueprint Interface | ❌ | 함수 시그니처만 있는 BP. 스킵 |
| Blueprint Macro Library | ❌ | 스킵 |
| Blueprint Function Library | ❌ | 스킵 |
| Editor Utility Blueprint | ❌ | 스킵 |
| Control Rig / Niagara / Metasound | ❌ | 전용 그래프 포맷, 미지원 |

---

## 블루프린트의 어떤 데이터가 추출되는가?

### Details 패널의 데이터

`bSkipUnchangedDefaults=true` 시 부모 클래스 CDO 대비 변경점만 기록(기본 세팅).  
`bSkipUnchangedDefaults=false` 시 전체 기록.

| 항목 | 지원 |
|---|---|
| 멤버 변수 | ✅ |
| 에셋 참조 (StaticMesh, Material 등) | ✅ 경로 문자열 |
| TArray, TMap, TSet 원소 | ✅ |
| Struct 멤버 | ✅ 재귀 기록 |
| Instanced Subobject | ✅ 재귀 기록 |
| `EditAnywhere`/`VisibleAnywhere`/`BlueprintReadWrite`/`BlueprintReadOnly`/`BlueprintAssignable` 속성 | ✅ |
| `Transient` / `DuplicateTransient` / `NonPIEDuplicateTransient` / `Deprecated` / `EditorOnly` 속성 | ❌ 의도적으로 제외 |

### 컴포넌트 (Components 탭에 추가한 것)
| 항목 | 지원 |
|---|---|
| BP의 Components 탭에서 직접 추가한 컴포넌트 | ✅ `components` 필드 (class/attachParent/attachSocket + CDO delta) |
| 컴포넌트 간 어태치 (부모 + 소켓) | ✅ |
| 부모 BP/C++에서 상속된 컴포넌트의 Override 값 | ✅ `classDefaults` 델타에 instanced subobject로 기록 |

### 위젯 블루프린트 (WBP)

MVVM 추출은 프로젝트 세팅에서 비활성화 가능.

| 항목 | 지원 |
|---|---|
| Widget Tree | ✅ 각 위젯에 class/parent/slot 기록 |
| 각 위젯의 Details 값 | ✅ |
| 각 위젯의 PanelSlot 값 | ✅ |
| MVVM ViewModel 컨텍스트 | ✅ class, creationType, optional, setter/getter 등 |
| MVVM 바인딩 | ✅ source/destination/bindingType + conversion 함수 |
| MVVM Conversion Function 인자 핀 | ✅ property path / literal value / orphaned 상태까지 |
| 비활성/비컴파일 바인딩 | ❌ 스킵 |
| Widget Animation | ❌ |
| UMG 구식 Property Binding | ❌ MVVM만 지원 |
| Named Slot의 내용물 | ❌ 런타임 주입이라 BP엔 없음 |

### 이벤트/함수 그래프

스크립트를 UE C++ 스타일의 의사 코드로 출력한다.
* 이벤트 그래프 : `eventGraph` 필드에 이벤트 이름을 키로 한 object로 기록된다.  
* 함수 그래프 : `functions` 필드에 함수 이름을 키로 한 object로 기록된다. Construction Script도 함수로 취급한다.  

| 항목 | 지원 | 예시 |
|---|---|---|
| Event, Custom Event | ✅ | `void BeginPlay() { ... }` |
| Branch (if/else) | ✅ | `if (Condition) { ... } else { ... }` |
| Sequence | ✅ | `// sequence[0]` |
| Switch (Enum / Integer / String / Name) | ✅ | `switch (S) { case EState::Idle: ... break; default: break; }` |
| 변수 Set | ✅ | `MyVar = Value;` |
| BreakStruct (구조체 분해) | ✅ | `MyVector.X` |
| MakeStruct (구조체 생성) | ✅ | `FVector(1, 2, 3)` |
| StructMemberGet (struct 변수 일부 멤버 get) | ✅ | `MyStruct.MemberName` |
| StructMemberSet (struct 변수 일부 멤버 set) | ✅ | `MyStruct.X = NewX;` |
| 함수 호출 (CallFunction) | ✅ | `MyFunc(Arg1, Arg2);` |
| `KismetMathLibrary` 연산자성 함수 | ✅ | `(A) + (B)`, `(X) == (Y)`, `!(B)` |
| Cast | ✅ | `AActor* AsActor = Cast<AActor>(Obj); if (AsActor) { ... }` |
| Return | ✅ | `return Value;` / `return { a, b };` (다중) |
| ForEach / ForEachLoopWithBreak | ✅ | `for (auto& ArrayElement : Array) { ... }`, `break;` |
| ReverseForEachLoop | ✅ | `for (int32 ArrayIndex = Array.Num() - 1; ArrayIndex >= 0; --ArrayIndex) { ... }` |
| WhileLoop | ✅ | `while (Condition) { ... }` |
| Async Action (Latent BP Async Task) | ✅ | `MyAsyncCall(Args); auto Finished = [this]() { ... };` |
| Delay, PrintString 등 Latent/유틸 함수 호출 | ✅ | `KismetSystemLibrary::Delay(Duration=1.0f);` |
| 함수 시그니처 한정자 | ✅ | `UFUNCTION(BlueprintPure, NetMulticast, Reliable)` |
| 일반 매크로 호출 (사용자 정의 등) | ✅ | `MyMacro(arg1, arg2);  // [macro]` (주석으로 호출 라인에 매크로 명시) |
| 매크로 데이터 출력 핀 참조 | ✅ | `MyMacro.OutputPinName` |
| Exec cycle / 분기 합류 | ⚠️ | `// merges into: NodeName` 주석으로 표시 (정확한 흐름 변환 X) |
| Timeline, Gate, 기타 전용 K2Node | ⚠️ | `// Timeline_0` (노드 타이틀 한 줄) |
| Event Dispatcher (Call / Bind / Assign) | ⚠️ | `// On Damaged` (노드 타이틀 한 줄) |
| 매크로 내부 본문 | ❌ | — |
| Knot(리라우팅 노드) | ❌ | — |
| 로컬 변수 | ❌ | — |
| 코멘트 박스 | ❌ | — |
| 노드 위치/색상 | ❌ | — |

### 변수 (Variables 패널)

`variables` 필드는 BP의 변수 목록을 변수명 키 + 값으로 기록한다.  

| 타입 | 지원 | 예시 |
|---|---|---|
| bool | ✅ | `true` / `false` |
| int | ✅ | `42` |
| float | ✅ | `0.f`, `1.5f` |
| double | ✅ | `0.0`, `1.5` |
| FString | ✅ | `FString(\"Foo\")` |
| FName | ✅ | `FName(\"Foo\")` |
| FText | ✅ | `NSLOCTEXT(\"[...]\", \"...\", \"Foo\")` |
| Enum | ✅ | `EEnumName::Value` |
| Struct | ✅ | `FVector(0, 0, 0)` |
| TSubclassOf / TSoftObjectPtr / TSoftClassPtr | ✅ | `TSoftObjectPtr<UTexture2D>(\"/Game/...\")` |
| TArray / TSet / TMap | ✅ | `TArray<int32>{}` |

### 그 외
| 항목 | 지원 |
|---|---|
| 부모 클래스 | ✅ |
| 구현 인터페이스 목록 | ✅ `interfaces` 필드에 기록 |

---

## 무엇을 위한 도구로 쓰면 안되는가?

- **블루프린트 백업/복원 도구가 아닙니다.**
  - JSON에서 `.uasset`을 재생성하지 않습니다.
  - 백업은 Source Control로 하세요.
- **시각적 그래프 뷰어가 아닙니다.**
  - 노드 위치·색상·코멘트를 기록하지 않습니다.
  - BP를 눈으로 보고 싶으면 에디터를 여세요.
- **런타임 도구가 아닙니다.**
  - 에디터 전용입니다.
  - 패키징된 게임에 포함되지 않습니다.
- **실시간 분석기가 아닙니다.**
  - BP를 저장할 때만 기록합니다.

---

## 트러블슈팅

**스냅샷이 안 생겨요**
- BP를 `Ctrl+S`로 실제 저장했는지 확인 (Autosave는 무시됨).
- `Project Settings > Wx > WxBlueprintSnapshot`에서 `bEnabled`가 켜져있는지.
- `IncludeDirectories`에 값이 있다면 내 BP 경로가 포함되는지.
- BP가 **Dirty / Error 상태**면 스킵됨. 컴파일 성공 후 저장 필요.
- 대상이 Blueprint Interface / Macro Library / Function Library면 의도적으로 스킵됨.

**PIE / 쿠킹 중엔 추출이 안되나요**
- 네. PIE, Cook, Autosave, Commandlet 실행 중엔 동작하지 않도록 의도했습니다.

**특정 BP만 스냅샷이 안 생겨요**
- Output Log에서 `LogWxBPSnapshot: Error` 로그를 확인하세요.
- 예상 경로가 240자를 넘으면(`Plugins/WxBlueprintSnapshot/Snapshots/...` 기준) Windows MAX_PATH(260) 제한으로 저장이 실패하므로 해당 BP는 스킵됩니다.
- 해결: 프로젝트를 더 짧은 드라이브 경로에 두거나, `Project Settings > Wx > WxBlueprintSnapshot`에서 `OutputDirectory`의 경로를 변경하세요.

---

## 설정

`Project Settings > Wx > WxBlueprintSnapshot`

| 항목 | 기본값 | 설명 |
|---|---|---|
| `bEnabled` | true | 전체 기능 on/off |
| `FileExtension` | `.json` | 스냅샷 파일 확장자. 점을 포함해 입력 (예: `.json`, `.bpj`) |
| `OutputDirectory` | `Plugins/WxBlueprintSnapshot/Snapshots` | 스냅샷 저장 루트 폴더. |
| `IncludeDirectories` | [] | 대상 BP 폴더 (비어있으면 전체) |
| `ExcludeDirectories` | [] | 제외 BP 폴더 |
| `bSkipUnchangedDefaults` | true | 기본값과 동일한 프로퍼티를 classDefaults/컴포넌트/위젯 delta에서 제외 (false로 두면 전체 덤프) |
| `bIncludeComponents` | true | Components 탭 추출 |
| `bIncludeVariables` | true | 변수 목록 추출 |
| `bIncludeInterfaces` | true | 구현 인터페이스 추출 |
| `bIncludeWidgetTree` | true | WBP 위젯 트리 추출 |
| `bIncludeMVVM` | true | WBP MVVM 바인딩 추출 |
| `bIncludeGraphs` | true | 이벤트/함수 그래프 의사코드 추출 |

---

## 출력

- **경로**: `<OutputDirectory>/<BP 패키지 경로><FileExtension>`
- **포맷**: UTF-8 (no BOM), 키 알파벳 정렬 (단 `variables`/`functions`/`components` 는 BP 선언/SCS 순서 유지), 탭 들여쓰기 pretty print (UE `TPrettyJsonPrintPolicy`)
- **ReadOnly 플래그**는 자동 해제 후 덮어쓰기 (Perforce 등에서 편의)
- **삭제 동기화**: BP 에셋을 삭제하면 대응되는 JSON 파일도 함께 제거됨
- **예시 파일**: 플러그인 Content 폴더의 `BP_SampleCharacter.uasset`에 대응되는 `Snapshots/WxBlueprintSnapshot/BP_SampleCharacter.json` 참조

---

### 최상위 JSON 필드

| 필드 | 타입 | 조건 |
|---|---|---|
| `blueprintPath` | string | 항상 |
| `parentClass` | string | 항상 |
| `classDefaults` | object | CDO와 차이점 존재 |
| `components` | object | Components 탭에 컴포넌트 1개 이상 |
| `variables` | object | 변수 1개 이상 |
| `interfaces` | object | 구현 1개 이상 |
| `eventGraph` | object | 이벤트 1개 이상 |
| `functions` | object | 함수 1개 이상 |
| `widgetTree` | object | WBP |
| `mvvm` | object | WBP + MVVM 확장 존재 |

---

## 요구사항

- Unreal Engine 5.7
- 에디터 빌드 (`WxEditor.Target.cs`)
- 의존 모듈: `ModelViewViewModel` (MVVM 추출용, 5.7 기본 포함)
