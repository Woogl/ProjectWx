# WxSave — 코드 리뷰

> 슬롯 수명·직렬화 버전 헤더·트래블 가드까지 위험 지점을 의식하고 설계한 흔적이 뚜렷하고, 널 체크와 설계 주석이 고르게 갖춰진 모듈이다. 이번 리뷰는 13개 소스 전부를 열어 두 서브시스템의 cpp(직렬화·플러시·복원·트래블)와 스폰 컴포넌트를 라인 단위로, 라이브러리·ST 태스크·모듈 파일을 훑어 봤다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 3 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 `LoadFromFile` 이 "파일 없음" 과 "파일 손상" 을 구분하지 않아, 읽기 실패한 세이브를 다음 오토세이브가 덮어써 영구 손실시킨다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:71`, `:79-85` (특히 `:83`)
- **범주**: 버그/정확성 (데이터 손실)
- **문제**: `UGameplayStatics::LoadGameFromSlot` 은 파일 부재뿐 아니라 **헤더 불일치·바이트 손상·클래스 미해석** 에서도 동일하게 nullptr 을 반환한다. 코드는 이 둘을 한 갈래로 묶어 같은 슬롯 이름으로 빈 SaveGame 을 만들고(`:83`) 트래블을 이어간다. 활성 슬롯 이름이 그대로 유지되므로, 이후 첫 체크포인트 오토세이브(`SaveToFile(FString(), ...)`)가 **아직 디스크에 남아 있던 원본 파일을 빈 세이브로 덮어쓴다**. 구체 시나리오: 비동기 디스크 기록(`:283`) 도중 크래시/전원 차단으로 파일이 잘림 → 다음 실행에서 로드 실패 → 빈 슬롯으로 리셋 → 체크포인트 한 번으로 플레이어의 진행이 전부 소멸. 사망 부활 경로가 매번 `LoadFromFile("")` 로 활성 슬롯을 다시 읽으므로 트리거 빈도도 높다. 진단조차 `Log` 레벨 한 줄(`:84`)이라 표면화되지 않는다. 「파일 없어도 리셋 후 트래블」 은 의도된 결정이지만(주석 `:81-82`, 사망 리스폰이 월드 리로드에 의존), 그 의도가 **파일이 존재하는데 못 읽는 경우까지** 포함하도록 설계된 흔적은 없다.
- **제안**: 이미 있는 `DoesSaveFileExist`(`:167`)로 두 경우를 가른다. 파일이 존재하는데 로드에 실패했다면 (a) 손상 파일을 `<Slot>.corrupt` 등으로 백업/리네임하거나 활성 슬롯 이름을 임시 슬롯으로 돌려 덮어쓰기를 차단하고, (b) `Error` 로그 + 호출자에게 실패를 알린다. 트래블(월드 리로드) 자체는 지금처럼 이어가도 사망 부활 의미론은 유지된다.
- **확신도**: 중간(리셋+트래블 폴백 자체는 의도된 설계지만, 손상 파일 덮어쓰기 파급까지 의도된 것으로 보이진 않음)

### 2. 🟡 `FlushPlayerStats` 는 캡처 실패 시 기존 저장 스탯을 지워버린다 — 같은 파일의 `FlushPlayerTransform` 과 비대칭
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:184-186` (대조: `:151-157`)
- **범주**: 버그/정확성 (데이터 손실)
- **문제**: `SaveGame->PlayerStats.Reset()` 을 먼저 하고 `CapturePlayerStats` 를 호출한 뒤 `bHasPlayerStats = PlayerStats.Num() > 0` 로 확정한다. 폰은 있는데 캡처가 0건이면(ASC 가 없는 폰을 빙의 중 — 탈것·연출용 폰·스펙테이터, 또는 `GetSpawnedAttributes()` 가 아직 비어 있는 초기화 창) 이전에 저장돼 있던 스탯이 통째로 사라지고 `bHasPlayerStats=false` 가 되어, 로드 후 데이터테이블 기본 스탯으로 되돌아간다. 이 플러시는 곧바로 디스크 기록으로 이어지므로 메모리뿐 아니라 파일까지 손실된다. 바로 위의 `FlushPlayerTransform` 은 같은 상황(폰 부재)에서 "이전 캡처 보존" 으로 명시 처리(`:154-157`)하는데 스탯 경로만 방어가 없다.
- **제안**: 로컬 `TMap` 에 캡처한 뒤 `Num() > 0` 일 때만 `PlayerStats` 를 교체하고 `bHasPlayerStats` 를 갱신한다(실패 시 기존 값 보존 + `Warning` 로그).
- **확신도**: 중간(현재 플레이어 폰은 ASC 를 직접 소유(`Source/WxGame/Character/WxCharacterBase.h:71`)해 재현 조건이 좁지만, 방어 비대칭은 코드상 명확)

### 3. 🟡 디스크 기록 실패가 로그로만 끝나고 호출자·UI 로 전달되는 경로가 없다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:283-294`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h:62`, `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h:38`
- **범주**: 설계/구조 (미처리 실패 경로)
- **문제**: `AsyncSaveGameToSlot` 의 완료 델리게이트는 성공/실패를 `Log`/`Warning` 으로 찍기만 한다. `SaveToFile` 은 `void` 이고 BP 래퍼도 `void` 라, UI 는 디스크 풀·권한·클라우드 동기화 잠금으로 기록이 실패해도 "저장됨" 으로 진행한다. 게다가 명명 저장 경로는 기록 **전에** 활성 슬롯 정체성을 새 이름으로 바꿔두므로(`:138-142`), 실패한 슬롯을 이후 체크포인트 오토세이브가 계속 목표로 삼아 실패가 누적된다. 세이브 시스템에서 "쓰기 실패를 아무도 모른다" 는 것은 기능 공백에 가깝다.
- **제안**: 서브시스템에 저장 완료 멀티캐스트(슬롯명 + 성공 여부)를 노출하거나 `SaveToFile` 에 완료 콜백 인자를 추가해 UI 가 실패 토스트/재시도를 띄울 수 있게 한다. 실패 로그는 `Error` 로 승격.
- **확신도**: 높음(코드 사실). 우선순위는 UI 요구사항에 따라 판단

### 4. 🟡 트래블 가드 `bTravelingFromSaveFile` 가 단일 래치라 해제 실패 시 세션 내내 자동 캡처가 조용히 멈춘다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:116`, `:243`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:83`
- **범주**: 설계/구조 (상태 관리)
- **문제**: 가드를 내리는 경로는 `ServerTravel` 즉시 실패(`:123`)와 목적지 월드의 `OnWorldBeginPlay` 보고(`WxSaveWorldSubsystem.cpp:83-86`) 둘뿐이다. `ServerTravel` 이 `true` 를 반환한 뒤(이미 다른 트래블이 인플라이트면 아무 일도 하지 않고 true 를 반환한다) 목적지 월드가 `UWxSaveWorldSubsystem` 을 만들지 않는 경우 — `ShouldCreateSubsystem` 이 거르는 클라이언트/비게임 월드로 폴백하는 경우 — 가드가 켜진 채 남는다. 그러면 `HandleLevelRemovedFromWorld`·`HandleWorldBeginTearDown` 의 자동 캡처가 GameInstance 수명 내내 전부 스킵되고, 스킵 로그는 `Verbose` 라 아무도 눈치채지 못한다.
- **제안**: 트래블 시작 후 일정 시간/프레임 내 완료 보고가 없으면 가드를 자동 해제하는 워치독을 두거나, 최소한 장기 유지 시 `Warning` 을 남긴다. 또는 `LoadFromFile` 진입 시 가드를 재설정해 스턱 상태가 다음 로드로 자연 해소되게 한다.
- **확신도**: 낮음(정상 스탠드얼론 경로에선 재현되지 않으며 의도된 단순화일 수 있음 — 견고성 관점)

### 5. 🟢 플레이어 스탯 키가 어트리뷰트 프로퍼티 이름뿐이라 AttributeSet 이 둘 이상이 되면 조용히 섞인다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:216`, `:249`
- **범주**: 버그/정확성 (잠재)
- **문제**: 캡처는 `OutStats.Add(It->GetFName(), ...)` 로, 적용은 `InStats.Find(It->GetFName())` 로 **AttributeSet 클래스를 무시한 평면 이름 키**를 쓴다. 이 순회는 `GetSpawnedAttributes()` 전체를 돌도록 일부러 일반화돼 있으므로, 같은 이름의 어트리뷰트를 가진 두 번째 AttributeSet 이 추가되는 순간 캡처는 뒤에 온 값이 앞 값을 덮고 적용은 한 값이 양쪽에 뿌려진다. 현재는 `UWxCombatAttributeSet` 하나뿐이라 발현하지 않는다.
- **제안**: 키를 `<AttributeSet 클래스명>.<프로퍼티명>` 으로 바꾼다. 기존 슬롯과 호환이 깨지므로 적용 시 구 평면 키 폴백을 한 버전 남기는 편이 안전하다.
- **확신도**: 중간(현 시점 미발현, 세트 추가 시 확정적으로 발현)

### 6. 🟢 저장 완료 콜백이 `Handle` prefix·람다 규칙에서 벗어난다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:157`, `:284`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h:94`
- **범주**: 규칙 위반 (CLAUDE.md 규칙 6 — 델리게이트 콜백 `Handle` prefix, 규칙 4 — 불필요한 람다)
- **문제**: `ContinueSaveToFileToDisk` 는 `FOnSaveFlushComplete::FDelegate::CreateUObject` 로 바인딩되는 콜백인데 `Handle` prefix 를 쓰지 않는다(직접 호출(`:163`)을 겸하는 continuation 이라 순수 핸들러와 결이 다르긴 하다). 또 `AsyncSaveGameToSlot` 완료 델리게이트(`:284`)는 캡처가 없고 UObject 멤버로 옮길 수 있는데 람다로 작성돼 있어, 규칙 4 와 6 을 동시에 비껴간다. 반면 `Wx.Save.Dump` 의 람다(`:16`)는 정적 초기화 시점이라 멤버 바인딩이 불가능해 정당하다. 그 외 콜백(`HandleWorldInitializedActors`·`HandleGameModePostLogin`·`HandlePossessedPawnChanged` 등)은 규칙을 지킨다.
- **제안**: `:284` 람다를 `HandleSaveGameToSlotComplete(const FString&, int32, bool)` 멤버로 빼고 `CreateUObject` 로 바인딩한다(항목 3 의 실패 통지도 같은 자리에 얹으면 된다). `ContinueSaveToFileToDisk` 는 `HandleSaveFlushComplete` 로 개명하거나 현행 유지 — 판단 필요.
- **확신도**: 높음(규칙 문언상 명확). 개명 여부는 낮음(의도된 continuation 명명일 수 있음)

### 7. 🟢 재개 지점 유효성의 `Identity` sentinel 이 월드 원점 저장과 충돌한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:209`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h:94`
- **범주**: 버그/정확성 (경계 조건)
- **문제**: `TryGetPlayerTransform` 이 `PlayerTransform.Equals(FTransform::Identity)` 를 "미설정" 으로 읽는다. 플레이어가 월드 원점 근처(위치 ~0,0,0 · 회전 ~0)에서 저장하면 유효한 재개 지점이 미설정으로 오판돼 `ChoosePlayerStart` 폴백으로 스폰된다. `Equals` 는 허용오차 비교라 정확히 원점이 아니어도 걸린다. `WxSaveGame.h:89` 에 의도된 tradeoff 로 명시돼 있다.
- **제안**: 엄밀히 하려면 `bHasPlayerStats` 처럼 `bHasPlayerTransform` 명시 플래그로 유효성을 표현한다. 실무 영향이 작다면 현행 유지도 합리적이다.
- **확신도**: 낮음(문서에 의도된 설계로 명시됨)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`(직렬화·버전 헤더·자동 캡처/복원·ASC 스탯 2패스 적용), `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp`(슬롯 수명·트래블 가드·비동기 디스크 기록), `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp`(스폰 경로 주입·스탯 복원 타이밍), `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h`(데이터 모델·버전 헤더 설계)
- **훑은 파일**: `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxStateTreeTask_SaveGame.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxPlayerSpawnComponent.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveModule.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveModule.h`, `Plugins/WxSave/Source/WxSave/WxSave.Build.cs`, `Plugins/WxSave/WxSave.uplugin`, `Plugins/WxSave/README.md`
- **규칙 점검 결과**: `WxSave.Build.cs` 는 `WxCore` + 엔진 모듈만 의존 — 「WxCore 외 Wx 플러그인 참조 금지」 준수 ✅. `BlueprintCallable` 은 `UWxSaveLibrary`(BlueprintFunctionLibrary)에만 — 규칙 7 준수 ✅. 전 소스 첫 줄 Copyright ✅. `Wx` prefix ✅. UObject override 의 `Super::` 호출 ✅(`FWxStateTreeTask_SaveGame::EnterState` 는 `Super::` 를 부르지 않으나, 베이스 반환값이 버려지는 값이고 프로젝트의 모든 ST 태스크가 동일 관행이라 지적에서 제외).
- **미검토 / 한계**: `IWxSavable`·`GetSaveId()` 의 GUID 부여 경로는 `WxCore`/`WxWorld` 소유라 범위 밖(중복 GUID 액터가 있으면 레코드가 충돌하지만 그 방지 책임은 이 모듈 밖이다). 「세이브 슬롯 월드 적용은 명시 로드 시점에만」 은 의도된 설계로 보고 지적하지 않았다. `FObjectAndNameAsStringProxyArchive` 를 `bLoadIfFindFails=false` 로 쓰는 선택은 미로드 에셋 참조를 null 로 만들 수 있으나, 현재 프로젝트의 `UPROPERTY(SaveGame)` 이 `bool`·`FGameplayTag` 뿐이라 발현 여지가 없어 발견으로 올리지 않았다. 동일 슬롯 다중 인플라이트 저장 시 엔진 `AsyncSaveGameToSlot` 내부 큐잉 동작은 코드로 확인하지 않았다.

---
*문서 기준 커밋 `c42b5fec` · 리뷰일 2026-07-25 · 소스 13파일 — `/module-review`로 갱신*
