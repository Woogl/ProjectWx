# WxLaserCorridor 이동을 StateTree 태스크로 전환 (최소 범위)

## 계획

### 목표
`AWxLaserCorridor`의 레이저 이동을 액터 `Tick` 오버라이드에서 빼내 StateTree 태스크가 구동하게 한다. 스폰 타이머·`State`·`RefreshLaserState`·`ActiveLasers` 소유는 C++에 그대로 두는 최소 범위 전환이다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Source/WxGame/WorldObject/WxLaserCorridor.h` | `StateTreeTaskBase.h` include, `Tick` 선언 제거, 읽기 전용 접근자 3개 추가, 태스크 USTRUCT 2개 선언 | 수정 |
| `Source/WxGame/WorldObject/WxLaserCorridor.cpp` | `Tick`·`bCanEverTick` 제거, `HandleSpawnTimer`에 null 컬링 추가, 태스크 `EnterState`/`Tick`/`GetDescription` 정의 | 수정 |
| `Source/WxGame/WxGame.Build.cs` | `StateTreeModule` 추가 | 수정 |

### 접근 방식
- **이동 루프를 태스크로, 데이터는 접근자로**: 이동 루프를 `FWxStateTreeTask_LaserAdvance::Tick`으로 옮기고, 루프가 쓰는 레이저 목록·진행 방향·속도를 액터가 읽기 전용 접근자로 노출한다(`Context.GetOwner()` 캐스트로 획득). 태스크는 const 뷰만 읽으므로 null 컬링은 스폰 타이머로 이관한다.
- **WxGame 배치**: `AWxEffectZone`이 WxCombat 소속이라 WxWorld 공용 노드에 둘 수 없다. 액터 전용 태스크라 `WxLaserCorridor.h/.cpp`에 공존 배치한다.
- **권위 가드 유지·영구 Running**: StateTree는 서버·클라 모두 틱하므로 이동은 `HasAuthority`에서만. 태스크는 완료 전이 없이 영구 `Running`(머무는 상태).
- **사용자 에디터 작업**: `ST_LaserCorridor`(루트 상태 1개·Wx Laser Advance) 생성 후 `BP_LaserCorridor`의 StateTree에 할당(베이스가 자동 시작).

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Source/WxGame/WorldObject/WxLaserCorridor.h` | `StateTreeTaskBase.h` include, `Tick` 선언 제거, 읽기 전용 접근자 3개 추가, `FWxStateTreeTask_LaserAdvance`(+InstanceData) 선언 | 수정 |
| `Source/WxGame/WorldObject/WxLaserCorridor.cpp` | `Tick` 정의·`bCanEverTick` 제거, `HandleSpawnTimer`에 null 컬링 추가, 태스크 `EnterState`/`Tick`/`GetDescription` 정의 | 수정 |
| `Source/WxGame/WxGame.Build.cs` | `StateTreeModule` 추가 | 수정 |
| `Wx.uproject` | `StateTree` 플러그인 명시 추가 | 수정 |

### 구현·결정과 그 이유
- **이동만 태스크로(데이터는 접근자 노출)**: 사용자 선택대로 최소 범위. 이동 루프를 태스크 `Tick`으로 옮기고 액터는 목록·박스·속도를 읽기 전용 접근자로 노출한다. 스폰·State·수명은 액터가 그대로 소유해 파급을 가둔다.
- **WxGame 공존 배치**: `AWxEffectZone`이 WxCombat 소속이라 WxWorld 공용 노드에 둘 수 없다. 액터 전용 태스크라 별도 파일 없이 `WxLaserCorridor.h/.cpp`에 함께 뒀다.
- **컬링을 스폰으로 이관**: 태스크가 const 뷰만 읽어 목록을 변형할 수 없으므로, 원래 `Tick`이 하던 null 정리를 `HandleSpawnTimer`로 옮겼다. 스폰 간격마다 걷어 목록을 유계로 유지한다.
- **권위 가드·영구 Running**: StateTree는 서버·클라 모두 틱하므로 이동은 `HasAuthority`에서만(클라는 복제 추종). 완료 전이 없는 머무는 태스크라 항상 Running을 반환한다.
- **`Wx.uproject`에 StateTree 명시**: WxGame이 `StateTreeModule`을 직접 참조하게 되어 UBT가 플러그인 미명시 경고를 냈다. 플러그인을 추가해 경고를 없앴다(WxWorld.uplugin은 이미 명시 중).
- **검증**: WxEditor(Development) 빌드 `Result: Succeeded`(EXIT 0). 재빌드 시 StateTree 플러그인 경고 소멸 확인. 경고는 변경과 무관한 엔진 C4996뿐.

### 계획 대비 달라진 점
- `Wx.uproject`에 StateTree 플러그인 명시 1건 추가(빌드 경고 해소). 그 외 계획대로.

### 후속 과제
- **에디터 작업(사용자)**: `Content/WorldObject/Gimmick/ST_LaserCorridor.uasset` 생성(루트 상태 1개·Wx Laser Advance·완료 전이 없음) 후 `BP_LaserCorridor`의 StateTree에 할당. 할당 전엔 레이저가 스폰만 되고 이동하지 않는다.
- **PIE 검증 미완**: 레이저 전진, 콘솔 상호작용 시 정지·기존 레이저 제거, 리슨 서버 2인 클라 추종 확인.
