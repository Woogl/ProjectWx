# WxGimmick 화살표를 PlayerStart와 동일한 크기·색으로

## 계획

### 목표
`AWxGimmick`의 에디터 전용 정면 표시 `ArrowComponent`를 엔진 `PlayerStart` 화살표와 동일한 색·겉보기 크기로 맞춘다. 레벨에 기믹과 PlayerStart가 섞여 있을 때 일관된 외형을 주기 위함이다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmick.cpp` | 생성자 `#if WITH_EDITORONLY_DATA` 블록의 화살표 설정을 PlayerStart 값으로 변경 | 수정 |

### 접근 방식
- **엔진 원본 값 복제**: `APlayerStart` 생성자(`Engine/Private/PlayerStart.cpp`)가 기본값과 다르게 만드는 것은 색상과 화면 크기 스케일이다.
  - `ArrowColor`: `FColor(255, 200, 0)`(노랑) → `FColor(150, 200, 255)`(연하늘색)
  - `bIsScreenSizeScaled = true` 추가 — 겉보기 "크기"의 핵심(화면 거리와 무관하게 일정 크기)
  - `ArrowSize = 1.0f` 명시 추가(기본값과 동일)
  - `bTreatAsASprite = true`는 유지
- `SpriteInfo.Category/DisplayName`(네비 스프라이트 분류)은 크기·색과 무관하므로 건드리지 않는다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmick.cpp` | 생성자 화살표 색을 연하늘색으로 변경, `ArrowSize=1.0f`·`bIsScreenSizeScaled=true` 추가 | 수정 |

### 구현·결정과 그 이유
- **PlayerStart 원본 값을 그대로 복제**: `APlayerStart`가 `UArrowComponent` 기본값과 다르게 만드는 항목은 색상(`150,200,255`)과 `bIsScreenSizeScaled=true` 둘뿐이다. 후자가 화면 거리와 무관한 일정 겉보기 크기를 만들어 사용자가 말한 "크기"의 핵심이다. `ArrowSize`는 기본값(1.0)과 같지만 PlayerStart가 명시하므로 동일하게 명시했다.
- **`SpriteInfo`는 건드리지 않음**: 네비게이션 스프라이트 분류는 크기·색과 무관해 요청 범위 밖이다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 없음. (에디터 육안 확인은 선택 사항)
