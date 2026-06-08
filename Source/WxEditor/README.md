# WxEditor — 에디터 확장 모듈

> 언리얼 에디터 전용 소스 모듈. 디테일 패널에서 `Wx` 접두사 카테고리를 최상단으로 끌어올려 작업 편의를 높이는 커스터마이징을 등록한다.

## 책임
**담당**
- 모듈 로드 시 `PropertyEditor` 에 디테일 커스터마이징을 등록/해제 (`FWxEditorModule`)
- `Wx`로 시작하는 디테일 카테고리를 디테일 패널 최상단에 사전순 정렬 (`FWxCategoryDetailCustomization`)

**경계 (비담당)**
- 런타임 게임 로직 일체 — 에디터 전용 모듈이며 패키지 빌드에 포함되지 않는다.

## 의존성
- **주요 의존**: `UnrealEd`, `PropertyEditor`, `Slate`/`SlateCore` (에디터 디테일 패널 커스터마이징). Wx 모듈 의존 없음.
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅ (Wx 모듈을 전혀 참조하지 않음)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `FWxEditorModule` | 모듈 진입점. `StartupModule`에서 `UObject::StaticClass()`에 커스터마이징 등록 | `Source/WxEditor/WxEditor.h` |
| `FWxCategoryDetailCustomization` | `IDetailCustomization` 구현. `Wx*` 최상위 카테고리를 음수 SortOrder로 상단 정렬 | `Source/WxEditor/WxCategoryDetailCustomization.h` |

## 확장 포인트 / 규약
- 새 디테일/프로퍼티 커스터마이징은 `FWxEditorModule::StartupModule`에서 `PropertyModule.RegisterCustomClassLayout`(또는 `RegisterCustomPropertyTypeLayout`)으로 등록하고, `ShutdownModule`에서 대칭으로 해제한다.
- 카테고리 정렬 기준은 `WxCategory::CategoryPrefix`(`"Wx"`)와 `BaseSortOrder`(`-10000`)로 제어한다. Transform 등 엔진 기본 카테고리보다 위에 두려고 충분히 작은 베이스를 쓴다.
- 서브카테고리(`Wx|Input` 등)는 최상위 세그먼트만 추출해 정렬한다. 서브카테고리에 직접 `EditCategory`를 호출하면 디테일 패널이 중복 렌더링되므로 주의.

## 여기서부터 읽어라
1. `Source/WxEditor/WxEditor.cpp` — 모듈 등록/해제 흐름. 무엇이 어디에 붙는지 파악
2. `Source/WxEditor/WxCategoryDetailCustomization.cpp` — 카테고리 정렬 로직과 중복 렌더링 회피 주석

## 관련
- 상위: 각 Wx 플러그인이 노출하는 `Wx*` 카테고리(예: GAS 컴포넌트의 디테일)의 에디터 표시 품질을 책임진다. 정렬 대상 카테고리를 정의하는 곳은 각 도메인 플러그인 측.

---
*문서 기준 커밋 `59bfe3f` · 생성일 2026-06-08 · 소스 4파일 — `/readme-writer`로 갱신*
