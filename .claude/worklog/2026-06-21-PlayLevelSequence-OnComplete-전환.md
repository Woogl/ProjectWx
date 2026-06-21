# PlayLevelSequence 종료 시 Succeeded 반환 + 호스트 통지 인터페이스 제거 (OnComplete 모델 전환)

## 계획

### 목표
`FWxStateTreeTask_PlayLevelSequence` 가 재생 종료 시 `Succeeded` 를 반환하게 해(PlayAnimation 동형) ST 의 OnComplete 전이가 상태를 진행시키도록 표준 완료 모델로 정리한다. 휴면 상태인 `IWxLevelSequencePlaybackHost` 통지 경로를 완전히 제거하고, `AWxCutsceneTrigger` 의 복제 State·길이추정 타이머(타 세션의 잘못된 작업)를 상호작용 이벤트 + OnComplete 복귀로 재작성한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp` | 인터페이스 include 제거, `PlayLevelSequence::Tick` 종료 시 정리 후 `Succeeded` 반환(호스트 통지 분기 삭제) | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` | PlayLevelSequence 주석을 OnComplete 완료형으로 갱신(머무는 태스크/호스트 통지 문구 제거) | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` | 베이스 훅 `HandleLevelSequenceFinished()` 제거 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxLevelSequencePlaybackHost.h` | 파일 완전 삭제 | 삭제 |
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxCutsceneTrigger.h` | State enum·복제·타이머 제거, `FGameplayTag PlayEventTag` 추가, 주석 재서술 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxCutsceneTrigger.cpp` | 타이머/복제 로직 제거, `HandleInteracted` 가 ST 이벤트 송신 | 수정 |

### 접근 방식
- **표준 완료 모델**: 태스크는 라이브 진입 시 재생, `Tick` 이 `IsPlaying` 폴링, 종료 시 시퀀스 정리 후 `Succeeded` 반환 → 소유 상태의 OnComplete 전이가 진행. 초기 진입(복원)·시퀀스 없음은 즉시 `Succeeded`, 중도 이탈은 `ExitState` 멱등 정리. 서버·클라 동일.
- **진입 트리거 = StateTree 이벤트**: OnComplete 복귀는 복제 State 추종(Enum Compare 재선택)과 섞이면 무한 재선택이 된다. 그래서 CutsceneTrigger 는 복제 State 를 버리고 상호작용 시 ST 에 이벤트를 보내 `Idle→Playing` 을 일으킨다. 기믹 공통 "이벤트 태그 없음" 원칙의 의도적 예외.
- **멀티플레이**: `OnInteracted` 가 `MulticastInteracted` 로 모든 피어에서 발화하므로 각 피어가 로컬 ST 에 이벤트를 보내 복제 없이 동시 재생. 이벤트 태그는 설정형(`EditDefaultsOnly FGameplayTag PlayEventTag`)으로 두고 디자이너가 ST 전이와 매칭(네이티브 태그 인프라 부재).
- **트레이드오프**: 진입이 Unreliable 멀티캐스트를 타 패킷 손실 시 해당 클라가 컷신을 건너뛸 수 있다(기존 복제 State 는 신뢰성). 일시 연출이라 수용.

```
Idle (초기) ──상호작용 이벤트──> Playing ──재생 종료(Succeeded→OnComplete)──> Idle
```

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp` | 인터페이스 include 제거, `PlayLevelSequence::Tick` 종료 시 정리 후 `Succeeded` 반환(호스트 통지 분기 삭제) | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h` | PlayLevelSequence 개요·구조체·공통 주석을 OnComplete 완료형으로 갱신 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h` | 베이스 훅 `HandleLevelSequenceFinished()` 제거 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxLevelSequencePlaybackHost.h` | 인터페이스 파일 제거(이미 디스크에서 삭제돼 있었음) | 삭제 |
| `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxCutsceneTrigger.h` | State enum·복제·타이머 제거, `FGameplayTag PlayEventTag` 추가, 주석 재서술 | 수정 |
| `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxCutsceneTrigger.cpp` | 타이머/복제 로직 제거, `HandleInteracted` 가 `StateTree->SendStateTreeEvent(PlayEventTag)` 호출 | 수정 |

### 구현·결정과 그 이유
- **태스크는 종료 시 Succeeded, 복귀는 ST 몫**: 재생 종료를 아는 주체(폴링하는 태스크)가 완료를 표준 신호로 발행하고, 상태 진행은 에셋의 OnComplete 전이가 맡게 분리했다. 태스크가 특정 호스트를 역참조(인터페이스 Cast)하던 결합을 끊어 PlayAnimation 과 동일한 범용 완료형 노드가 됐다.
- **진입은 ST 이벤트로(복제 State 폐기)**: OnComplete 복귀와 복제 State 추종(Enum Compare 재선택)은 양립 불가다 — State 가 Playing 으로 남아 무한 재선택이 된다. 그래서 CutsceneTrigger 만 복제 State 를 버리고 상호작용 시 ST 이벤트로 `Idle→Playing` 을 일으킨다. 기믹 공통 "이벤트 태그 없음" 원칙의 의도적 예외이며, 헤더 주석에 명시했다.
- **멀티캐스트 재사용으로 복제 불필요**: `OnInteracted` 가 `MulticastInteracted` 로 모든 피어에서 발화하므로, 각 피어의 `HandleInteracted` 가 로컬 ST 에 이벤트를 보내면 추가 RPC·복제 없이 동시 재생된다. 재진입은 Playing 상태의 `Wx Enable Interaction(false)` 가 막는다(서버 `TryInteract` 가 비활성 시 멀티캐스트 자체를 안 함).
- **이벤트 태그는 설정형**: 프로젝트에 네이티브 게임플레이 태그 인프라가 없어, 새 파일을 만들기보다 `EditDefaultsOnly FGameplayTag PlayEventTag` 한 필드로 두고 디자이너가 ST 전이와 같은 태그로 맞추게 했다(에셋 배선과 동일 맥락의 1회 설정).

### 계획 대비 달라진 점
- **인터페이스 헤더는 이미 삭제돼 있었음**: 계획 시점엔 미추적(`??`)으로 존재했으나, 구현 중 보니 다른 세션이 이미 디스크에서 지운 상태였다(`nodes.cpp` 의 dangling include 만 남아 있어 그것을 제거). 결과적으로 "완전 제거" 목표는 충족.
- **README 미수정**: 계획엔 갱신 대상으로 적었으나, README 의 PlayLevelSequence 는 공통 태스크 *목록의 이름*으로만 등장(동작 설명 없음)해 수정이 불필요했다.

### 후속 과제
- **에디터(사용자)**: `ST_CutsceneTrigger.uasset` 배선 — Idle(기본): `Wx Enable Interaction(true)`, 전이 `On Event = PlayEventTag → Playing`. Playing: `Wx Enable Interaction(false)` + `Wx Play Level Sequence(LevelSequence)` + `Wx Enable Player Input(false)`, 전이 `On State Completed → Idle`. `BP_CutsceneTrigger` 의 `PlayEventTag` 를 전이 태그와 동일하게 설정.
- **PIE 검증 미완**: 상호작용→재생+입력잠금→종료 시 입력복구+재상호작용(반복), 리슨 서버 2인 동시 재생.
- **트레이드오프(인지)**: 진입이 Unreliable 멀티캐스트를 타 패킷 손실 시 해당 클라가 컷신을 건너뛸 수 있음(기존 복제 State 는 신뢰성). 일시 연출이라 현재는 수용.

### 검증
- WxEditor(Development) 빌드 `Result: Succeeded`(8.39s). `SendStateTreeEvent(FGameplayTag)` 시그니처 확인(엔진 5.7 `StateTreeComponent.h`). 경고는 변경과 무관한 엔진 헤더 C4996(LevelSequence/MovieScene/Niagara/AIModule)뿐.
