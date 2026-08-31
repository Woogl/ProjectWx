# WxSave — 코드 리뷰

> LSP 중심의 책임 분리와 모듈 경계는 비교적 명확하지만, Mass 원시 스냅샷과 비동기 슬롯 수명에는 실제 저장 손상·크래시 경로가 남아 있다. WxSave 25개 C++ 파일을 검토하고, WxCore 계약 및 WxGame·WxWorld·WxCombat·WxInventory·Config의 저장 통합면을 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 5 |
| 🟡 개선 | 11 |
| 🟢 사소 | 1 |

## 결과

### 1. 🔴 임의 Mass fragment를 원시 메모리로 저장해 비단순 타입의 수명이 깨진다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveSettings.cpp:13`, `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:271`, `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:388`
- **범주**: 성능/안전
- **문제**: 설정 UI는 모든 `FMassFragment` 파생 타입을 후보로 내놓지만, 저장·복원은 `UScriptStruct` 직렬화가 아니라 `GetStructureSize()`만큼 메모리를 그대로 복사한다. `TArray`, `TObjectPtr`, 공유 포인터 등 프로세스 내부 주소나 소유권을 가진 fragment를 허용하면 다음 실행에서 댕글링 참조·이중 해제·크래시가 발생한다. 현재 기본값인 `FTransformFragment`는 안전하지만 API가 일반 fragment를 안전하게 제한하지 않는다.
- **제안**: `UScriptStruct::SerializeItem`과 SaveGame archive로 필드 직렬화한다. 원시 복사를 유지한다면 별도 등록된 trivially-copyable 타입만 설정 UI와 런타임 양쪽에서 허용한다.
- **확신도**: 높음

### 2. 🔴 Mass 스키마와 payload를 검증하기 전에 엔티티를 생성하고 메모리에 쓴다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:247`, `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:321`
- **범주**: 버그/정확성
- **문제**: 저장은 원본 EntityConfig archetype의 fragment 목록이 같은 config 출신 모든 엔티티에 남아 있다고 가정한다. 런타임 archetype 변경으로 fragment가 빠지면 빈 `FStructView`에 접근할 수 있다. 복원도 새 config archetype이 저장 fragment를 실제 포함하는지, `EntityCount * stride`가 `Data.Num()`과 일치하는지, 엔티티 수가 합리적인지 확인하기 전에 `SpawnEntities`와 메모리 쓰기를 수행한다. config 변경이나 손상 슬롯은 assert, 부분 복원, 과도한 할당으로 이어질 수 있다.
- **제안**: 저장은 실제 archetype별로 그룹화하고 view 유효성을 검사한다. 복원은 fragment 포함 여부, 양수 크기, 엔티티 수 상한, 오버플로 안전한 payload 길이를 모두 검증한 뒤 그룹 전체를 생성한다.
- **확신도**: 높음

### 3. 🔴 플러시 대기 중 활성 SaveGame을 바꾸면 다른 슬롯에 현재 월드가 섞인다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:49`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:77`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:150`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:307`
- **범주**: 버그/정확성
- **문제**: `SaveToFile`은 Mass `FrameEnd`까지 플러시 완료를 기다리지만 `StartNewSaveFile`과 `LoadFromFile`은 `bSaveInProgress`를 검사하지 않고 멤버 `SaveGame`을 교체한다. 그 사이 새 슬롯 시작이나 로드를 호출하면 남은 플러시 결과와 `BeginAsyncSaveToDisk()`가 요청 당시 객체가 아닌 교체된 객체·슬롯을 사용해 이전 월드 상태를 섞고 잘못된 슬롯을 덮어쓴다.
- **제안**: 저장 요청마다 대상 `UWxSaveGame`, 슬롯, 사용자 인덱스를 캡처한 불변 컨텍스트를 사용한다. 단순한 정책을 원하면 저장 중 슬롯 교체·로드·삭제·트래블을 거부하거나 완료 뒤 큐잉한다.
- **확신도**: 높음

### 4. 🔴 다른 스트리밍 레벨의 GE Instigator가 아직 복원되지 않으면 저장 효과를 영구 삭제한다
- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:364`, `Plugins/WxSave/Source/WxSave/Private/WxPersistableActorReference.cpp:70`, `Plugins/WxSave/Source/WxSave/Private/WxPersistableActorReferenceManager.cpp:119`
- **범주**: 버그/정확성
- **문제**: 효과 소유자 레벨의 `PostRestoreLevel`에서 복원을 시작한 뒤 Instigator `Resolve()`가 실패하면 해당 효과를 즉시 `RemoveAtSwap`한다. Instigator가 다른 스트리밍 레벨에 있고 그 레벨이 나중에 복원되는 정상 순서에서도 효과가 유실된다. WxSave에는 이를 기다리기 위한 `ResolveOrRegister`가 이미 있지만 소비자가 사용하지 않는다.
- **제안**: `LevelActor` 참조는 Instigator 레벨의 `ResolveOrRegister`에 등록하고 성공 또는 해당 레벨의 복원 완료 실패가 확정될 때까지 효과를 보류한다. 플레이어 참조와 즉시 해석 가능한 참조만 현재 경로에서 처리한다.
- **확신도**: 높음

### 5. 🔴 직접 배치한 적은 HP 0만 복원되고 사망 상태로 전이하지 않을 수 있다
- **위치**: `Config/DefaultEngine.ini:142`, `Source/WxGame/Character/WxEnemyCharacter.cpp:72`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:106`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:157`
- **범주**: 버그/정확성
- **문제**: `ShouldPersistRuntimeActor()`의 사망 필터는 런타임 액터에만 적용된다. 맵에 직접 배치한 적은 LSP가 HP base 0을 저장·복원하며, 복원은 ASC `SetNumericAttributeBase`만 호출하고 피해 처리의 `Event.Death`를 발행하지 않는다. 사망 어빌리티가 액터를 파괴하지도 않으므로 레벨 재생성 뒤 HP 0이지만 AI·콜리전·사망 태그가 살아 있는 적이 생길 수 있다.
- **제안**: 적은 스포너 런타임 생성만 허용하고 직접 배치를 검증에서 막거나, 직접 배치 적의 파괴/사망 상태도 명시적으로 영속화한다. HP 0 복원 시에는 중복 안전한 사망 전이 경로를 호출한다.
- **확신도**: 중간(직접 배치 적을 금지하는 콘텐츠 규약이면 발생하지 않음)

### 6. 🟡 하위 단계 실패와 유효한 빈 상태를 구분하지 않아 부분 저장도 성공으로 보인다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:42`, `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:186`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:279`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:545`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:328`
- **범주**: 버그/정확성
- **문제**: Mass subsystem 부재·fragment 해석 실패는 빈 snapshot 성공과 같은 값으로 완료돼 기존 정상 Mass 데이터를 지운다. LSP 직렬화 실패는 이전 LSP 데이터를 유지한 채 새 IAM/Mass 상태와 함께 계속 저장한다. 마지막 디스크 `bSuccess`도 로그에만 쓰이며 `OnSaveCompleted`에는 전달되지 않아 체크포인트는 혼합·실패 저장도 성공으로 끝낸다.
- **제안**: 각 플러시 파트와 디스크 커밋에 성공 여부를 두고, 하나라도 실패하면 기존 스냅샷을 유지한 채 전체 요청을 실패로 완료한다. 실제 엔티티 0개의 성공만 빈 배열로 커밋한다.
- **확신도**: 높음

### 7. 🟡 BP 저장·로드 API가 클라이언트에서도 서버 소유 슬롯을 직접 조작한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp:58`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:39`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:121`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:173`
- **범주**: 설계/구조
- **문제**: 월드 서브시스템은 `NM_Client`에서 생성하지 않지만 BP Function Library와 GameInstance 서브시스템에는 같은 권위 게이트가 없다. 클라이언트 `SaveToFile`은 월드 플러시 없이 로컬 기본 슬롯을 기록하고, 로드·삭제·트래블 API도 로컬에서 실행한다. 서버가 세이브를 소유한다는 StateTree 경계와 다르다.
- **제안**: 실제 상태 변경 주체인 `UWxSaveGameSubsystem`에서 `NM_Client` 호출을 거부하고 명시적 실패를 반환한다. 멀티플레이 저장이 필요하면 RPC와 플레이어별 데이터 소유권을 별도 계약으로 둔다.
- **확신도**: 중간(제품이 엄격한 싱글플레이 전용이면 영향 없음)

### 8. 🟡 접수된 ServerTravel이 나중에 실패하면 로드 중 플래그가 영구 고정된다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:121`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:259`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:378`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:523`
- **범주**: 버그/정확성
- **문제**: `ServerTravel`의 즉시 거절만 `bTravelingFromSaveFile`을 해제한다. 접수 후 맵 로드가 실패해 새 월드의 `ReportTravelFromSaveFileComplete`가 오지 않으면 플래그가 남고, 이후 IAM 스트리밍 아웃 캡처와 teardown 플러시가 계속 건너뛰어진다.
- **제안**: 엔진 travel failure delegate를 구독해 해당 요청 실패 시 플래그를 해제한다. 요청 월드와 목표 맵도 함께 저장해 무관한 실패와 구분한다.
- **확신도**: 중간

### 9. 🟡 완료 브로드캐스트 뒤 Clear가 재진입 중 등록된 다음 저장 대기자를 지운다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:341`
- **범주**: 버그/정확성
- **문제**: 기존 완료 콜백이 동기적으로 다음 저장을 시작하고 새 `OnSaveCompleted` 콜백을 등록하면, 현재 `Broadcast()` 직후의 `Clear()`가 새 대기자까지 삭제할 수 있다. 다음 저장은 완료돼도 StateTree나 UI가 통지받지 못한다.
- **제안**: 발화 전에 현재 delegate 목록을 로컬로 옮기고 원본을 비운 뒤 로컬만 브로드캐스트한다. 발화 중 등록된 콜백은 다음 요청용 원본에 남긴다.
- **확신도**: 높음

### 10. 🟡 OnPostRestoreLevel이 IWxSavable의 오브젝트 계약과 달리 액터에만 전달된다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxSavable.h:20`, `Plugins/WxSave/Source/WxSave/Private/WxSaveModule.cpp:141`, `Plugins/WxSave/Source/WxSave/Public/WxPersistableReferencedActorComponent.h:13`
- **범주**: 설계/구조
- **문제**: `IWxSavable`은 UObject·ActorComponent·AttributeSet이 함께 구현하는 계약이지만 레벨 완료 훅은 `Level->Actors` 중 인터페이스 액터만 순회한다. 컴포넌트나 서브오브젝트가 `OnPostRestoreLevel`을 구현해도 호출되지 않으며 인터페이스 문서만으로 이 제한을 알 수 없다.
- **제안**: 훅을 actor 전용 별도 계약으로 좁히거나, LSP에 등록된 savable subobject까지 중복 없이 전달하는 명시적 수집 경로를 둔다.
- **확신도**: 높음

### 11. 🟡 scene attachment를 수명 추적에 겸용해 Character 이동 복제와 무관한 자식 액터를 침범한다
- **위치**: `Source/WxGame/Character/WxEnemyCharacter.cpp:210`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:50`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:137`
- **범주**: 설계/구조
- **문제**: 이동하는 Character를 스포너에 `AttachToActor`하여 복제 시 `AttachmentReplication` 경로를 타게 하므로 원격 CMC 이동·스무딩 검증이 필요하다. 동시에 스포너는 붙어 있는 모든 액터를 복원 적으로 간주해 자동 스폰을 막고 `Respawn`·`EndPlay`에서 파괴한다. 디자이너가 붙인 표식·연출·보조 액터도 적과 같은 수명으로 처리된다.
- **제안**: 스포너 연관성을 별도 약참조/식별 컴포넌트로 표현하고 scene attachment와 분리한다. 전환 전까지는 클래스, `IWxSpawnable`, 역참조 스포너 일치까지 확인한 액터만 탐색·삭제한다.
- **확신도**: 높음(실제 원격 이동 품질 영향은 네트워크 PIE 검증 필요)

### 12. 🟡 플레이어 상태가 한 칸뿐이라 멀티플레이에서 첫 플레이어와 첫 빙의자가 상태를 공유한다
- **위치**: `Source/WxGame/Framework/WxWorldSettings.cpp:32`, `Source/WxGame/Framework/WxWorldSettings.cpp:47`, `Source/WxGame/Character/WxPlayerCharacter.cpp:59`
- **범주**: 설계/구조
- **문제**: 캡처는 `GetFirstPlayerController()` 하나를 `PlayerPersistenceState` 단일 필드에 저장하고, 복원은 먼저 `PossessedBy`에 도달한 플레이어가 그 상태를 소비해 pending 플래그를 내린다. 두 명 이상이면 다른 플레이어의 스탯·GE가 오적용되거나 나머지가 복원되지 않는다.
- **제안**: 싱글플레이 전용 전제를 런타임 검증과 문서에 명시하거나, 안정적인 플레이어 ID별 상태 맵과 각 컨트롤러의 복원 완료 상태를 둔다.
- **확신도**: 중간(싱글플레이 전용이면 의도된 제한)

### 13. 🟡 스탯 저장 대상이 CPF_Net 추론과 세 곳의 수동 목록으로 갈라져 쉽게 어긋난다
- **위치**: `Source/WxGame/Framework/WxWorldSettings.cpp:80`, `Config/DefaultEngine.ini:142`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:157`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:195`
- **범주**: 중복/복잡도
- **문제**: 플레이어는 `CPF_Net`인 모든 `FGameplayAttributeData`를 저장 대상으로 추론하지만 LSP AttributeSet은 config 16개, pending 캡처 16개, ASC 적용 16개를 각각 수동 관리한다. 새 replicated 임시 속성이 플레이어 저장에 섞이거나, 새 영속 속성을 세 목록 중 하나에서 빠뜨려 플레이어와 월드 액터의 결과가 달라질 수 있다.
- **제안**: 명시적 metadata/allowlist 하나를 정의하고 config 생성·캡처·적용을 같은 테이블에서 파생한다. 네트워크 복제 여부와 영속 여부는 별도 의미로 둔다.
- **확신도**: 높음

### 14. 🟡 AttributeSet pending base 적용이 AWxCharacterBase 초기화 경로에만 의존한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:182`, `Source/WxGame/Character/WxCharacterBase.cpp:216`
- **범주**: 설계/구조
- **문제**: LSP가 ASC 등록 전에 복원한 base 값은 `PendingRestoredBaseValues`에 남고, 현재는 `AWxCharacterBase::InitAbilitySystem()`만 `ApplyPendingSaveRestore()`를 다시 호출한다. 같은 AttributeSet을 다른 ASC host에서 사용하면 pending 값이 ASC API로 게시되지 않아 aggregator/current 값이 기본값과 어긋날 수 있다.
- **제안**: ASC가 AttributeSet 등록을 완료하는 공통 훅에서 pending 적용을 호출하거나, WxCombat이 독립적으로 완료 시점을 관찰하는 계약을 제공한다.
- **확신도**: 중간(현재 모든 host가 AWxCharacterBase이면 발생하지 않음)

### 15. 🟡 런타임 픽업 복원이 soft asset을 개별 동기 로드한다
- **위치**: `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:124`
- **범주**: 성능/안전
- **문제**: LSP가 많은 픽업을 한꺼번에 재생성할 때 각 `OnSaveRestored`가 `ItemDefinition.LoadSynchronous()`를 호출한다. 정의가 메모리에 없으면 게임 스레드에서 연속 로드가 발생해 스트리밍 인·세이브 로드 프레임이 크게 멈출 수 있다.
- **제안**: 저장 데이터에서 필요한 item definition을 레벨 복원 전에 일괄 preload하거나 async load 완료 후 시각·물리 상태를 적용한다.
- **확신도**: 중간(실제 hitch 크기는 콘텐츠 수와 캐시 상태 측정 필요)

### 16. 🟡 참조 컴포넌트가 등록 키를 덮어써 EndPlay에서 이전 키를 해제하지 못한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxPersistableReferencedActorComponent.cpp:15`, `Plugins/WxSave/Source/WxSave/Private/WxPersistableReferencedActorComponent.cpp:28`, `Plugins/WxSave/Source/WxSave/Private/WxPersistableReferencedActorComponent.cpp:51`, `Plugins/WxSave/Source/WxSave/Private/WxPersistableActorReferenceManager.cpp:62`
- **범주**: 버그/정확성
- **문제**: 복원 시 이전 세션 경로·이름으로 manager에 등록한 뒤 다음 저장의 `OnSavePreparing`이 같은 필드를 현재 경로·이름으로 덮어쓴다. `EndPlay`는 변경된 키로 unregister하므로 원래 `RegisteredActors` 항목이 남고, manager도 만료된 weak entry를 정리하지 않는다. 장시간 스트리밍·반복 저장에서 stale key가 누적되고 이름 재사용 시 해석이 모호해진다.
- **제안**: persisted identity와 현재 등록된 transient key를 분리해 정확한 키로 unregister한다. 조회·레벨 종료 시 invalid weak entry도 제거한다.
- **확신도**: 높음

### 17. 🟢 delegate callback 두 개가 프로젝트의 Handle prefix 규칙을 따르지 않는다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:64`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:215`
- **범주**: 규칙 위반
- **문제**: `OnLevelBeginMakingInvisible`에 바인딩한 `FlushInstancedActorManagerDataForLevel`과 `FWxOnMassPreSnapshot`에 바인딩한 `PerformPreSaveMassTasks`가 AGENTS.md의 delegate callback `Handle` prefix 규칙을 위반한다.
- **제안**: 작업 함수는 유지하고 바인딩 전용 `HandleLevelBeginMakingInvisible`, `HandleMassPreSnapshot` 래퍼를 추가하거나 함수명을 규칙에 맞춘다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveModule.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxPersistableActorReferenceManager.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxPersistableReferencedActorComponent.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxPersistedMassSpawner.cpp` 및 대응 Public 헤더
- **훑은 파일**: `Plugins/WxSave/README.md`, `Plugins/WxSave/WxSave.uplugin`, `Plugins/WxSave/Source/WxSave/WxSave.Build.cs`, 나머지 `Public/`·`Private/` C++ 전부
- **미검토 / 한계**: 외부 통합 파일인 `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Framework/WxWorldSettings.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Config/DefaultEngine.ini`은 WxSave 계약과 맞닿는 부분만 확인했다. BP/WBP와 EntityConfig/DataAsset 실제 값은 범위 밖이며, 손상 슬롯·네트워크 PIE·대규모 Mass 런타임 실패 주입과 성능 측정은 수행하지 않았다. 저작권 첫 줄, `Wx` prefix, `BlueprintCallable`, inline 예외, Wx 플러그인의 `WxCore` 전용 의존 경계는 위 17번 외 위반을 찾지 못했다.

---
*문서 기준 커밋 `ba33d69e` · 리뷰일 2026-09-01 · 소스 25파일 — `/module-review`로 갱신*
