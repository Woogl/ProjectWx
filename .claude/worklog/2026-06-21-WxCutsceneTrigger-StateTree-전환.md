# WxCutsceneTrigger StateTree 전환 + LevelSequence 재생 공용 태스크 신설

## 계획

### 목표
`AWxCutsceneTrigger`를 다른 기믹과 동일한 「권위 State enum → StateTree 추종」 패턴으로 전환하고, LevelSequence 재생을 어떤 기믹이든 재사용할 공용 StateTree 태스크로 추출한다. 컷신은 반복 재생 가능(현 동작 유지), 재생 태스크는 입력잠금 옵션을 가진 공용 노드로 만든다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxLevelSequencePlaybackHost.h` | 재생 종료를 통지받는 경량 UINTERFACE `IWxLevelSequencePlaybackHost{HandleLevelSequenceFinished()}` | 신규 |
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` | `FWxStateTreeTask_PlayLevelSequence`(+InstanceData) 선언, forward decl·개요 주석 추가 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp` | 태스크 EnterState/Tick/ExitState/GetDescription, `SetLocalPlayerInputEnabled` 익명 헬퍼, include 추가 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxCutsceneTrigger.h` | `EWxCutsceneTriggerState` enum, 인터페이스 구현, State(Replicated) 멤버, 구식 멤버·메서드 제거 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxCutsceneTrigger.cpp` | HandleInteracted/HandleLevelSequenceFinished/SetCutsceneState/GetLifetimeReplicatedProps, 구식 재생 로직 제거 | 수정 |
| `Plugins/WxWorld/README.md` | 공통 태스크 목록에 `PlayLevelSequence` 추가 | 수정 |

### 접근 방식
- **반복 모델의 상태 흐름**: `Idle ──상호작용(권위)──> Playing ──재생종료(권위 통지)──> Idle`. 전이는 오직 복제 State 변화 → ST Enum Compare 가 구동(ST 자체 OnComplete 전이 없음). Playing 상태가 재생 태스크 + 인터랙션 비활성을 담는다.
- **종료 통지(난점 해소)**: 재생 종료를 아는 주체는 로컬에서 재생을 폴링하는 태스크뿐이다. 태스크가 종료 시 권위 측에서 `IWxLevelSequencePlaybackHost`(Owner) 로 직접 발행하고, 호스트(컷신 액터)가 권위 State 를 Idle 로 되돌린다(클라는 복제 추종). 신호의 계산 원천이 직접 발행하는 패턴.
- **공용 재생 태스크**: 초기 진입(복원)이면 침묵·즉시 완료, 라이브 진입이면 `CreateLevelSequencePlayer`+`Play`, `Tick`에서 `IsPlaying()` 폴링으로 종료 감지(PlayAnimation 동형). `bLockLocalPlayerInput` 옵션으로 재생 중 로컬 입력 잠금. 입력 복구·cleanup 은 종료 시점과 `ExitState`(중도 이탈·액터 파괴 안전망)에서 멱등 처리.
- **State 영속 제외**: Playing 은 일시 상태라 SaveGame 미지정(다른 기믹과 다른 점). Replicated 만 둬 멀티 동기화.
- **모듈/검증**: `LevelSequence`/`MovieScene` 의존성은 WxWorld.Build.cs 에 이미 있어 추가 없음. ST_CutsceneTrigger 에셋 생성·BP 할당은 사용자 에디터 후속 작업.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxLevelSequencePlaybackHost.h` | 재생 종료 통지용 UINTERFACE `IWxLevelSequencePlaybackHost{HandleLevelSequenceFinished()}` | 신규 |
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` | `FWxStateTreeTask_PlayLevelSequence`(+InstanceData) 선언, forward decl·개요 주석 추가 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp` | 태스크 EnterState/Tick/ExitState/GetDescription, `SetLocalPlayerInputEnabled`·`FinishSequencePlayback` 익명 헬퍼, include 추가 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxCutsceneTrigger.h` | `EWxCutsceneTriggerState` enum, 인터페이스 구현, State(Replicated) 멤버, 구식 멤버·메서드 제거 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxCutsceneTrigger.cpp` | HandleInteracted/HandleLevelSequenceFinished/SetCutsceneState/GetLifetimeReplicatedProps, 구식 재생 로직 제거 | 수정 |
| `Plugins/WxWorld/README.md` | 공통 태스크 목록에 `PlayLevelSequence` 추가 | 수정 |

### 구현·결정과 그 이유
- **반복 모델의 전이는 오직 복제 State 변화로**: ST 자체 OnComplete 전이를 쓰지 않고 `Idle↔Playing` 복제 State 변화만으로 전이를 구동했다. ST 가 자율 전이하면 권위 State 와 어긋나는데, 이 기믹 패턴은 「C++ 권위 State → ST 추종」 단방향이라 그 어긋남을 원천 차단해야 했다.
- **재생 종료를 노드가 호스트에 직접 발행**: 재생 종료를 아는 주체는 로컬에서 시퀀스를 폴링하는 태스크뿐이다. 태스크가 권위 측에서 `IWxLevelSequencePlaybackHost` 로 통지하고 호스트가 State 를 Idle 로 되돌린다(클라는 복제 추종). 신호의 계산 원천이 직접 발행하는 형태라, 액터가 길이를 따로 추정하는 relay 방식보다 정확하다. 종료 통지가 불필요한 기믹은 인터페이스를 구현하지 않으면 노드가 Cast 실패로 스킵한다(옵트인).
- **공용 노드 + 입력잠금 옵션**: 입력잠금을 별도 라이프사이클 태스크로 빼면 영속 상태에서 복구 타이밍을 잡기 어렵다. 재생과 입력잠금은 생명주기가 같고 종료를 아는 주체가 같으므로 한 태스크에 옵션(`bLockLocalPlayerInput`)으로 묶었다. 재생 자체는 액터 무지(State 미독)라 다른 기믹도 재사용 가능하다.
- **정리·복구를 멱등 헬퍼로**: cleanup(시퀀스 액터 파괴·핸들 해제·입력 복구)을 정상 종료(Tick)와 중도 이탈·액터 파괴(ExitState) 두 경로가 공유한다. `FinishSequencePlayback` 헬퍼로 묶어 누락·순서 실수를 막고, 핸들이 비면 멱등 노옵이 되게 했다(StateTreeComponent 가 EndPlay 시 ExitState 를 호출하므로 액터 파괴 입력락도 커버).
- **State 는 Replicated 만, SaveGame 제외**: Playing 은 일시 상태라 저장이 무의미하고, 복원 시 Playing 이면 곧장 재생이 트리거될 위험이 있다. 다른 기믹과 달리 SaveGame 을 빼 항상 Idle 로 시작하게 했다.

### 계획 대비 달라진 점
- **헬퍼 1개 추가**: cleanup 중복을 `FinishSequencePlayback` 익명 헬퍼로 묶었다(계획엔 미명시). 정상 종료·이탈 두 경로의 정리 누락 방지.
- **State 값 가드 단순화**: 계획의 `State==Idle`/`State==Playing` 진입 가드를 `SetCutsceneState` 멱등(`State==NewState` 노옵)에 맡기고, 핸들러는 `HasAuthority` 만 체크해 SpawnConsole 패턴과 일치시켰다.
- 그 외 계획대로. (작업 중 `WxGimmickStateTreeNodes` 에 별개 작업의 `LaserSpawn` 태스크가 추가돼 있어 현재 상태에 맞춰 통합했고, README 의 LaserSpawn 누락은 그 작업 범위라 건드리지 않음.)

### 후속 과제
- **에디터 작업(사용자)**: `Content/WorldObject/Gimmick/ST_CutsceneTrigger.uasset` 생성 후 `BP_CutsceneTrigger` 의 StateTree 에 할당.
  - Idle 상태: enter condition `State == Idle`, `Wx Enable Interaction(InteractionComponent, true)`.
  - Playing 상태: enter condition `State == Playing`, `Wx Enable Interaction(InteractionComponent, false)` + `Wx Play Level Sequence(LevelSequence 바인딩, bLockLocalPlayerInput=true)`, OnComplete 전이 없음.
  - 할당 전엔 상호작용해도 컷신이 재생되지 않는다.
- **PIE 검증 미완**: 상호작용 시 재생·입력 잠금, 종료 후 입력 복구·재상호작용(반복), 리슨 서버 2인에서 클라 추종.
- **검증**: WxEditor(Development) 빌드 `Result: Succeeded`(EXIT 0). 경고는 변경과 무관한 엔진 헤더 C4996(LevelSequence/MovieScene/Niagara)뿐.
