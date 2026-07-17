# PrimaryGameLayout 레이어 정의 유연화

## 계획

### 목표
`UWxPrimaryGameLayout`의 레이어 집합이 4곳(WxCore 태그 · 헤더 `BindWidget` 멤버 · `NativeOnInitialized`의 `LayerMap.Add` · WBP의 동명 스택)에 lockstep으로 하드코딩되어 레이어 추가/재정렬이 경직적이다. 소비자는 모두 태그 기반 제네릭이라 특정 레이어를 분기하지 않으므로, 정의를 **레이아웃 클래스가 소유하는 선언적 태그 배열** 하나로 접어 "레이어 추가 = 태그 1줄 + 배열 1항목"이 되게 한다. (런타임 추가/제거는 목표 아님.)

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` | 4개 `BindWidget` 스택 멤버 제거, 단일 루트 `LayerContainer`(`UOverlay`, `BindWidget`)·`LayerTags`(`TArray<FGameplayTag>`, `EditDefaultsOnly`)·생성자 선언 추가, `class UOverlay;` 전방선언 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp` | 생성자에서 `LayerTags` 기본값(현재 4태그) 시드, `NativeOnInitialized`가 태그 순회로 스택을 동적 생성해 `LayerContainer`에 채우고 `LayerMap` 구성, 인클루드 3종 추가 | 수정 |
| `Content/UI/Widget/WBP_PrimaryGameLayout.uasset` | 4개 스택 삭제, 루트를 `LayerContainer` 이름의 `Overlay`로 교체 (에디터 작업) | 수정 |
| `Plugins/WxBlueprintSnapshot/Snapshots/WxUI/WBP_PrimaryGameLayout.json` | WBP 저장 시 자동 재생성 | 수정 |

**불변**: 공개 API(`GetLayerWidgetStack`/`GetLayerMap`/`Push*`), `LayerMap` 타입, `UWxUIManagerSubsystem`(`RefreshGamePause`), `UWxUIDeveloperSettings`, WxCore `UI.Layer.*` 태그 4종, 모든 push 호출부.

### 접근 방식
- **데이터 기반 태그 목록**: 레이어를 정의 시점의 `TArray<FGameplayTag> LayerTags`(배열 순서 = z-order, 0=최하단)로 표현. 생성자가 현재 4태그로 기본값을 채우고, WBP Class Defaults에서 재정의 가능.
- **초기화 시 동적 구성**: `NativeOnInitialized`에서 각 태그마다 `WidgetTree->ConstructWidget<UCommonActivatableWidgetStack>()`로 스택을 만들어 `LayerContainer`(Overlay)에 Fill로 붙이고 `LayerMap.Add`. 스택은 `UWidget`이라 `CreateWidget`이 아닌 `WidgetTree->ConstructWidget`를 쓴다.
- **동작 보존**: 새 스택 CDO 기본 `TransitionDuration`(0.4s)을 `SetTransitionDuration(0.f)`로 되돌려 현재의 즉시전환 유지. Overlay 슬롯 기본 정렬이 Left/Top이므로 `HAlign_Fill`/`VAlign_Fill` 명시(누락 시 좌상단 축소 회귀).
- **구조체 대신 태그 배열**: 승인 프리뷰의 `FWxUILayerDefinition{ Tag, StackClass }`는 4레이어가 전부 동일 스택·즉시전환이라 `StackClass`가 투기적 → 태그 배열로 단순화. 레이어별 스택 커스터마이즈가 실제 필요해지면 그때 구조체로 승격.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxUI/Source/WxUI/Public/System/WxPrimaryGameLayout.h` | 4개 `BindWidget` 스택 멤버 제거; `LayerContainer`(`UOverlay`)·`LayerTags`(`TArray<FGameplayTag>`)·생성자 선언 추가; `class UOverlay;` 전방선언 | 수정 |
| `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp` | 생성자에서 `LayerTags`를 현재 4태그로 시드; `NativeOnInitialized`가 태그를 순회해 스택을 동적 생성·부착하고 `LayerMap` 구성; 인클루드 3종 추가 | 수정 |
| `Content/UI/Widget/WBP_PrimaryGameLayout.uasset` | 4개 스택 삭제, 루트를 `LayerContainer` 이름의 `Overlay`로 교체 | 수정 예정(에디터) |

### 구현·결정과 그 이유
- **레이어를 순서 있는 태그 목록으로 정의**: 4개 레이어가 모두 동일한 스택·즉시전환이라 레이어별 스택 클래스를 담는 구조체는 투기적. 태그 목록만으로 "레이어 추가 = 한 줄"을 가장 단순하게 달성하고, 커스터마이즈가 실제 필요해지면 그때 구조체로 승격.
- **스택을 초기화 시점에 동적 생성**: 스택 컨테이너는 유저위젯이 아니라 위젯 트리로 직접 만들어 루트 컨테이너에 붙인다. 슬레이트 빌드 전이라도 큐잉되어 안전하고, WBP에 스택을 authoring할 필요가 사라진다.
- **현재 표시 동작 보존**: 동적 생성 시 전환 페이드와 슬롯 정렬 기본값이 기존과 달라, 즉시전환과 전체 화면 채움을 각각 명시해 시각 회귀를 차단.

### 계획 대비 달라진 점
- 지역 변수명 하나를 바꿔 베이스 위젯 멤버 섀도잉(경고=에러)으로 인한 빌드 실패를 해소. 그 외 계획대로.

### 후속 과제
- **WBP 교체(필수·에디터 작업)**: 기존 4스택을 지우고 루트를 `LayerContainer` 이름의 Overlay로. 미완료 시 `BindWidget` 실패로 WBP 컴파일 에러·런타임 UI 미표시.
- **엔드투엔드 미검증**: HUD·메뉴·모달·데스스크린 표시, 메뉴 진입 시 일시정지, 전체 화면 정렬 확인.
