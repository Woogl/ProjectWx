# UWxAIBehaviorComponent 미사용 감각 수치 getter 제거

## 계획

### 목표
`UWxAIBehaviorComponent` 의 `GetSightRadius`/`GetSightAngle`/`GetHearingRadius` 를 지운다. 호출부가 없는 데드 코드이며, 외부 조회 경로처럼 읽혀 "컴포넌트가 컨트롤러 퍼셉션에 밀어 넣는다"는 소유 방향과 어긋난다(`/module-review` WxAI 3번 🟢).

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h` | getter 선언 3개 제거 | 삭제 |
| `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp` | getter 정의 3개 제거 | 삭제 |

### 접근 방식
- **단순 삭제**: 저장소 전체에서 호출부 0건이고 `UFUNCTION` 도 아니라 BP·에셋 참조가 불가능하므로, 대체 경로 없이 선언·정의만 지운다. 수치 멤버와 이를 직접 읽는 `ApplySenseSettings`·에디터 `TickComponent` 는 유지한다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxAI/Source/WxAI/Public/WxAIBehaviorComponent.h` | `GetSightRadius`/`GetSightAngle`/`GetHearingRadius` 선언 제거 | 삭제 |
| `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp` | 같은 세 함수 정의 제거 | 삭제 |

### 구현·결정과 그 이유
- **대체 경로 없이 삭제**: 호출부가 0건이고 리플렉션 노출도 없어 에셋이 깨질 여지가 없다. 감각 수치는 컴포넌트 안에서만 소비되므로 멤버 직접 접근으로 충분하다.
- **리뷰 문서는 손대지 않음**: `module_review_WxAI.md` 는 `/module-review` 산출물이라, 다음 실행 때 stale 로 잡혀 재생성되도록 둔다.

### 계획 대비 달라진 점
- 계획대로

### 후속 과제
- 없음 (WxEditor Development 빌드 성공, 소스에 세 이름이 남지 않음을 Grep 으로 확인)
