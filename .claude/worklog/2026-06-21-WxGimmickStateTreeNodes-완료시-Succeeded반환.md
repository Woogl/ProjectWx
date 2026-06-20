# WxGimmick StateTree 노드 — 작업 완료 시 Succeeded 반환(OnComplete 발화)

## 계획

### 목표
WxGimmick 공용 StateTree 태스크 6종이 지금껏 `Running`만 반환해 상태가 스스로 완료되지 않았고, 그래서 OnComplete 전이가 영영 발화하지 않았다. 각 태스크가 자기 작업을 끝낸 순간 `Succeeded`를 반환하게 해, 모든 Task가 완료됐을 때 OnComplete가 발화하도록 만든다. 엔진 순정 유지(`bConsideredForCompletion`은 코드에서 안 건드림) — 상태 완료 게이팅(`TasksCompletion=All`/완료 전이)은 에셋 오서링이 담당.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/.../Private/Gimmick/WxGimmickStateTreeNodes.cpp` | 6종 태스크가 작업 완료 시 `Succeeded` 반환(아래 규칙), `PlaySkeletalAnim::Tick` 신규 | 수정 |
| `Plugins/WxWorld/.../Public/Gimmick/WxGimmickStateTreeNodes.h` | `PlaySkeletalAnim` 생성자 선언 제거+`Tick` 선언 추가, 개요·구조체 주석을 완료 반환 취지로 정정 | 수정 |

### 접근 방식
- **즉시형(GimmickInteraction·PlayFx·TriggerSpawners)**: 틱 없음. 동작(또는 초기 진입/논권위 무동작) 직후 `EnterState`에서 `Succeeded`.
- **모션형(ComponentMove·ComponentSplineMove)**: 초기 진입·`Duration<=0`·이미 목표·포인트 0개면 `EnterState`에서 스냅 후 `Succeeded`, 라이브 슬라이드는 `EnterState` `Running`→`Tick`이 목표 도달 시 `Succeeded`.
- **애니형(PlaySkeletalAnim)**: 초기 진입 끝프레임 스냅이면 `EnterState`에서 즉시 `Succeeded`, 라이브 재생은 `Running`→`Tick`이 싱글노드 `!IsPlaying()`(재생 종료) 시 `Succeeded`. 완료 감지를 위해 틱 활성화(`bShouldCallTick=false` 제거).
- 에러(타깃 null 등)는 기존대로 `Failed` 유지.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.cpp` | 6종 태스크가 작업 완료 시 `Succeeded` 반환, `PlaySkeletalAnim::Tick` 신규(재생 종료 감지)·생성자 제거 | 수정 |
| `WxGimmickStateTreeNodes.h` | `PlaySkeletalAnim` 생성자 선언 제거+`Tick` 선언 추가, 개요·구조체 주석을 완료 반환 취지로 정정 | 수정 |

### 구현·결정과 그 이유
- **무조건 완료 반환**: 즉시형(Interaction/PlayFx/TriggerSpawners)은 진입 직후, 모션형(ComponentMove/ComponentSplineMove)은 목표 도달 시, 애니형(PlaySkeletalAnim)은 재생 종료 시 `Succeeded`를 반환한다. 초기 진입(복원/시작)은 스냅 즉시 완료해 복원 시 단계가 캐스케이드되게 했다. 타깃 포인터 null 등 에러만 기존대로 `Failed`.
- **PlaySkeletalAnim 틱 전환**: 재생 종료를 알려면 매 틱 싱글노드 재생 여부를 확인해야 해서 무틱 생성자를 없앴다(틱 기본 활성). 종료·애니 교체 시 완료로 본다.
- **완료 판정은 엔진 순정·에셋 책임**: 사용자가 `bConsideredForCompletion`을 코드에서 건드리지 않기로 해서(기본 `true`, 상태별 `TasksCompletion` 기본 `Any`), 어떤 태스크를 완료 판정에 넣을지·`All`/`Any`·완료 전이는 전적으로 에셋 오서링이 정한다. 완료 전이 없는 머무는 상태는 그 태스크를 완료 판정에서 빼야 루트 재선택 thrash(라이브 재진입 시 PlayFx 매 틱 재생 등)를 피한다 — 헤더 개요에 명시.

### 계획 대비 달라진 점
- 구현 중 사용자가 모션 2종에 `bCompleteOnReach` 옵트인 플래그를 직접 추가하는 안을 시험했다가, 플래그를 빼고 "완료 시 무조건 `Succeeded`"로 최종 결정했다. 그 실험 편집(플래그·조건 분기·GetDescription 표기)을 모두 걷어내 원안(6종 무조건 완료)으로 수렴했다.

### 후속 과제
- **에셋 오서링(사용자)**: "모든 Task 완료 시" 전이를 원하는 상태는 `Tasks Completion=All` 설정. 머무는 상태(Closed/Open/Idle 등)는 완료 전이를 추가하거나 그 태스크의 "Consider for completion"을 꺼 thrash 방지. `ST_Elevator` 등에 남았을 수 있는 `bCompleteOnReach` 저장값은 프로퍼티 제거로 로드 시 자동 폐기(무해).
- **PIE 검증 미완**: 전이형 상태가 모션·애니 종료 순간 다음 단계로 자동 전이되는지, 초기 진입(복원) fast-forward, 머무는 상태 thrash 부재, 리슨 서버 추종 확인.
- 검증: WxEditor(Development) 빌드 `Result: Succeeded`(WxWorld 컴파일·링크, 경고는 변경 무관 엔진 C4996).
