# 텔레그래프 길이조절 AnimNotifyState

## 계획

### 목표
공격 텔레그래프 NS를 몽타주에서 재생하되, **차징 길이를 노티파이 구간 길이로 조절**한다. 엔진 내장 `UAnimNotifyState_TimedNiagaraEffect`를 상속해, 구간 길이(`TotalDuration`)를 NS의 `User.Duration`에 주입하는 커스텀 AnimNotifyState를 신설한다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs` | `PublicDependencyModuleNames`에 `NiagaraAnimNotifies` 추가(베이스 헤더가 public 헤더에 포함됨) | 수정 |
| `Plugins/WxCombat/Source/WxCombat/Public/AnimNotify/WxAnimNotifyState_AttackTelegraph.h` | `UWxAnimNotifyState_AttackTelegraph : public UAnimNotifyState_TimedNiagaraEffect` 선언(생성자, SpawnEffect·NotifyBegin 오버라이드) | 신규 |
| `Plugins/WxCombat/Source/WxCombat/Private/AnimNotify/WxAnimNotifyState_AttackTelegraph.cpp` | SpawnEffect(베이스 미러+비활성 스폰), NotifyBegin(Super 후 User.Duration 주입·Activate), 생성자(bDestroyAtEnd=true) | 신규 |

### 접근 방식
- **내장 `UAnimNotifyState_TimedNiagaraEffect`(UE5.8) 상속**: Template/SocketName/오프셋/Scale/bDestroyAtEnd 프로퍼티 + 태그기반 `NotifyEnd` 정리 + `GetNotifyName`(NS명 트랙 표기)을 그대로 재사용. 신규 코드는 "길이 주입"만.
- **길이 주입(핵심)**: `SpawnEffect`를 오버라이드해 베이스 로직을 미러링하되 `bAutoActivate=false`로 **비활성 스폰**. `NotifyBegin`에서 `Super`(베이스가 SpawnEffect 호출+태깅) 뒤 `GetSpawnedEffect(MeshComp)`(public)로 컴포넌트를 찾아 `SetVariableFloat("User.Duration", TotalDuration)` → `Activate()`. 자동활성이면 기본 1초로 이미 버스트돼 늦으므로 비활성 스폰이 필수. NS의 Lifetime·Emitter Loop Duration이 `User.Duration`에 연동돼 Fill(=NormalizedAge)이 구간 전체에 걸쳐 완성.
- **정리·취소**: 생성자에서 `bDestroyAtEnd=true` → 상속 `NotifyEnd`가 구간 끝/중단 시 즉시 Destroy(선딜 중단 시 텔레그래프 취소). `SpawnEffect`의 `bAutoDestroy = !bDestroyAtEnd`.
- **색 불간섭**: 색은 Template(색 NS 변형)로 정하고 `User.Color` 미변경(색 NS 결정과 일관).
- Build.cs: `Niagara`는 이미 Private 의존(cpp에서 사용)이라 유지, 베이스 헤더 때문에 `NiagaraAnimNotifies`만 Public 추가.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxCombat.Build.cs` | `PublicDependencyModuleNames`에 `NiagaraAnimNotifies` 추가 | 수정 |
| `Public/AnimNotify/WxAnimNotifyState_AttackTelegraph.h` | 베이스 상속 선언 + 생성자·SpawnEffect·NotifyBegin 오버라이드 | 신규 |
| `Private/AnimNotify/WxAnimNotifyState_AttackTelegraph.cpp` | 비활성 스폰·User.Duration 주입·활성화, 생성자 bDestroyAtEnd=true | 신규 |

### 구현·결정과 그 이유
- **내장 노티파이 상속**: 직접 UAnimNotifyState를 짜는 대신 엔진 Timed Niagara Effect를 상속해, 태그 기반 정리·소켓 부착·프로퍼티·트랙 표기를 재사용. 신규 코드가 "길이 주입"으로 최소화되고 엔진 표준에 부합.
- **비활성 스폰 후 주입**: 버스트가 주입 값을 쓰려면 스폰 시점에 User.Duration이 정해져 있어야 함. 베이스는 자동활성이라, SpawnEffect를 오버라이드해 bAutoActivate=false로 스폰하고 NotifyBegin에서 값 주입 후 수동 활성화.
- **bDestroyAtEnd 기본 true**: 선딜이 중단되면 텔레그래프도 즉시 사라지는 게 맞음(구간 종료=상속 NotifyEnd가 정리).
- **Build.cs 최소 변경**: Niagara는 이미 Private라 cpp에서 사용 가능. 베이스 헤더가 public 헤더에 포함되므로 NiagaraAnimNotifies만 Public 추가.

### 계획 대비 달라진 점
- 초기 계획(직접 `UAnimNotifyState` 상속)에서 **사용자 제안대로 `UAnimNotifyState_TimedNiagaraEffect` 상속**으로 변경. 이로 인해 Build.cs에 `NiagaraAnimNotifies` 의존 추가됨.

### 후속 과제
- **빌드 검증 미완(사용자 요청으로 보류)**: 에디터 실행 중 Live Coding이 커맨드라인 빌드를 차단하고, 새 파일이라 전체 리빌드가 필요함. 엔진 헤더로 API 시그니처(`SpawnSystemAttached` 오버로드, `GetSpawnedEffect`/`ValidateParameters` 접근성)는 대조 확인했으나 실제 컴파일은 사용자가 에디터 종료 후 리빌드로 확인 예정.
- 새 UCLASS라 *Add Notify State* 목록에 나타나려면 에디터 재시작이 필요할 수 있음.
