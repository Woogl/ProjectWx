# WxSave — 코드 리뷰

> 13파일의 작고 응집도 높은 모듈이다. 널 가드·권위 게이트·PIE 격리·직렬화 버전 헤더 같은 세이브 시스템의 대표 함정이 대부분 처리돼 있고, 판단 근거가 주석에 남아 있어 읽기 쉽다. 남은 결함은 전부 정상 경로 밖 — 완료 신호의 재진입, 트래블 실패, BP 파사드의 무게이트 — 에 몰려 있다. 이번 리뷰는 `Plugins/WxSave` 의 헤더·cpp 전량(13파일)과 계약 정의인 `WxCore/WxSavable.h` 를 읽었고, `.Build.cs`/`.uplugin` 의 의존 경계까지 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 6 |

## 결과

### 1. 🟡 `FinishSaveInProgress` 의 Broadcast→Clear 가 재진입 구독자를 지워, ST 태스크가 `Running` 에 고착된다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:319-325`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp:50-60`
- **범주**: 버그/정확성
- **문제**: `FinishSaveInProgress` 는 `bSaveInProgress = false` → `OnSaveCompleted.Broadcast()` → `OnSaveCompleted.Clear()` 순서다. UE 의 멀티캐스트 `Broadcast` 는 역순 순회라 **콜백 도중 추가된 구독을 그 회차에서 건너뛰고**, 이어지는 `Clear()` 가 그 구독까지 통째로 지운다. 실패 시나리오:
  1. 체크포인트 A 의 기록 완료 → `bSaveInProgress=false` → Broadcast → ST 태스크 A 가 `WeakContext.FinishTask(Succeeded)`.
  2. 그 완료가 같은 콜스택에서 전이를 일으켜 다음 체크포인트로 진입 → 플래그가 이미 내려갔으므로 `SaveToFile` 이 접수되고, 태스크 B 가 `OnSaveCompleted.AddLambda` 로 붙는다(`WxStateTreeTask_SaveGame.cpp:57`).
  3. 스택이 풀리며 `:324` 의 `Clear()` 가 태스크 B 의 대기를 지운다. B 의 저장은 디스크에 정상적으로 써지지만 **완료 통지가 오지 않아 태스크는 `Running` 으로 고착**되고, 그 상태를 빠져나가는 전이가 없으면 그래프가 멈춘다.
- **제안**: 발화 전에 목록을 떼어낸다 — `FSimpleMulticastDelegate Local = MoveTemp(OnSaveCompleted); OnSaveCompleted.Clear(); Local.Broadcast();`. 그러면 발화 중 붙은 구독이 다음 저장의 대기자로 온전히 남는다.
- **확신도**: 중간 (델리게이트 동작은 확신하나, 같은 프레임 연쇄 저장이 실제 ST 그래프에 존재하는지는 에셋 쪽이라 미확인)

### 2. 🟡 트래블이 실패하면 `bTravelingFromSaveFile` 가드가 영구히 걸린 채 남는다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:121`, `:125-129`, `:254`
- **범주**: 버그/정확성
- **문제**: 가드는 `TravelFromSaveFile` 이 세우고, 오직 새 월드의 `UWxSaveWorldSubsystem::OnWorldBeginPlay` → `ReportTravelFromSaveFileComplete`(`WxSaveWorldSubsystem.cpp:80-83`)만이 내린다. 새 월드가 뜨지 못하면 내려줄 주체가 없다. 그런데 `UWorld::ServerTravel` 은 GameMode 의 `CanServerTravel` 거절만 false 로 답하고, 잘못된 맵 경로 같은 **실제 로드 실패는 접수 단계에서 검증하지 않아** 다음 틱의 travel failure 경로로 빠질 뿐 이 코드로 돌아오지 않는다. 가드가 걸린 채 남으면 teardown 플러시와 스트리밍-아웃 캡처가 **세션 내내 조용히 전부 스킵**되어(`WxSaveWorldSubsystem.cpp:511-515`, `:546-550`) 이후 모든 저장이 낡은 상태를 기록한다.
- **제안**: `GEngine->OnTravelFailure()` 를 구독해 실패 시 가드를 내린다. 또는 가드에 만료 조건(트래블을 시작한 월드 포인터·프레임 워치독)을 붙여 기대한 새 월드가 오지 않으면 경고와 함께 해제한다.
- **확신도**: 중간 (정상 경로에선 재현되지 않고, 실패 시 조용히 나빠지는 유형이다)

### 3. 🟡 BP 진입점에 권위 게이트가 없다 — 클라이언트에서 호출하면 로컬 트래블로 세션을 이탈한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp:68-84` (`SaveToFile`, `TravelFromSaveFile`)
- **범주**: 설계/구조
- **문제**: 모듈의 나머지는 권위를 일관되게 지킨다 — `UWxSaveWorldSubsystem::ShouldCreateSubsystem` 이 `NM_Client` 를 제외하고(`WxSaveWorldSubsystem.cpp:50`), ST 태스크(`WxStateTreeTask_SaveGame.cpp:27-31`)와 스폰 컴포넌트(`WxPlayerSpawnComponent.cpp:20`)는 `HasAuthority()` 로 막는다. 그런데 BP 파사드인 `UWxSaveLibrary` 만 무게이트다. 클라이언트에서 `TravelFromSaveFile` 을 부르면 `GetAuthGameMode()` 가 null 이라 `CanServerTravel` 검사 자체가 건너뛰어지고 `NextURL` 만 세팅돼, 그 클라이언트가 **서버에서 떨어져 로컬 맵으로 이동**한다. 클라의 `SaveToFile` 도 월드 서브시스템이 없어 플러시 없이 인메모리 슬롯을 그대로 로컬 디스크에 쓴다.
- **제안**: 라이브러리 진입점 선두에서 `World->GetNetMode() != NM_Client`(또는 `GetAuthGameMode() != nullptr`)를 확인하고, 아니면 경고 후 noop 한다.
- **확신도**: 중간 (모듈 주석이 "스탠드얼론 싱글 전제"를 여러 곳에 남기고 있어 의도된 미대응일 수 있으나, 같은 모듈의 다른 진입점과 불일치한다)

### 4. 🟢 `StartNewSaveFile` 이 빈 SlotName 을 검증하지 않아 이후 모든 기록이 조용히 실패한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:45-66` (특히 `:60`)
- **범주**: 버그/정확성
- **문제**: 헤더(`WxSaveGameSubsystem.h:39`)는 "SlotName 은 그대로 슬롯 정체성이 되므로 유효한 이름을 넘겨야 한다"고 계약을 적었지만, 코드는 `SpecificClass` 만 검사하고 SlotName 은 그대로 받아 `SaveGame->SlotName` 에 꽂는다. 이 함수는 `UWxSaveLibrary::StartNewSaveFile`(`WxSaveLibrary.h:26-27`)로 BP 에 노출돼 있어 빈 문자열이 들어오기 쉽고, 그러면 엔진 `AsyncSaveGameToSlot` 의 슬롯 이름 유효성 검사에 걸려 기록이 매번 실패한다. 남는 단서는 `:314` 의 "디스크 기록 실패" Warning 한 줄뿐이라 원인이 슬롯 이름임을 짚기 어렵다.
- **제안**: `SlotName.IsEmpty()` 면 다른 실패와 같은 형식의 Warning 을 남기고 nullptr 을 반환한다(`:47-51` 의 `SpecificClass` 가드와 대칭).
- **확신도**: 높음

### 5. 🟢 저장 성공/실패가 대기자에게 전달되지 않아 ST 태스크가 실패에도 `Succeeded` 를 반환한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:300-316`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp:57-62`
- **범주**: 설계/구조
- **문제**: 비동기 콜백의 `bSuccess` 는 로그로만 쓰이고 버려진다. `OnSaveCompleted` 가 `FSimpleMulticastDelegate` 라 결과를 실을 자리가 없고, 그래서 체크포인트 ST 태스크는 디스크 기록이 실패해도 `Succeeded` 로 흐른다. 저장이 실패했는데 그래프가 "저장 완료"로 진행하는 상황을 그래프 쪽에서 분기할 수단이 없다.
- **제안**: `DECLARE_MULTICAST_DELEGATE_OneParam(..., bool /*bSuccess*/)` 로 바꾸고 ST 태스크가 실패 시 `Failed` 를 반환하게 한다. 그래프 저작에 분기를 강요하지 않으려면 최소한 `:314` 의 실패 로그를 Error 로 승격한다.
- **확신도**: 중간 (체크포인트는 실패해도 진행하는 편이 낫다는 판단일 수 있다)

### 6. 🟢 규칙 위반 — 불필요한 람다 + 델리게이트 콜백의 `Handle` prefix 누락
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:301`, `:169`
- **범주**: 규칙 위반
- **문제**: `:301` 의 `FAsyncSaveGameToSlotDelegate::CreateLambda` 는 `TWeakObjectPtr` 캡처 하나 때문에 쓰였는데, 같은 시그니처의 멤버 함수 + `CreateUObject` 로 그대로 대체된다(약한 수명 처리는 `CreateUObject` 가 이미 해준다) — 코딩 규칙 3(람다는 반드시 필요한 경우에만) 위반이다. `:169` 에서 `FOnSaveFlushComplete` 에 바인딩되는 `ContinueSaveToFileToDisk` 는 델리게이트 콜백인데 `Handle` prefix 가 없다(규칙 4). 같은 파일 `:15` 의 콘솔 명령 람다는 전역 static 등록이라 대체 수단이 없고, `WxStateTreeTask_SaveGame.cpp:57` 의 람다는 약한 실행 컨텍스트 캡처가 엔진 권장 형태이며 `:55` 에 사유가 명시돼 있어 둘 다 대상이 아니다.
- **제안**: `:301` 람다를 `HandleSaveGameWritten(const FString&, int32, bool)` 멤버 + `CreateUObject` 로 교체한다. `ContinueSaveToFileToDisk` 는 직접 호출부(`:174`)도 있으니 델리게이트 바인딩 전용 얇은 `Handle...` 래퍼를 두거나 이름을 `HandleSaveFlushComplete` 로 바꾼다.
- **확신도**: 높음

### 7. 🟢 플레이어 스탯 키가 어트리뷰트 프로퍼티 이름뿐이라 AttributeSet 이 늘면 조용히 충돌한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:208-225`, `:240-270`
- **범주**: 버그/정확성
- **문제**: `CapturePlayerStats` 는 ASC 의 **모든** AttributeSet 을 순회하며 `OutStats.Add(It->GetFName(), ...)`(`:223`)로 담고, `ApplyPlayerStats` 도 프로퍼티 이름만으로 `InStats.Find`(`:254`) 한다. 서로 다른 두 AttributeSet 이 같은 이름의 어트리뷰트를 가지면 캡처에서는 나중 세트가 앞 세트를 덮고, 복원에서는 **두 세트 모두에 같은 값이 들어간다**. 지금은 프로젝트 AttributeSet 이 하나뿐이라 발현하지 않지만, 세트 분리는 GAS 프로젝트가 흔히 밟는 확장이고 그때 증상은 "특정 스탯만 로드 후 이상해짐"으로 나타나 원인을 짚기 어렵다. 저장 파일에 남는 키라 나중에 바꾸면 마이그레이션이 필요하다는 점에서 지금 정하는 편이 싸다.
- **제안**: 키를 `AttributeSet 클래스명 + 프로퍼티명`(또는 `FGameplayAttribute::GetName()` 형태의 정규화 문자열)으로 바꾸고, 구버전 키는 이름만으로 폴백 조회한다.
- **확신도**: 낮음(현재 세트가 하나뿐이라 오늘의 버그는 아니다)

### 8. 🟢 저장 스탯 적용 구독이 일회성이 아니라 이후의 모든 빙의에서 재적용된다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp:53`, `:90-98`
- **범주**: 설계/구조
- **문제**: `OnPossessedPawnChanged` 구독은 한 번 붙으면 세션 내내 유지되고, 매 빙의마다 `ApplySavedPlayerStats` 가 **마지막 저장 시점의** 어트리뷰트 스냅샷을 새 폰에 덮어쓴다. 지금은 재빙의 경로가 없어(사망 부활도 맵 리로드 경유) 트리거되지 않지만, 탈것·포탑·조종 대상 전환 같은 오픈월드 액션 RPG 의 흔한 확장이 들어오는 순간 원래 폰으로 돌아올 때 체크포인트 시점 HP 로 되감기는 회귀가 된다. 같은 맥락으로 `OnUnregister`(`:36-42`)는 `PostLoginHandle` 만 정리하고 이 동적 구독은 떼지 않는다.
- **제안**: 첫 유효 빙의에서 적용한 뒤 `RemoveDynamic` 으로 떼거나, 적용 여부 플래그로 1회로 제한한다.
- **확신도**: 낮음(의도된 설계일 수 있음 — "스탠드얼론 싱글·단일 폰 전제" 주석이 모듈 전반에 있다)

### 9. 🟢 복원 아카이브가 `bLoadIfFindFails=false` 라, 미로드 에셋을 가리키는 SaveGame 참조는 조용히 null 이 된다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:419`, `:443`
- **범주**: 버그/정확성
- **문제**: `FObjectAndNameAsStringProxyArchive` 는 오브젝트 참조를 경로 문자열로 저장하고, 로드 시 두 번째 인자(`bLoadIfFindFails`)가 false 면 **`FindObject` 만 시도하고 실패하면 null 을 넣는다**. 복원은 `OnWorldInitializedActors`/`LevelAddedToWorld` 시점이라 아직 로드되지 않은 에셋을 가리키는 참조는 여기서 조용히 사라진다. 현재 프로젝트의 `UPROPERTY(SaveGame)` 필드는 `FGameplayTag`·`bool` 뿐이라(`WxWorld/.../WxDeviceStateTreeComponent.h:75`, `WxSpawner.h:69`) 오늘의 버그는 아니지만, 저장 대상에 오브젝트/클래스 참조가 추가되는 순간 크래시 없이 데이터만 유실되는 형태로 나타난다.
- **제안**: 복원 경로 두 곳만 `true` 로 바꾸거나(동기 로드 비용 감수), 오브젝트 참조는 SaveGame 필드로 쓰지 않는다는 제약을 `IWxSavable` 계약 주석에 명시한다.
- **확신도**: 낮음(현재 저장 필드에 오브젝트 참조가 없어 의도된 제약일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h`
- **훑은 파일**: `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveLibrary.h`, `Plugins/WxSave/Source/WxSave/Public/WxPlayerSpawnComponent.h`, `Plugins/WxSave/Source/WxSave/Public/WxStateTreeTask_SaveGame.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveModule.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveModule.h`, `Plugins/WxSave/Source/WxSave/WxSave.Build.cs`, `Plugins/WxSave/WxSave.uplugin`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`(계약 확인용, 리뷰 대상 아님)
- **미검토 / 한계**:
  - 모듈·코딩 규칙 준수는 전량 확인했고 6번 외 위반 없다 — 13파일 모두 Copyright 첫 줄이 있고, `WxSave.Build.cs`/`.uplugin` 의 Wx 의존은 `WxCore` 하나뿐이며, `BlueprintCallable` 은 BP Function Library 안에서만 쓰였고, 헤더의 인라인 함수 정의는 `WxStateTreeTask_SaveGame.h:43` 의 `GetInstanceDataType()` 하나로 `:13` 에 예외 사유가 명시돼 있다(`WxSaveGameSubsystem.h:17` 의 `inline` 은 변수라 규칙 6 대상 아님). override 의 `Super::` 호출도 누락 없음.
  - 이번 세션은 엔진 소스가 없는 샌드박스라, 엔진 동작(멀티캐스트 `Broadcast` 의 역순 순회, `UWorld::ServerTravel` 의 반환 계약, `AsyncSaveGameToSlot` 의 슬롯 검증, `FObjectAndNameAsStringProxyArchive` 의 `bLoadIfFindFails`)은 API 계약 지식에 근거했고 UE 5.8 소스 대조는 하지 못했다. 1·2·4·9번의 확신도에 반영했다.
  - 직렬화 정확성(`CaptureActor`/`RestoreActor` 의 버전 헤더 왕복, 아키타입 대비 `ShouldSave` 판정)은 코드 독해로만 검증했고 실제 이기종 빌드 왕복 테스트는 하지 않았다. 로직상 결함은 찾지 못했다.
  - World Partition 실환경에서의 `LevelAddedToWorld`/`LevelRemovedFromWorld` 발화 타이밍, Experience 에셋의 `UWxPlayerSpawnComponent` 주입 등록 여부, ST 그래프에서의 체크포인트 태스크 배치(1번의 재진입 조건)는 에셋/런타임 영역이라 범위 밖이다.

---
*문서 기준 커밋 `b47e709` · 리뷰일 2026-08-30 · 소스 13파일 — `/module-review`로 갱신*
