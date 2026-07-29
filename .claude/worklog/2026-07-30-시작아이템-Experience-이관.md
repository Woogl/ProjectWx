# 시작 아이템을 Experience 에셋으로 이관

## 계획

### 목표
시작 아이템 목록을 GameMode 프로퍼티(GM_Combat BP)에서 Experience 계열 에셋으로 옮겨, 게임 구성 데이터화를 시작 아이템까지 확장한다. 지급 시점·주체(서버 GameMode, 완료 전=일괄/완료 후=PostLogin)는 그대로 두고 목록의 출처만 바꾼다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Framework/WxExperienceDefinition.h` | DefaultInventoryItems 필드 추가 | 수정 |
| `Framework/WxExperienceActionSet.h` | 동일 필드 추가 | 수정 |
| `Framework/WxGameMode.h/.cpp` | 프로퍼티 삭제, 지급 함수가 Experience 본체+액션셋 목록을 합산해 지급 | 수정 |
| `Content/Framework/WAS_StartingItems.uasset` | 시작 아이템 전용 액션셋(DA_Potion ×1) | 신규 |
| `Content/Framework/EXP_Combat·EXP_MiniGame.uasset` | ActionSets에 WAS_StartingItems 합성 | 수정 |

### 접근 방식
- **필드는 정의·액션셋 양쪽에**: 두 클래스가 이미 액션·GameFeature 목록을 거울로 갖는 구조라 같은 패턴을 따르고, GameMode가 본체+액션셋을 이어붙인다(매니저의 GF 목록 합성과 같은 규칙).
- **데이터는 전용 에셋으로 분리**: 배선(WAS_CoreGameplay, 프로그래머 영역)과 자주 바뀔 튜닝(WAS_StartingItems, 기획 영역)의 편집 영역을 가른다. 모드 고유 아이템은 각 EXP 필드에 추가.
- **마이그레이션 안전**: C++에서 프로퍼티를 지우면 BP 저장값이 로드 시 드롭되므로, 값(DA_Potion ×1)을 사전에 확보해 두고 에셋에 재기입한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Framework/WxExperienceDefinition.h`, `WxExperienceActionSet.h` | DefaultInventoryItems 필드 추가 | 수정 |
| `Framework/WxGameMode.h/.cpp` | 프로퍼티 삭제, 지급 함수가 Experience 인자를 받아 본체+액션셋 합산 지급, 클래스 주석 갱신 | 수정 |
| `Source/WxGame/README.md` | Experience 구성에 시작 아이템 반영 | 수정 |
| `Content/Framework/WAS_StartingItems.uasset` | 시작 아이템 전용 액션셋(DA_Potion ×1) | 신규 |
| `Content/Framework/EXP_Combat·EXP_MiniGame.uasset` | ActionSets=[WAS_CoreGameplay, WAS_StartingItems] | 수정 |
| `Content/Framework/GM_Combat.uasset` | 재저장(고아가 된 구 프로퍼티 데이터 정리) | 수정 |

### 구현·결정과 그 이유
- **지급 함수가 Experience 를 인자로 받는다**: 두 호출처(완료 콜백·PostLogin)가 이미 로드 완료를 보장하는 문맥이라, 함수 내부 재조회 대신 확정된 Experience 를 넘겨 전제 의존을 명시했다.
- **값 사전 확보 후 이관**: C++ 프로퍼티 삭제로 BP 저장값이 드롭되기 전에 값(DA_Potion ×1)을 계획 단계에서 읽어 두고 재기입했다.

### 계획 대비 달라진 점
- 계획대로. 검증만 방식 변경: 퀵슬롯 화면 캡처가 불가(뷰포트 캡처 도구는 에디터 월드 재렌더 전용)해서, PIE 월드의 아이템 인스턴스 오브젝트(WxItemInstance_0, 정확히 1개)의 존재로 지급을 확인했다.

### 후속 과제
- 없음.
