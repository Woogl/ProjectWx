# LaserAdvance 태스크를 공용 노드로 이동 + 인자화

## 계획

### 목표
`FWxStateTreeTask_LaserAdvance`(+InstanceData)를 `WxLaserCorridor.h/.cpp`(WxGame)에서 WxWorld 공용 노드 `WxGimmickStateTreeNodes` 로 옮긴다. 옮기려면 WxGame 타입 의존(`AWxLaserCorridor` 캐스트, `AWxEffectZone`)을 끊어야 하므로, 소유자에서 읽던 목록·방향·속도를 에디터 바인딩 InstanceData 인자로 대체한다(사용자 결정: `Actors`(액터 배열) + `Velocity`(FVector)). 결과적으로 "바인딩된 액터들을 일정 속도로 이동"하는 범용 태스크가 된다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.h` | `FWxStateTreeTask_LaserAdvance`(+InstanceData `{Actors, Velocity}`) 신설, `class AActor;` 전방선언, 개요·섹션 주석 | 수정(이관) |
| `WxGimmickStateTreeNodes.cpp` | LaserAdvance `EnterState`/`Tick`/`GetDescription` 정의(권위 가드, 영구 Running) | 수정(이관) |
| `WxLaserCorridor.h` | 태스크 USTRUCT 2개·접근자 3개·`StateTreeTaskBase.h`·ST 전방선언 제거; `ActiveLasers`→바인딩 UPROPERTY `TArray<TObjectPtr<AActor>>` | 수정 |
| `WxLaserCorridor.cpp` | 태스크 정의·`StateTreeExecutionContext.h` 제거; `ActiveLasers` 컬링/파괴 루프 `IsValid()` 화 | 수정 |
| `WxGame.Build.cs` | `StateTreeModule` 제거(데드 의존) | 수정 |
| `WxWorld/README.md` | 태스크 목록에 `LaserAdvance` 추가 | 수정 |

### 접근 방식
- **태스크 이관·인자화**: 생성자 없음(틱 on). `EnterState`→`Running`. `Tick`은 소유자를 AActor로만 보고 `HasAuthority()`에서만 `Step = Velocity*DeltaTime` 으로 각 액터 `AddActorWorldOffset`(IsValid 가드), 항상 `Running`. DisplayName "Wx Laser Advance" 유지.
- **LaserCorridor 디커플링**: 태스크 전용 접근자 3개 제거. 방향·속도는 에디터 `Velocity` 로 직접. `MoveSpeed`/`CorridorBox` 멤버는 스폰 로직이 계속 사용해 유지.
- **ActiveLasers 노출**: ST 배열 바인딩 정확 타입 일치(TriggerSpawners↔TargetSpawners 선례) + 모듈 경계상 태스크는 AActor만 가능 → `ActiveLasers`를 `TArray<TObjectPtr<AActor>>` UPROPERTY로. 약참조→하드참조라 null 체크 `IsValid()` 화(현 동작 보존).
- **WxGame 의존 정리**: 이동 후 WxGame 유일 StateTree 사용처 소멸 → `WxGame.Build.cs`의 `StateTreeModule` 제거. `Wx.uproject` 플러그인 enable은 WxWorld가 써서 유지.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxGimmickStateTreeNodes.h` | `FWxStateTreeTask_LaserAdvance`(+InstanceData `{Actors, Velocity}`) 추가, `class AActor;` 전방선언, 개요·섹션 주석 | 수정(이관) |
| `WxGimmickStateTreeNodes.cpp` | LaserAdvance `EnterState`(즉시 Running)/`Tick`(권위+IsValid, Velocity·DeltaTime 이동)/`GetDescription` 정의 | 수정(이관) |
| `WxLaserCorridor.h` | 태스크 USTRUCT 2개·접근자 3개·`StateTreeTaskBase.h`·ST 전방선언 제거; `ActiveLasers`→`UPROPERTY(VisibleInstanceOnly, Transient, AllowPrivateAccess) TArray<TObjectPtr<AActor>>` | 수정 |
| `WxLaserCorridor.cpp` | 태스크 정의·`StateTreeExecutionContext.h` 제거; 파괴 루프·스폰 컬링을 `IsValid()` 기반으로 | 수정 |
| `WxGame.Build.cs` | `StateTreeModule` 제거 | 수정 |
| `WxWorld/README.md` | 태스크 목록에 `LaserAdvance` 추가 | 수정 |

### 구현·결정과 그 이유
- **인자화로 모듈 디커플링**: 태스크가 소유자를 `AWxLaserCorridor` 로 캐스트하던 것이 WxGame 결속의 원인이었다. 목록·방향·속도를 InstanceData(`Actors`, `Velocity`)로 끌어올려 소유자 타입 가정을 없애니, WxWorld 공용 노드로 옮길 수 있고 어떤 기믹이든 "바인딩된 액터 이동"에 재사용 가능해졌다.
- **Velocity 단일 벡터**: 사용자 선택. 방향·속력을 한 벡터로 받아 `Step = Velocity*DeltaTime`. 코리도의 forward×MoveSpeed 의존을 끊는 대신, 에디터에서 통로 방향에 맞춰 직접 지정한다.
- **ActiveLasers 타입 전환(약→하드, AActor)**: ST 배열 바인딩은 정확 타입 일치가 안전하고(TriggerSpawners 선례), 태스크는 모듈 경계상 AActor 만 쓸 수 있어 코리도도 `TArray<TObjectPtr<AActor>>` 로 노출했다. 코리도 C++ 는 레이저에 AActor API 만 써서 베이스 타입으로 무방. 약참조의 자동 stale 처리를 잃는 대신 null 체크를 `IsValid()` 로 바꿔 동작을 보존했다(자동파괴 레이저는 pending-kill 이라 IsValid=false 로 걸러짐).
- **권위 가드·영구 Running 유지**: 이동은 서버에서만(클라는 복제 추종), 완료 전이 없는 머무는 태스크라 항상 Running. 이관 전 의미 그대로.
- **데드 의존 정리**: 이동 후 WxGame 의 유일한 StateTree 사용처(코리도 태스크)가 사라져 `WxGame.Build.cs` 의 `StateTreeModule` 을 제거. `Wx.uproject` 의 StateTree 플러그인 enable 은 WxWorld 가 계속 쓰므로 유지.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- **에디터 작업(사용자)**: `ST_LaserCorridor` 의 Wx Laser Advance 노드에 `Actors ← ActiveLasers` 바인딩, `Velocity`(통로 forward × 속도) 지정. 미바인딩/Velocity 0 이면 이동 안 함.
- **PIE 검증 미완**: 레이저 전진(서버), 콘솔 상호작용 시 정지·기존 레이저 제거, 클라 복제 추종 확인.
