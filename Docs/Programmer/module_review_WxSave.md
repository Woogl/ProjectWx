# WxSave — 코드 리뷰

> 규모 대비 설계 밀도가 높고 매우 깨끗한 모듈이다. 권위(서버) 게이팅, 레코드별 버전 헤더 보존, 로드 트래블 가드 플래그 등 함정을 이미 문서화·방어해 뒀다. 직전 리뷰(2026-07-21)의 두 🟡(하드코딩 슬롯명 `"Test"`, `ApplyPlayerStats`의 `"Max"` 문자열 접두 의존)은 그 뒤 각각 `DefaultSaveSlotName` 상수화·이름 무관 멱등 2패스로 해소됐다. 이번 리뷰는 11개 소스 전체를 통독했고 직렬화 핵심(`WxSaveWorldSubsystem.cpp`)과 슬롯/트래블 오케스트레이션(`WxSaveGameSubsystem.cpp`)을 깊게 봤다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 2 |

가장 먼저: 저장 액터 복원 archive가 `bLoadIfFindFails=false`라, 향후 `UPROPERTY(SaveGame)` 오브젝트 참조 필드가 추가되면 로드되지 않은 애셋 참조가 조용히 null로 복원될 수 있다(현재 저장 대상은 전부 값 타입이라 미발현).

## 발견

### 🟡 복원 archive의 `bLoadIfFindFails=false` — 오브젝트 참조 필드에 대한 잠복 데이터 손실
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:365`, `:389` (복원). 대칭 캡처는 `:286`, `:304`.
- **범주**: 버그/정확성
- **문제**: `RestoreActor`가 `FObjectAndNameAsStringProxyArchive(MemReader, false)`로 복원한다. 두 번째 인자 `bLoadIfFindFails=false`는 문자열로 직렬화된 오브젝트 참조를 복원할 때 `FindObject`만 시도하고 실패해도 `LoadObject`를 하지 않는다. 따라서 `UPROPERTY(SaveGame)`로 저장된 오브젝트/소프트 참조가 가리키는 애셋이 복원 시점에 아직 로드돼 있지 않으면 참조가 **조용히 null**이 된다(에러·경고 없음). 현재 소비 도메인의 저장 필드는 값 타입뿐이라(`WxWorld` `Spawnable/WxSpawner.h:82` `bool bIsKilled`, `Gimmick/WxGimmick.h:92` `FGameplayTag State`) 실제로는 발현하지 않지만, 누군가 저장 액터에 애셋 참조 필드를 `SaveGame` 플래그로 추가하는 순간 재현이 어려운 상태 손실로 이어질 수 있는 함정이다.
- **제안**: 복원 경로의 두 archive를 `bLoadIfFindFails=true`로 두거나(참조 애셋을 로드해 해결), "값 타입만 저장한다"는 제약을 `CaptureActor`/`RestoreActor` 및 `WxCore`의 `WxSavable.h` 계약 주석에 명시한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 값 타입 전용 저장을 전제한 선택으로 보임).

### 🟢 `ApplyPlayerStats` 헤더 주석이 제거된 "Max 접두" 방식을 그대로 서술 (문서 드리프트)
- **위치**: `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h:43-44`
- **범주**: 중복/복잡도(문서 드리프트)
- **문제**: 헤더 주석은 여전히 "Max 접두 어트리뷰트를 먼저 세팅해 …충돌을 막는다"라고 적혀 있으나, 2026-07-22 작업으로 구현은 이름 heuristic 없는 멱등 2패스로 교체됐다(`WxSaveWorldSubsystem.cpp:221-258`의 cpp 주석은 정확히 갱신됨). 헤더 doc만 옛 방식을 설명해 실제 로직과 어긋난다 — 미래 세션이 오독할 소지.
- **제안**: 헤더 주석을 "이름 규칙 없이 전량 적용 후 미복원분만 재적용하는 멱등 2패스로 Max/current 세팅 순서 의존을 흡수한다" 취지로 갱신.
- **확신도**: 높음.

### 🟢 델리게이트에 바인딩되는 `ContinueSaveToFileToDisk`의 `Handle` 접두 부재 (규칙 6)
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:157` (바인딩), 선언 `Public/WxSaveGameSubsystem.h:93`, 정의 `:273`
- **범주**: 규칙 위반
- **문제**: `ContinueSaveToFileToDisk`는 `FOnSaveFlushComplete::FDelegate::CreateUObject(this, &UWxSaveGameSubsystem::ContinueSaveToFileToDisk)`로 플러시 완료 델리게이트에 바인딩되는 콜백이다. CLAUDE.md 규칙 6("Delegate에 바인딩되는 Callback 함수는 `Handle` Prefix")에 해당하며, 이 모듈의 다른 핸들러(`HandleWorldInitializedActors`, `HandlePossessedPawnChanged` 등)는 전부 이를 지킨다. 다만 이 함수는 월드 서브시스템 부재 경로(`SaveToFile` else 분기 `:162`)에서 직접 호출되는 이중 용도라, 순수 콜백만을 겨냥한 규칙에 딱 맞지는 않는다.
- **제안**: 판단 위임. 규칙을 엄격 적용하려면 `HandleSaveFlushComplete` 등으로 개명하되, "연속 단계 직접 호출"이라는 의미와의 상충을 감안하면 현행 유지도 합리적이다.
- **확신도**: 낮음(의도된 명명일 수 있음).

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`(직렬화·버전 헤더·자동 캡처/복원·GAS 2패스), `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp`(슬롯 수명·트래블 가드·디스크 I/O), `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp`(스폰 경로 주입)
- **훑은 파일**: `Public/WxSaveGame.h`, `Public/WxSaveGameSubsystem.h`, `Public/WxSaveWorldSubsystem.h`, `Public/WxPlayerSpawnComponent.h`, `Private/WxSaveLibrary.cpp`·`Public/WxSaveLibrary.h`(BP 정적 래퍼 — 순수 위임), `Private/WxSaveModule.cpp`·`Public/WxSaveModule.h`(빈 모듈), `WxSave.Build.cs`·`WxSave.uplugin`·`README.md`
- **미검토 / 한계**:
  - GAS 2패스 `ApplyPlayerStats`의 실기 정확성(로드 시 저장 MaxHP < 기본 MaxHP에서 current가 안 잘리는지)은 코드 근거상 타당하나 런타임 미검증 — 워크로그 후속 과제와 동일.
  - `UGameplayStatics::AsyncSaveGameToSlot`의 내부 스레딩(주석 전제: "게임 스레드 동기 직렬화 + 비동기 디스크 쓰기")은 엔진 소스로 직접 확인하지 않았다. 전제가 맞다면 저장 직후 SaveGame 변형과의 경쟁은 없다.
  - 두 서브시스템에서 반복되는 `World→GameInstance→GameSubsystem→SaveGame` 획득 보일러플레이트(약 8회)는 프로젝트 메모리의 "prefer-explicit-over-tiny-helpers / prefer-inplace-over-structural-extraction" 선호에 부합하므로 발견으로 올리지 않았다.
  - 규칙 준수(Copyright 첫 줄·`Super::` 호출·`Wx` 접두·`BlueprintCallable`은 BP 라이브러리에서만·플러그인 의존 WxCore+엔진 한정)는 전부 확인해 위반 없음(규칙 6 이중목적 콜백 1건만 위 🟢로 표면화).

---
*문서 기준 커밋 `702fc70f` · 리뷰일 2026-07-22 · 소스 11파일 — `/module-review`로 갱신*
