# WxSave — 코드 리뷰

> 슬롯 기반 세이브/로드·맵 트래블·savable 액터 직렬화가 방어적 널 체크와 상세한 설계 주석으로 잘 다듬어진 모듈이다. 직렬화 버전 관리·비동기 저장·트래블 가드 등 위험 지점을 깊게 봤고, 심각 결함은 없다. 발견은 견고성 1건과 규칙/설계 소수다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 트래블 가드 `bTravelingFromSaveFile` 가 단일 래치라 해제 실패 시 이후 자동 캡처가 영구 스킵될 수 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:116` / `:242`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:83`
- **범주**: 버그/정확성, 상태 관리
- **문제**: `TravelFromSaveFile` 이 가드를 `true` 로 세운 뒤 이를 되돌리는 경로는 두 곳뿐이다 — `ServerTravel` 이 즉시 실패(false 반환)했을 때, 또는 목적지 월드의 `OnWorldBeginPlay` 가 `ReportTravelFromSaveFileComplete` 를 호출했을 때(`WxSaveWorldSubsystem.cpp:83`). `ServerTravel` 이 `true` 를 반환(트래블 시작 성공)했지만 이후 실제 맵 로드가 실패하거나, 목적지 월드에서 `UWxSaveWorldSubsystem` 의 `OnWorldBeginPlay` 가 traveling 플래그를 관측하지 못하는 경로가 생기면 가드가 래치된 채 남는다. 그 뒤 `HandleLevelRemovedFromWorld`/`HandleWorldBeginTearDown` 의 자동 캡처가 GameInstance 수명 내내 조용히 전부 스킵되어 세이브가 라이브 상태를 담지 못하게 된다(로그도 `Verbose` 라 표면화되지 않음).
- **제안**: 정상 스탠드얼론 트래블 경로에선 재현되지 않을 가능성이 높다. 다만 방어적으로, 트래블 시작 후 일정 프레임/시간 내 완료 보고가 없으면 가드를 자동 해제하는 워치독을 두거나, 최소한 가드가 예상보다 오래 유지될 때 경고 로그를 남기면 침묵 실패를 조기에 드러낼 수 있다.
- **확신도**: 낮음(정상 경로에선 재현되지 않으며 의도된 단순화일 수 있음 — 견고성 관점 지적)

### 2. 🟢 `ContinueSaveToFileToDisk` 가 델리게이트에 바인딩되는 콜백인데 `Handle` prefix 규칙을 따르지 않는다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:157` (바인딩), `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h:93`
- **범주**: 규칙 위반 (CLAUDE.md 규칙 6 — 델리게이트 바인딩 콜백은 `Handle` prefix)
- **문제**: `FOnSaveFlushComplete::FDelegate::CreateUObject(this, &UWxSaveGameSubsystem::ContinueSaveToFileToDisk)` 로 델리게이트에 바인딩되므로 규칙상 `Handle` prefix 대상이나 `Continue...` 명명을 쓴다. 다만 이 함수는 직접 호출(`:162`)과 델리게이트 콜백을 겸하는 continuation 성격이라 순수 이벤트 핸들러와 결이 다르다. 나머지 델리게이트 콜백(`HandleWorldInitializedActors`, `HandleGameModePostLogin`, `HandlePossessedPawnChanged` 등)은 규칙을 모두 준수한다.
- **제안**: 규칙을 엄격히 따르려면 `HandleSaveFlushComplete` 등으로 개명. 직접 호출 겸용 의도를 살리려면 현행 유지도 합리적 — 판단 필요.
- **확신도**: 높음(규칙 문언상 명확한 대상). 개명 필요 여부는 낮음(의도된 continuation 명명일 수 있음).

### 3. 🟢 `PlayerTransform` 의 `Identity` sentinel 이 월드 원점 저장과 충돌할 수 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:208`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h:92`
- **범주**: 버그/정확성 (경계 조건)
- **문제**: `TryGetPlayerTransform` 은 `PlayerTransform.Equals(FTransform::Identity)` 를 "미설정"으로 판정한다. 플레이어가 정확히 월드 원점(위치 0,0,0 · 회전 없음)에서 저장하면 유효한 재개 지점이 "미설정"으로 오판되어 `ChoosePlayerStart` 폴백으로 스폰된다(재개 지점 손실). 확률이 극히 낮고 `WxSaveGame.h:88` 주석에 의도된 tradeoff 로 명시돼 있다.
- **제안**: 실무상 무해에 가깝다. 엄밀히 하려면 `bHasPlayerStats` 처럼 `bHasPlayerTransform` 명시 플래그로 유효성을 표현하면 sentinel 충돌이 사라진다.
- **확신도**: 낮음(문서에 의도된 설계로 명시됨)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`(직렬화·버전 헤더·자동 캡처/복원·ASC 스탯 2패스 적용), `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp`(슬롯 수명·트래블 가드·비동기 디스크 기록), `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h`(데이터 모델·버전 헤더 설계), `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp`(스폰 경로 주입·스탯 복원 타이밍)
- **훑은 파일**: `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxPlayerSpawnComponent.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveModule.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveModule.h`, `Plugins/WxSave/Source/WxSave/WxSave.Build.cs`, `Plugins/WxSave/README.md`
- **모듈 경계**: `WxSave.Build.cs` 는 `WxCore` + 엔진(`GameplayAbilities`/`ModularGameplay`)만 의존 — 「WxCore 외 Wx 플러그인 참조 금지」 준수 ✅. `BlueprintCallable` 은 `UWxSaveLibrary`(BlueprintFunctionLibrary)에만 사용 — 규칙 7 준수 ✅. 모든 소스 첫 줄 Copyright 준수 ✅. `Super::` 호출 준수 ✅.
- **미검토 / 한계**: `IWxSavable`/`GetSaveId()` 정의 자체는 WxCore 소유라 이 리뷰 범위 밖. 비동기 저장 경합은 엔진 `AsyncSaveGameToSlot` 이 게임 스레드에서 메모리 직렬화 후 디스크 쓰기만 비동기라는 전제로 판단했으며(주석·구현 일치), 동일 슬롯에 저장 다중 인플라이트 시 엔진 내부 큐잉 동작은 코드로 확인하지 않음.

---
*문서 기준 커밋 `9661edf` · 리뷰일 2026-07-21 · 소스 11파일 — `/module-review`로 갱신*
