# Enable Npc Interaction 진입 시 대상 3회 해석 제거

## 계획

### 목표
`FWxStateTreeTask_EnableNpcInteraction::EnterState`가 대상 하나를 세 번 해석(`SyncFind`)하는 중복을 걷어내 진입 시 대상당 1회로 줄인다. 로그 문구·경고 조건·토글 동작은 그대로 둔다.

현재 대상 하나당 ① 경고 루프의 `SyncFind`, ② 같은 루프의 `FindTargetDialogue` 안 `SyncFind`, ③ `RefreshNpcInteraction`의 `FindTargetDialogue` 안 `SyncFind` 로 세 번 돈다. 셋 다 같은 프레임·같은 인자로 같은 답을 낸다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueStateTreeNodes.cpp` | `RefreshNpcInteraction`에 경고 흡수 + 해석 인라인, `FindTargetDialogue` 헬퍼 제거, `EnterState`의 경고 루프 삭제 | 수정 |

### 접근 방식
- **경고를 `RefreshNpcInteraction`으로 흡수**: 이 함수가 이미 "해석 → 컴포넌트 유무 분기"를 하고 있어 경고 판정에 필요한 정보를 그대로 쥐고 있다. 진입/틱 구분은 `bWarnNotDialogueTarget` 인자 하나로 끝난다(진입 true, 틱 false — 매 틱 로그 폭주 방지는 현행과 동일).
- **`FindTargetDialogue` 제거**: 경고가 "해석 실패(스트리밍 아웃, 정상)"와 "해석은 됐는데 대화 상대가 아님(잘못된 조립)"을 갈라야 하는데, 컴포넌트만 돌려주는 헬퍼로는 그 구분이 안 나온다. `SyncFind` → `Cast<AActor>` → `FindComponentByClass` 를 루프에 인라인하고 헬퍼는 지운다(호출부가 한 곳으로 줄어든다).
- **유지**: 빈 로케이터 경고 루프는 해석을 하지 않는 `IsEmpty` 검사라 중복 대상이 아니므로 그대로 둔다. 경고 판정을 액터가 아닌 `UObject` 기준으로 두어, 액터 아닌 오브젝트가 해석된 경우에도 지금처럼 경고가 나가게 한다.

### 범위 밖
같은 리뷰 문서의 발견 5(`FindTargetDialogue` 결과 `IsValid` 가드 부재)는 별건이라 건드리지 않는다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueStateTreeNodes.cpp` | `RefreshNpcInteraction`에 `bWarnNotDialogueTarget` 인자 추가·대상 해석 인라인·경고 흡수, `FindTargetDialogue` 헬퍼 삭제, `EnterState`의 해석 경고 루프 삭제 | 수정 |

### 구현·결정과 그 이유
- **경고를 해석하는 자리로 옮김**: 경고는 "해석 결과에 대화 컴포넌트가 없다"는 판정이라, 해석을 하는 함수 밖에서 내려면 같은 해석을 다시 해야 한다. 판정을 아는 쪽으로 책임을 옮겨 진입 경로의 `SyncFind` 3회 → 1회가 됐다.
- **헬퍼 대신 인라인**: `FindTargetDialogue`는 컴포넌트만 돌려줘 "해석 실패(스트리밍 아웃, 정상)"와 "해석은 됐는데 대화 상대가 아님(잘못된 조립)"을 가르지 못한다. 두 경우를 갈라야 경고 조건이 유지되므로 원본 오브젝트를 쥔 채 인라인했다. 호출부도 한 곳뿐이라 헬퍼로 남길 이유가 없다.
- **경고 판정을 `UObject` 기준 유지**: 액터가 아닌 오브젝트가 해석된 경우에도 이전처럼 경고가 나가야 해서 `Cast<AActor>` 전의 원본으로 판정했다.

### 계획 대비 달라진 점
계획대로.

### 후속 과제
- 없음. 같은 리뷰 문서의 발견 5(`IsValid` 가드)는 손대지 않았다.
