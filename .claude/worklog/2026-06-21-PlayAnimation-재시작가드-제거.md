# FWxStateTreeTask_PlayAnimation — "같은 애니 재생 중 재시작 안 함" 가드 제거

## 계획

### 목표
`FWxStateTreeTask_PlayAnimation::EnterState`의 라이브 전이 분기에서 `bAlreadyPlaying` 가드를 제거해, 라이브 진입 시 항상 처음부터 재생하게 한다. 가드는 본래 "재진입 프레임 0 끊김 방지"용이라, 제거 후엔 같은 상태 라이브 재진입 시 매번 처음부터 재생된다(끊김 가능). 초기 진입(끝 프레임 스냅)은 불변.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Private/Gimmick/WxGimmickStateTreeNodes.cpp` | 라이브 분기의 `SingleNode`/`bAlreadyPlaying` 가드 제거 → 무조건 `PlayAnimation(..., false)`, 분기 주석 정정 | 수정 |
| `Plugins/WxWorld/.../Public/Gimmick/WxGimmickStateTreeNodes.h` | 개요·구조체 doc 주석에서 "재시작 안 함" 표현 제거 | 수정 |

### 접근 방식
- 라이브 분기를 무조건 `Mesh->PlayAnimation(Instance.Animation, false)` + `Running` 으로 단순화.
- `AnimSingleNodeInstance.h` include는 `Tick`이 여전히 종료 감지에 쓰므로 유지.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.cpp` | 라이브 분기의 `SingleNode`/`bAlreadyPlaying` 가드 제거 → 무조건 `Mesh->PlayAnimation(Instance.Animation, false)`, 분기 주석 정정 | 수정 |
| `WxGimmickStateTreeNodes.h` | 개요·구조체 doc 주석에서 "이미 같은 애니 재생 중이면 재시작 안 함" 표현 제거 | 수정 |

### 구현·결정과 그 이유
- **가드 제거**: 라이브 전이 진입 시 같은 애니를 재생 중이어도 무조건 처음부터 다시 재생하도록 단순화했다. 사용자 요청.
- **include 유지**: `AnimSingleNodeInstance.h`는 `Tick`이 여전히 `GetSingleNodeInstance()`로 재생 종료를 감지하므로 그대로 뒀다.
- **검증**: WxEditor(Development) 빌드 `Result: Succeeded`. WxWorld 컴파일·링크 완료, 경고는 변경과 무관한 엔진 C4996뿐.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- **PIE 검증 미완(사용자)**: 같은 애니 상태로 라이브 재진입(루트 재선택 등) 시 프레임 0부터 재생되는지·끊김이 허용 범위인지 확인. 끊김이 문제면 가드 복원 또는 상태 오서링으로 재진입 자체를 줄이는 방향 검토.
