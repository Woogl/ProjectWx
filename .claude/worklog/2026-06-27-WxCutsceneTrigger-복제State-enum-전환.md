# WxCutsceneTrigger를 복제 State enum 패턴으로 전환

## 계획

### 목표
전 기믹 중 유일하게 StateTree 이벤트(`SendStateTreeEvent(PlayEventTag)`)로 구동되던 `AWxCutsceneTrigger`를, 다른 기믹과 동일한 「C++ 권위 State enum → GimmickStateTree 추종」 패턴으로 통일한다. 이로써 기믹 공통 "이벤트 태그 없음" 원칙의 마지막 예외가 사라진다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxCutsceneTrigger.h` | `EWxCutsceneTriggerState{Idle,Playing}`, `State`(ReplicatedUsing=OnRep_GimmickState, SaveGame 제외), `SetGimmickState`·`HandleLevelSequenceFinished` override, `GetLifetimeReplicatedProps`, `PlayEventTag`·`GameplayTagContainer.h` 제거, doc 재작성 | 수정 |
| `WxCutsceneTrigger.cpp` | `DOREPLIFETIME`, `HandleInteracted`→권위 시 `CommitGimmickState(Playing)`, `HandleLevelSequenceFinished`→`CommitGimmickState(Idle)`, include 정리 | 수정 |
| `WxGimmick.h` | public no-op 가상 훅 `HandleLevelSequenceFinished()` 추가, State 쓰기 주체 doc 보정 | 수정 |
| `WxGimmickStateTreeNodes.{cpp,h}` | `WxGimmick.h` include 복구, PlayLevelSequence 종료 시 권위에서 소유 기믹의 `HandleLevelSequenceFinished` 통지(`NotifyHostSequenceFinished`), 관련 doc 보정 | 수정 |
| `Plugins/WxWorld/README.md` | State 구동 패턴의 컷신 예외 문장 제거 | 수정(문서) |

### 접근 방식
- **상태 흐름**: `Idle ──상호작용(권위)──> Playing ──재생 종료 통지(권위)──> Idle`. State 쓰기는 액터(C++)만, ST는 추종만.
- **재생은 ST 태스크가, 복귀는 베이스 훅 통지로**: 재생은 기존 `Wx Play Level Sequence` 태스크 그대로 쓴다(액터는 재생 코드 없음). 재생 종료를 아는 주체는 그 태스크뿐이라, 태스크가 종료 시 권위 측에서 소유 기믹의 `AWxGimmick::HandleLevelSequenceFinished()`(public no-op)를 호출하고, 컷신만 오버라이드해 `CommitGimmickState(Idle)`. "Set State 태스크가 State를 쓰는" 안과 "인터페이스" 안을 모두 기각하고, 공용 코드 최소 변경(no-op 훅 1개 + 태스크 통지)으로 정했다.
- **사용자의 베이스 리팩터에 정렬(이벤트 기반)**: 작업 중 베이스가 「Enum Compare 폴링」에서 「`Event.GimmickStateChanged` 발행 + OnEvent 전이」로 바뀌었다. 컷신 State도 `ReplicatedUsing=OnRep_GimmickState`(베이스 OnRep 재사용)로 선언해, 권위는 `SetGimmickState`, 클라는 OnRep이 같은 이벤트를 발행하게 했다(WxSpawnConsole과 동형).
- **State 영속 제외**: Playing은 일시 상태라 SaveGame 미지정. 복원 시 재생 재트리거 방지, 항상 Idle로 시작.

```mermaid
sequenceDiagram
    autonumber
    participant Comp as WxInteractionComponent
    participant Actor as WxCutsceneTrigger (권위 State)
    participant Task as Wx Play Level Sequence (ST)
    Comp->>Actor: OnInteracted → HandleInteracted
    Actor->>Actor: HasAuthority → CommitGimmickState(Playing)
    Note over Task: State==Playing 추종 → 재생 시작, Tick 으로 IsPlaying 폴링
    Task->>Actor: 종료(권위) → HandleLevelSequenceFinished
    Actor->>Actor: CommitGimmickState(Idle)
    Note over Task: State==Idle 추종 → 종료
```

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxCutsceneTrigger.h` | `EWxCutsceneTriggerState{Idle,Playing}`, `State`(ReplicatedUsing=OnRep_GimmickState, SaveGame 제외, AllowPrivateAccess), public `HandleLevelSequenceFinished` override + protected `SetGimmickState` override, `GetLifetimeReplicatedProps`, `PlayEventTag`·`GameplayTagContainer.h` 제거, doc 재작성 | 수정 |
| `WxCutsceneTrigger.cpp` | `DOREPLIFETIME`, `HandleInteracted`→권위 시 `CommitGimmickState(Playing)`, `HandleLevelSequenceFinished`→`CommitGimmickState(Idle)`, LevelSequence 재생 코드 없음(태스크가 담당), include 정리 | 수정 |
| `WxGimmick.h` | public no-op 가상 훅 `HandleLevelSequenceFinished()` 추가, 상태 구동 패턴·CommitGimmickState doc를 "인터랙션 핸들러 등 액터 측 콜백"으로 보정 | 수정 |
| `WxGimmickStateTreeNodes.cpp` | `Gimmick/WxGimmick.h` include 복구, 익명 헬퍼 `NotifyHostSequenceFinished`(권위 시 호스트 훅 호출), `PlayLevelSequence` EnterState(라이브 즉시완료)·Tick(재생 종료) 양쪽에서 통지, 초기 진입은 통지 안 함 | 수정 |
| `WxGimmickStateTreeNodes.h` | PlayLevelSequence 개요·struct doc를 "종료 시 소유 기믹에 통지" 모델로 보정 | 수정(주석) |
| `Plugins/WxWorld/README.md` | State 구동 패턴의 컷신 예외 문장 제거 | 수정(문서) |

### 구현·결정과 그 이유
- **마지막 예외를 표준 State 패턴으로 통일**: 컷신만 ST 이벤트로 구동되던 것을 복제 State enum 추종으로 바꿔 전 기믹이 일관된다. 정방향(상호작용→권위 commit)·State 선언은 `AWxSpawnConsole`과 동형(SaveGame만 제외).
- **복귀: 태스크가 호스트에 통지(베이스 public no-op 훅)**: 재생을 ST 태스크에 그대로 두고(액터는 재생 코드 없이 얇게 유지), 종료 신호만 태스크→액터로 올린다. "Set State 태스크(제거됨)", "인터페이스 신설", "액터가 재생 직접 소유" 셋을 논의한 끝에, 공용 코드 변경이 가장 작은(베이스 no-op 훅 1개 + 태스크 통지) 안을 택했다. 훅은 외부(태스크)가 호출하는 진입점이라 `CommitGimmickState`처럼 public, 구현 안 한 기믹은 노옵이라 무해하다.
- **OnFinished 대신 Tick 폴링은 태스크에 그대로**: 태스크가 이미 `OnFinished` 콜백 중 시퀀스 액터 파괴(UAF)를 피하려 Tick 폴링→다음 틱 정리를 쓰고 있어, 통지를 그 안전 지점(Tick 종료 처리)에 얹기만 했다. 라이브 진입인데 재생할 게 없으면(시퀀스 부재·플레이어 생성 실패) EnterState가 곧장 통지해 호스트가 Playing에 갇히지 않게 했다(초기 진입/복원은 통지 안 함).
- **이벤트 기반 베이스에 정렬**: 베이스가 작업 중 「OnRep_GimmickState가 Event.GimmickStateChanged 발행 → ST OnEvent 재선택」으로 바뀌어, 컷신 State를 `ReplicatedUsing=OnRep_GimmickState`로 선언해 권위·클라가 같은 통지를 공유하게 했다.
- **SaveGame 제외**: Playing은 일시 상태라 저장이 무의미하고 복원 시 재생 재트리거 위험이 있어, 다른 기믹과 달리 Replicated만 두고 항상 Idle로 시작한다.

### 계획 대비 달라진 점
- **복귀 메커니즘이 ST `Wx Set State`에서 베이스 훅 통지로 전환**: 최초 계획은 ST의 `Wx Set State(Idle)`였으나 작업 중 그 태스크가 제거됐고, 이어 "액터가 재생 직접 소유(B)" 안을 한 번 구현했다가 사용자가 "기존 `Wx Play Level Sequence` 태스크를 쓰고 싶다"고 해 현재의 「태스크 재생 + 베이스 no-op 훅 통지(A)」로 최종 전환했다. 그 과정에서 액터의 재생 소유 코드(Tick·OnRep·Start/StopPlayback)는 전부 걷어내 다시 얇아졌다.
- **베이스 패턴이 이벤트 기반으로 바뀜(사용자 동시 작업)**: 컷신을 거기에 맞춰 `ReplicatedUsing=OnRep_GimmickState`로 정렬했다. WxGimmick.cpp·WxSpawnConsole 등 베이스/타 기믹의 이벤트 전환 자체는 사용자 작업이라 이번 범위 밖.
- 검증: WxEditor(Development) 빌드 `Result: Succeeded`(경고는 변경 무관한 엔진 C4996뿐).

### 후속 과제
- **에디터 작업(사용자)**: `ST_CutsceneTrigger.uasset` 재오서링 — Idle/Playing 상태를 State로 선택(이벤트 기반 OnEvent 재선택), Playing에서 `Wx Play Level Sequence(LevelSequence)` + `Wx Enable Interaction(false)` + `Wx Enable Player Input(false)`. 기존 이벤트(`PlayEventTag`)·OnComplete-직접복귀 전이 제거(복귀는 태스크 통지가 구동).
- **BP_CutsceneTrigger**: C++ 멤버 제거로 orphan된 `PlayEventTag` 디폴트 정리, StateTree 에셋 할당 확인. 저장 시 스냅샷 자동 갱신.
- **PIE 검증 미완(컴파일만 확인)**: 상호작용 시 재생·입력 잠금, 종료 후 입력 복구·재상호작용(반복), 리슨 서버 2인에서 클라가 복제 State 추종. 시퀀스 미지정 시 Playing에 안 갇히는지(EnterState 즉시 통지).
