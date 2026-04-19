# WxBlueprintSnapshot

블루프린트를 저장할 때마다 **그 안의 내용**(기본값, 컴포넌트, 변수, 그래프 로직)을 사람과 AI가 읽을 수 있는 JSON 파일로 자동 기록합니다.

## 이 플러그인으로 할 수 있는 것

### 1. 블루프린트 변경점을 diff로 읽기
`.uasset`은 바이너리라 source control에서 `Binary files differ`만 뜹니다. 이 플러그인이 생성하는 JSON은 **텍스트 + 키 정렬** 포맷이라, PR 리뷰에서 "이 커밋에서 캐릭터 BP의 MaxHP가 얼마로 바뀌었는지" 같은 변경점을 줄 단위로 볼 수 있습니다.

### 2. AI에게 블루프린트 구조 전달
생성된 JSON은 BP의 **구조·설정·대략적 로직 흐름**을 텍스트로 담기 때문에 AI 코드 어시스턴트에 붙여넣어 질문·리뷰 보조로 쓸 수 있습니다. 스크린샷보다 파싱 정확도가 높습니다.

### 3. 블루프린트 일별 변화 추적
JSON 파일이 리포지토리에 커밋되므로 "누가 언제 이 BP의 컴포넌트를 추가/제거했는지"를 추적할 수 있습니다.

### 4. 블루프린트 일괄 검수
JSON이라 **전체 프로젝트 BP를 스캔**할 수 있습니다.  
예: "Tick이 켜진 Actor BP 찾기", "특정 인터페이스를 구현한 BP 전체 목록".

---

## Quick Start

1. 플러그인 활성화 (기본값으로 이미 활성화 상태).
2. `Project Settings > Wx > WxBlueprintSnapshot`에서 대상 폴더를 `IncludeDirectories`에 지정 (비워두면 전체 BP).
3. 블루프린트를 저장 (Ctrl+S).
4. `Plugins/WxBlueprintSnapshot/Snapshots/<BP 경로>.json` 파일이 생성/업데이트됨.
5. 이 폴더를 Git에 커밋하면 이후 변경점이 PR에 텍스트로 찍힘.

---

## 어떤 블루프린트가 대상인가?

| 블루프린트 종류 | 지원 | 비고 |
|---|---|---|
| UObject Blueprint | ✅ | Character, Actor, GameplayAbility 등 UObject를 상속한 모든 BP |
| Widget Blueprint (UMG) | ✅ | 위젯 트리 + MVVM 바인딩까지 추출 |
| Animation Blueprint | ⚠️ | 변수·기본값까지만. AnimGraph 스테이트머신은 노드 타이틀만 찍힘 |
| Data Asset | ❌ | 블루프린트가 아닌 UObject 에셋 |
| Data Table / Curve Table | ❌ | CSV/JSON Export 사용 권장 |
| Blueprint Interface | ❌ | 함수 시그니처만 있는 BP. 스킵 |
| Blueprint Function Library | ❌ | 스킵 |
| Editor Utility Blueprint | ❌ | 스킵 |
| Control Rig / Niagara / Metasound | ❌ | 전용 그래프 포맷, 미지원 |

---

## 블루프린트의 어떤 데이터가 추출되는가?

### 기본값 (Details 패널의 값)
| 항목 | 지원 |
|---|---|
| 변수 기본값 변경 (숫자, 문자열, 불리언) | ✅ |
| 에셋 참조 (StaticMesh, Material, 등) | ✅ 경로 문자열로 |
| Struct 멤버 | ✅ 재귀 기록 |
| TArray, TSet 원소 | ✅ 전체 덤프 |
| TMap 엔트리 | ✅ |
| 인스턴스드 서브오브젝트 (e.g. UObject 필드) | ✅ 재귀 기록 |
| `EditAnywhere`/`BlueprintReadWrite` 속성 | ✅ |
| 순수 C++ 내부 상태 (`VisibleAnywhere`만 아닌 것) | ❌ 기록 안 함 |
| `Transient` / `Deprecated` 속성 | ❌ 의도적으로 제외 |
| FText 로컬라이제이션 키/네임스페이스 | ❌ 보이는 문자열만 저장 |

### 컴포넌트 (Components 탭에 추가한 것)
| 항목 | 지원 |
|---|---|
| Add Component으로 붙인 컴포넌트 | ✅ |
| 컴포넌트 간 어태치(부모 + 소켓) | ✅ |
| 컴포넌트별 Details 값 | ✅ CDO 차이만 |
| C++ 생성자에서 만든 네이티브 컴포넌트 | ❌ BP 소유가 아님 |
| 상속된 컴포넌트의 Override 값 | ⚠️ 일부만 |

### 위젯 블루프린트 (WBP)
| 항목 | 지원 |
|---|---|
| Designer 탭의 위젯 계층 | ✅ |
| 각 위젯의 Details 값 + Slot 값 | ✅ |
| MVVM ViewModel 바인딩 | ✅ Binding Type, Source, Destination 모두 지원 |
| MVVM Conversion Function | ✅ 함수 경로만 |
| Widget Animation | ❌ |
| UMG 구식 Property Binding | ❌ MVVM만 지원 |
| Named Slot의 내용물 | ❌ 런타임 주입이라 BP엔 없음 |

### 이벤트/함수 그래프 (의사코드)

이벤트 그래프는 `eventGraph` 필드에 하나의 배열로 평탄화되고, 함수 그래프는 `newFunctions` 필드에 함수 이름을 키로 한 오브젝트로 기록된다.

| 항목 | 지원 |
|---|---|
| Event, Custom Event | ✅ |
| Branch (if/else) | ✅ |
| Sequence | ✅ |
| 변수 Set | ✅ |
| 함수 호출 (CallFunction) | ✅ |
| Cast | ✅ |
| ForEach / ForLoop / While | ✅ |
| Return | ✅ |
| Delay, Timeline, Gate | ⚠️ 노드 이름만 |
| Async / Latent Action | ⚠️ 완료 분기 해석 없음 |
| Event Dispatcher | ⚠️ 일반 함수 호출처럼 보임 |
| 매크로 내부 본문 | ❌ 호출 라인만 |
| 로컬 변수 | ❌ |
| 코멘트 박스 | ❌ |
| 노드 위치/색상 | ❌ |

### 그 외
| 항목 | 지원 |
|---|---|
| 부모 클래스 | ✅ |
| 변수 목록 + 타입 + 기본값 | ✅ `newVariables` 필드에만 기록 (중복 방지 위해 `classDefaults` 델타에서는 제외) |
| 구현 인터페이스 목록 | ✅ |

---

## 무엇을 위한 도구로 쓰면 안되는가?

- **블루프린트 백업/복원 도구가 아닙니다.** JSON에서 `.uasset`을 재생성하지 않습니다. 백업은 Source Control로 하세요.
- **시각적 그래프 뷰어가 아닙니다.** 노드 위치·색상·코멘트를 기록하지 않습니다. BP를 눈으로 보고 싶으면 에디터를 여세요.
- **런타임 도구가 아닙니다.** 에디터 전용입니다. 패키징된 게임에 포함되지 않습니다.
- **실시간 분석기가 아닙니다.** BP를 저장할 때만 기록합니다.

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

**JSON이 깨져보여요 / 경로가 이상해요**
- Windows 경로 260자 제한 회피를 위해 240자 초과 시 해시 폴더(`_long_path_hash/`)로 폴백합니다.

---

## 설정

`Project Settings > Wx > WxBlueprintSnapshot`

| 항목 | 기본값 | 설명 |
|---|---|---|
| `bEnabled` | true | 전체 기능 on/off |
| `IncludeDirectories` | [] | 대상 BP 폴더 (비어있으면 전체) |
| `ExcludeDirectories` | [] | 제외 BP 폴더 |
| `bIncludeUnchangedDefaults` | false | 기본값과 동일한 프로퍼티도 classDefaults/컴포넌트/위젯 delta에 기록 |
| `bIncludeComponents` | true | Components 탭 추출 |
| `bIncludeVariables` | true | 변수 목록 추출 |
| `bIncludeInterfaces` | true | 구현 인터페이스 추출 |
| `bIncludeWidgetTree` | true | WBP 위젯 트리 추출 |
| `bIncludeMVVM` | true | WBP MVVM 바인딩 추출 |
| `bIncludeGraphs` | true | 이벤트/함수 그래프 의사코드 추출 |

---

## 출력

- **경로**: `Plugins/WxBlueprintSnapshot/Snapshots/<BP 패키지 경로>.json`
- **포맷**: UTF-8, 키 정렬, 들여쓰기 2-space
- **ReadOnly 플래그**는 자동 해제 후 덮어쓰기 (Perforce 등에서 편의)

---

## 요구사항

- Unreal Engine 5.7
- 에디터 빌드 (`WxEditor.Target.cs`)
- 의존 모듈: `ModelViewViewModel` (MVVM 추출용, 5.7 기본 포함)
