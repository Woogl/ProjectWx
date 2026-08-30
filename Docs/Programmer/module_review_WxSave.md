# WxSave — 코드 리뷰

> 저장 계층의 책임 분리는 명확하지만, 새 Mass 영속화가 임의 fragment를 원시 메모리로 다루고 스키마·payload 검증 없이 복원해 크래시와 저장 손상을 일으킬 수 있다. README에서 안내한 진입점을 따라 슬롯 I/O, 월드 플러시, IAM, Mass, 플레이어 복원을 깊게 검토하고 나머지 C++와 의존 경계를 훑었다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 3 |
| 🟡 개선 | 5 |
| 🟢 사소 | 2 |

## 결과

### 1. 🔴 임의 Mass fragment를 원시 메모리로 저장해 비단순 타입의 수명이 깨진다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveSettings.cpp:13-18`, `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:271-291`, `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:388-401`
- **범주**: 성능/안전
- **문제**: 설정 UI는 로드된 모든 `FMassFragment` 파생 타입을 후보로 내놓지만, 저장과 복원은 `UScriptStruct` 직렬화가 아니라 `GetStructureSize()`만큼 메모리를 그대로 복사한다. `TArray`, `TObjectPtr`, `TSharedPtr`처럼 프로세스 내부 주소나 소유권을 가진 fragment를 허용 목록에 넣으면 주소값이 파일에 기록되고, 다음 실행에서 초기화된 fragment 위에 덮여 댕글링 참조·이중 해제·크래시가 발생한다. 현재 기본값인 `FTransformFragment`는 안전하지만 API가 보장하는 일반 fragment 영속화는 안전하지 않다.
- **제안**: `UScriptStruct::SerializeItem`과 SaveGame용 archive로 필드 직렬화하거나, 원시 복사를 유지한다면 명시적으로 등록한 trivially-copyable fragment만 허용하고 설정 단계와 런타임 양쪽에서 거부한다.
- **확신도**: 높음

### 2. 🔴 Mass 스키마와 payload를 검증하기 전에 엔티티를 생성하고 메모리에 쓴다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:247-278`, `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:321-400`
- **범주**: 버그/정확성
- **문제**: 스냅샷은 `EntityConfig`의 원본 archetype에서 fragment 목록을 만든 뒤 같은 config 출신 엔티티가 현재도 그 fragment를 가졌다고 가정한다. 런타임 archetype 변경으로 fragment가 빠지면 `GetFragmentDataStruct()`의 빈 view를 원시 복사한다. 복원도 타입의 존재·크기·허용 목록만 확인하고 새 config archetype에 fragment가 실제 있는지 확인하지 않아, config 변경 후 구 슬롯을 열면 null 메모리에 쓸 수 있다. 또한 `EntityCount`와 `Data.Num()`의 일치나 상한을 검사하기 전에 `uint32`로 캐스팅해 스폰하므로 손상된 슬롯은 대량 할당 후 부분 복원까지 일으킨다.
- **제안**: 저장은 실제 archetype별로 그룹화하고 모든 `FStructView` 유효성을 확인한다. 복원은 config archetype 포함 여부, 양수 크기, 엔티티 수 상한, `EntityCount * entityStride == Data.Num()`을 오버플로 안전하게 검증한 뒤에만 그룹 전체를 생성·적용한다.
- **확신도**: 높음

### 3. 🔴 플러시 대기 중 활성 SaveGame을 바꾸면 다른 슬롯에 현재 월드가 섞인다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:49-118`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:172-188`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:334-347`
- **범주**: 버그/정확성
- **문제**: `SaveToFile`은 `bSaveInProgress`를 세운 뒤 Mass `FrameEnd`까지 플러시 완료를 기다리지만, `StartNewSaveFile`과 `LoadFromFile`은 이 상태를 검사하지 않고 멤버 `SaveGame`을 교체한다. 그 사이 새 슬롯 시작이나 로드를 호출하면 플러시 콜백은 요청 당시 객체가 아니라 교체된 `SaveGame`에 이전 월드 스냅샷을 쓰고 그 객체의 슬롯으로 디스크 저장한다. 저장 요청 하나가 다른 슬롯 오염과 의도하지 않은 덮어쓰기로 바뀐다.
- **제안**: 저장 요청마다 대상 `UWxSaveGame`·슬롯·사용자 인덱스를 캡처한 불변 컨텍스트를 두고 모든 플러시 결과를 그 객체에만 적용한다. 더 단순하게는 저장 중 `StartNewSaveFile`, `LoadFromFile`, `DeleteSaveFile`, 트래블을 거부하거나 완료 뒤 큐잉한다.
- **확신도**: 높음

### 4. 🟡 디스크 저장 실패도 완료 성공으로 전달된다
- **위치**: `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h:53`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:355-372`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp:73-75`
- **범주**: 버그/정확성
- **문제**: 비동기 콜백의 `bSuccess`는 로그에만 쓰이고 `FSimpleMulticastDelegate`는 성공 여부 없이 발화한다. 디스크 용량 부족·권한 오류가 나도 체크포인트 StateTree 태스크는 `Succeeded`로 끝나므로 게임 진행은 저장된 것으로 간주되지만 재실행 시 복구할 기록이 없다.
- **제안**: 완료 delegate에 `bool bSuccess`를 전달하고 StateTree 태스크가 실패·재시도 정책을 선택하게 한다. 요청 접수 성공과 디스크 커밋 성공도 API에서 분리한다.
- **확신도**: 높음

### 5. 🟡 Mass 스냅샷 실패가 기존 정상 데이터를 빈 배열로 덮는다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:42-47`, `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:186-213`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:507-515`
- **범주**: 버그/정확성
- **문제**: `UMassSimulationSubsystem`·`UMassSpawnerSubsystem` 부재나 설정된 fragment를 하나도 해석하지 못한 경우는 빈 snapshot으로 완료되며, 호출자는 이를 "유효한 빈 월드"와 구분하지 않고 `MassEntitySnapshots`를 교체한다. teardown 순서나 일시적 초기화 실패 중 저장하면 이전 슬롯에 있던 정상 Mass 상태를 조용히 지운다.
- **제안**: snapshot 결과에 성공 여부를 추가하고, 실패면 LSP 직렬화 실패 처리처럼 기존 데이터를 유지한다. 실제 엔티티가 0개인 성공 결과만 빈 배열로 커밋한다.
- **확신도**: 높음

### 6. 🟡 BP 저장·로드 진입점이 클라이언트에서도 서버 월드 저장 API를 실행한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp:58-82`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:40-49`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:121-147`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:173-193`
- **범주**: 설계/구조
- **문제**: 월드 서브시스템은 `NM_Client`에서 생성하지 않아 서버 권위 설계를 명시하지만, BP Function Library와 GameInstance 서브시스템에는 같은 게이트가 없다. 클라이언트의 `SaveToFile`은 월드 플러시 없이 로컬 기본 슬롯을 기록하고, `LoadFromFile`은 `ServerTravel`을 로컬 월드에 예약해 세션에서 이탈할 수 있다.
- **제안**: 실제 상태 변경 주체인 `UWxSaveGameSubsystem`에서 `NM_Client` 호출을 거부하고 명확한 실패를 반환한다. 멀티플레이 저장을 지원할 계획이면 RPC와 플레이어별 데이터 소유권을 별도 계약으로 정의한다.
- **확신도**: 높음

### 7. 🟡 비동기 트래블 실패 후 `bTravelingFromSaveFile`이 영구 고정될 수 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:136-147`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:286-297`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:340-343`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:485-498`
- **범주**: 버그/정확성
- **문제**: `ServerTravel`의 즉시 거절만 플래그를 내리고, 접수 후 맵 로드 실패는 처리하지 않는다. 새 월드의 `ReportTravelFromSaveFileComplete`가 오지 않으면 플래그가 남아 IAM 스트리밍-아웃 캡처와 teardown 플러시가 이후 계속 건너뛰어진다.
- **제안**: `GEngine->OnTravelFailure()`를 구독해 해당 요청의 실패 시 플래그를 해제하고, 요청 월드·목표 맵을 함께 저장해 무관한 실패와 구분한다.
- **확신도**: 중간

### 8. 🟡 완료 브로드캐스트 뒤 `Clear()`가 재진입 중 등록된 다음 저장 대기자를 지운다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:368-372`
- **범주**: 버그/정확성
- **문제**: UE multicast는 브로드캐스트 중 새로 추가된 항목을 현재 순회에서 제외한다. 기존 완료 콜백이 동기적으로 다음 저장을 시작하고 새 완료 콜백을 붙이면, 현재 `Broadcast()`가 이를 호출하지 않은 채 직후 `Clear()`가 삭제해 다음 요청의 대기자가 영원히 통지받지 못한다.
- **제안**: 발화 전에 기존 목록을 로컬 delegate로 떼고 원본을 비운 뒤 로컬만 브로드캐스트한다. 발화 중 등록된 항목은 다음 저장까지 원본에 남긴다.
- **확신도**: 높음

### 9. 🟢 플레이어 스탯 키가 AttributeSet 소유 타입을 잃는다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:579-592`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:608-633`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h:116-120`
- **범주**: 설계/구조
- **문제**: 모든 AttributeSet을 순회하면서 프로퍼티의 `FName`만 키로 저장한다. 서로 다른 AttributeSet에 같은 이름이 생기면 캡처는 뒤의 값을 덮고 복원은 두 속성에 같은 값을 적용한다. 현재 저장 포맷의 키이므로 AttributeSet 분리 뒤 수정하면 마이그레이션까지 필요해진다.
- **제안**: `AttributeSet` 클래스 경로와 프로퍼티 이름을 합친 구조화 키를 저장하고, 기존 이름 전용 키는 포맷 마이그레이션에서 처리한다.
- **확신도**: 낮음(현재 AttributeSet이 하나라 의도된 설계일 수 있음)

### 10. 🟢 규칙 위반 — delegate callback 두 개가 `Handle` prefix를 쓰지 않는다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:65-67`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:194-197`
- **범주**: 규칙 위반
- **문제**: `OnLevelBeginMakingInvisible`에 바인딩한 `FlushInstancedActorManagerDataForLevel`과 `FWxOnMassPreSnapshot`에 바인딩한 `PerformPreSaveMassTasks`가 프로젝트의 "delegate callback은 `Handle` prefix" 규칙을 위반한다.
- **제안**: 직접 호출용 작업 함수는 유지하고, 바인딩 전용 `HandleLevelBeginMakingInvisible`·`HandleMassPreSnapshot` 래퍼를 추가한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxMassPersistence.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGameSubsystem.h`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxPlayerSpawnComponent.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxPersistedMassSpawner.cpp`
- **훑은 파일**: `Plugins/WxSave/README.md`, `Plugins/WxSave/WxSave.uplugin`, `Plugins/WxSave/Source/WxSave/WxSave.Build.cs`, 나머지 `Public/`·`Private/` C++ 전부, `Config/DefaultEngine.ini`의 LSP·WxSave 설정
- **미검토 / 한계**: BP/WBP 내부 구조와 EntityConfig/DataAsset 실제 값은 범위 밖이다. 손상 슬롯, 네트워크 PIE, 대규모 Mass 월드의 런타임 실패 주입·성능 측정은 수행하지 않았고 UE 5.8 엔진 소스로 delegate, `ServerTravel`, `FMemoryReader`, Mass view 계약만 대조했다. `AGENTS.md` 규칙은 전체 C++와 descriptor에서 점검했으며 10번 외 저작권 첫 줄, `Wx` prefix, `BlueprintCallable`, inline 예외, `WxCore` 전용 의존 경계 위반은 찾지 못했다.

---
*문서 기준 커밋 `fc2239e6` · 리뷰일 2026-08-31 · 소스 21파일 — `/module-review`로 갱신*
