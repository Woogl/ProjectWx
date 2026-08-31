# WxSave — 코드 리뷰

> 엔진 LSP·Instanced Actors·Mass를 하나의 슬롯으로 묶는 구조는 경계가 명확하고 의존성도 `WxCore`만 참조해 깨끗하다. 다만 저장 완료 판정을 Mass FrameEnd 페이즈 한 곳에 걸어둔 탓에 정지 상태에서 저장이 조용히 멈추는 실패 경로가 있다. 이번 리뷰는 25개 소스 전부를 훑고 `WxSaveWorldSubsystem`·`WxMassPersistence`·`WxSaveGameSubsystem`의 저장/복원 흐름과 `Config/DefaultEngine.ini`의 LSP 설정, 엔진 측 계약(LSP 모듈 델리게이트 디스패치, Mass 페이즈 틱, `GetFragmentDataStruct` 반환 규약)까지 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 7 |
| 🟢 사소 | 3 |

## 결과

### 1. 🔴 게임이 정지된 동안 저장 요청이 무기한 보류되고, 이후 모든 저장이 거절된다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:96`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:160`
- **범주**: 버그/정확성
- **문제**: `SnapshotEntities`는 teardown이 아닐 때 완료 신호를 `EMassProcessingPhase::FrameEnd` 페이즈에만 건다. 이 페이즈는 `TG_LastDemotable` 틱 함수이고 `bTickEvenWhenPaused`가 false라, 일시정지 프레임에서는 `bTickEvenWhenPaused` 틱만 도는 엔진 경로(`TickTaskManager.cpp`의 `LEVELTICK_PauseTick`) 때문에 아예 돌지 않는다. `UWxUIManagerSubsystem::RefreshGamePause`가 `bPauseGame` 위젯(`UWxGamePopup` 포함)에서 `SetGamePaused`를 부르므로 "정지 메뉴에서 저장"이 바로 이 경로다. 그러면 `SaveToFile`은 true를 돌려주고도 디스크 기록이 시작되지 않고, `bSaveInProgress`가 true로 남아 정지가 풀릴 때까지 뒤따르는 모든 저장 요청이 `SaveToFile: 이전 저장이 진행 중이라 요청 거절`로 반려된다. `FWxStateTreeTask_SaveGame`도 그동안 Running에 묶인다. 정지 상태에서 앱을 종료하면 그 저장은 사라진다. Mass 시뮬레이션이 아직 시작되지 않은 시점의 저장도 같은 방식으로 걸린다.
- **제안**: teardown 분기(`WxMassPersistence.cpp:33`)와 같은 이유가 정지에도 성립한다 — 프로세서가 돌지 않으므로 FrameEnd 경계를 기다릴 필요가 없다. `World->IsPaused()`이거나 `UMassSimulationSubsystem::IsSimulationStarted()`가 false면 즉시 동기 스냅샷 경로를 타게 한다. (타이머 폴백은 답이 아니다 — `FTimerManager::Tick`도 정지 프레임에서는 돌지 않는다.)
- **확신도**: 높음

### 2. 🟡 Mass fragment 뷰의 널 검사가 없어 nullptr memcpy로 크래시할 수 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:277`, 같은 파일 `:395`
- **범주**: 버그/정확성
- **문제**: `FMassEntityManager::GetFragmentDataStruct`는 엔진 주석이 명시하듯 *엔티티가 그 fragment를 갖고 있지 않으면 빈 FStructView*를 돌려준다. 두 지점 모두 `GetMemory()`를 검사 없이 `FMemoryWriter/FMemoryReader::Serialize`에 넘기므로 그 경우 nullptr 원본/대상 memcpy가 된다. 저장 쪽은 엔티티를 "EntityConfig 값"으로 묶고 fragment 목록은 "템플릿 아키타입"에서 뽑기 때문에 런타임에 아키타입이 갈린 엔티티가 섞이면 어긋난다. 복원 쪽이 더 현실적이다 — 저장 이후 EntityConfig를 편집해 fragment가 빠지면, `Resolved.Struct`는 로드·크기·허용목록 3검사를 모두 통과하지만 새로 스폰된 엔티티의 아키타입에는 그 fragment가 없어 옛 세이브를 여는 순간 죽는다. 지금은 허용 목록이 `TransformFragment` 하나뿐이라 드러나지 않는다.
- **제안**: 두 지점 모두 `FragmentView.GetMemory()`를 확인하고, 저장 쪽은 해당 엔티티를 그룹에서 제외(바이트 스트림 정합 유지), 복원 쪽은 `Reader.Seek`로 건너뛰며 경고를 남긴다.
- **확신도**: 높음(메커니즘) / 중간(현재 설정에서의 발생 빈도)

### 3. 🟡 스트리밍 레벨 키에 PIE 접두사가 그대로 들어간다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:385`, 같은 파일 `:492`
- **범주**: 버그/정확성
- **문제**: 맵 키는 `GetStableMapPackageName`이 `UWorld::RemovePIEPrefix`로 안정화하지만, 레벨 키는 `LevelStreaming->GetWorldAssetPackageFName()`을 그대로 쓴다. `ULevelStreaming::RenameForPIE`가 `WorldAsset`을 PIE 패키지 이름으로 바꾸므로 PIE에서는 이 값이 `/Game/UEDPIE_0_...` 형태다. 결과적으로 PIE에서 만든 슬롯의 IAM 델타는 패키징 빌드에서 키가 맞지 않아 조용히 무시되고(파괴한 Instanced Actor가 되살아난다), PIE 인스턴스 ID가 달라지는 실행 구성에서도 같은 일이 벌어진다. `Public/WxSaveGame.h:67`의 "PIE 접두사에 영향받지 않는 스트리밍 레벨 패키지 이름별"이라는 계약과도 어긋난다. 퍼시스턴트 레벨 폴백은 안정화된 `MapKey`를 쓰므로 한 함수 안에서 두 규칙이 섞여 있다.
- **제안**: 레벨 키도 `UWorld::RemovePIEPrefix`를 통과시킨다(저장·복원 두 지점 동일). 기존 슬롯은 포맷 버전을 올려 폐기한다.
- **확신도**: 높음

### 4. 🟡 `UWxSaveLibrary::SaveToFile`에 권위 가드가 없어 클라이언트가 슬롯을 비우며 덮어쓴다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp:68`
- **범주**: 설계/구조
- **문제**: `UWxSaveWorldSubsystem::ShouldCreateSubsystem`이 `NM_Client`를 배제하므로 클라이언트에는 월드 서브시스템이 없다. 그 상태에서 `SaveToFile`이 불리면 `WxSaveGameSubsystem.cpp:190`의 else 분기를 타 플러시 없이 곧장 디스크에 기록한다 — 클라이언트의 인메모리 SaveGame은 `Initialize`가 만든 빈 것이므로 로컬 슬롯 파일이 빈 세이브로 덮인다. `FWxStateTreeTask_SaveGame`은 `HasAuthority()`로 막고 README도 "저장 파일은 서버가 소유"라고 적었는데 BP 진입점만 무방비다.
- **제안**: 라이브러리(또는 `UWxSaveGameSubsystem::SaveToFile` 진입부)에서 권위가 아니면 false로 조기 반환한다.
- **확신도**: 중간(현재 사실상 싱글플레이라 노출되지 않았을 수 있음)

### 5. 🟡 Mass fragment를 구조체 크기만큼 통째로 memcpy하는데 POD 가드가 없다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:278`, `Plugins/WxSave/Source/WxSave/Private/WxSaveSettings.cpp:13`
- **범주**: 성능/안전
- **문제**: 스냅샷은 `FragmentType->GetStructureSize()`만큼 원시 복사한다. 그런데 설정 드롭다운(`GetMassFragmentOptions`)은 `FMassFragment` 파생 전부를 후보로 내놓는다. `FString`·`TArray`·`TObjectPtr`를 가진 fragment를 허용 목록에 넣으면 힙 포인터가 그대로 디스크에 저장되고 다음 세션에서 되살아나 이중 해제·덤프 불가능한 손상으로 이어진다(경고 한 줄 없음). 지금 등록된 `TransformFragment`는 안전하지만, 확장 규약이 그 제약을 코드나 UI 어디에도 새기지 않았다.
- **제안**: 허용 목록 후보를 만들 때(그리고 스냅샷 직전에) `UScriptStruct`의 `StructFlags`/프로퍼티를 훑어 오브젝트 참조·비POD 멤버가 있으면 제외하고 경고를 남긴다. `FWxPersistableEntityConfigFragment` 자신이 `TObjectPtr`를 가진 예외라는 점(그래서 `:260`에서 제외한다)도 같은 가드로 자연히 걸린다.
- **확신도**: 높음

### 6. 🟡 `FSnapshotState::Fire`가 실행 도중 자기 자신을 해제한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:81`, 같은 파일 `:156`
- **범주**: 성능/안전
- **문제**: 공유 상태의 유일한 소유자는 두 람다의 캡처다. `Fire()` 안에서 두 델리게이트를 모두 `Remove`하면 마지막 `Remove`가 그 람다를 파괴해 `TSharedRef` 참조 수가 0이 되고, `this`가 가리키는 `FSnapshotState`/`FRestoreState`가 아직 스택에 남아 있는 `Fire()` 실행 중에 사라진다. 지금은 마지막 `Remove` 뒤에 멤버 접근이 없어 우연히 살아남지만(로그 한 줄만 추가해도 use-after-free), 실행 중인 델리게이트 인스턴스 자체를 언바인드하는 것이기도 해 계약이 위태롭다.
- **제안**: 람다 본문에서 `TSharedRef` 사본을 지역 변수로 잡은 뒤 `Fire()`를 호출해 함수가 끝날 때까지 수명을 고정한다.
- **확신도**: 중간(현 코드는 동작하나 한 줄만 늘려도 깨짐)

### 7. 🟡 호출자 없는 콜백 레지스트리와 플러시 델리게이트가 남아 있다
- **위치**: `Plugins/WxSave/Source/WxSave/Public/WxPersistableActorReferenceManager.h:40`, `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h:29`
- **범주**: 중복/복잡도
- **문제**: 저장소 전체를 검색해도 `ResolveOrRegister`·`AddOnLevelPostRestoreCallback`·`UnregisterResolveCallback`을 부르는 코드가 없다. 그래서 `PendingCallbacks`/`HandleToKey`/`PendingLevelCallbacks`/`LevelHandleToPath`와 `FirePending`·`FireLevelPending`, `Deinitialize`의 배수 처리까지 약 150줄이 항상 빈 맵 위에서 돈다(`WxPersistableActorReferenceManager.cpp:119`~`:296`). 실제로 쓰이는 것은 `GetPersistedRuntimeActor`(`FWxPersistableActorReference::Resolve` 경유)와 `IsLevelCurrentlyPostRestored`(`Source/WxGame/Character/WxCharacterBase.cpp:336`)뿐이다. 마찬가지로 `OnPreFlushInstancedActorsData`·`OnPreFlushMassEntityData`도 구독자가 하나도 없이 매 저장마다 브로드캐스트된다(`WxSaveWorldSubsystem.cpp:360`, `:542`).
- **제안**: 지연 해석이 실제로 필요해질 때 되살리기로 하고 지금은 걷어낸다. 남긴다면 왜 선행 구축했는지 헤더에 한 줄 근거를 단다.
- **확신도**: 높음

### 8. 🟡 `AWxPersistedMassSpawner::EndPlay`가 엔진 despawn을 통째로 무력화한다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxPersistedMassSpawner.cpp:29`
- **범주**: 설계/구조
- **문제**: `Super::EndPlay` 앞에서 `AllSpawnedEntities.Empty()`를 부르는데, `AMassSpawner::EndPlay`는 그 배열을 근거로 `DoDespawning()`을 수행한다. 따라서 스포너가 있는 레벨이 스트리밍 아웃되거나 액터가 파괴돼도 그 엔티티는 아무도 정리하지 않는다. 월드 teardown이면 어차피 EntityManager가 사라지니 무해하지만, 월드가 살아 있는 채 레벨만 내려가는 오픈월드 스트리밍에서는 주인 없는 엔티티가 남는다. 클래스 주석은 "중복 생성 방지"만 설명해 이 부작용의 의도 여부를 알 수 없다.
- **제안**: 의도라면(복원 세션에서 엔티티를 살려두려는 것) 그 이유를 `EndPlay`에 한 줄로 남긴다. 의도가 아니라면 `EndPlayReason`으로 갈라 `RemovedFromWorld`/`Destroyed`에서는 기본 despawn을 그대로 태운다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 9. 🟢 델리게이트 콜백 `Handle` 접두사와 타입 `Wx` 접두사 누락
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:64`, `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h:25`, `Plugins/WxSave/Source/WxSave/Public/WxPersistableActorReferenceManager.h:30`
- **범주**: 규칙 위반
- **문제**: 코딩 규칙 4에 따라 델리게이트에 바인딩되는 콜백은 `Handle` 접두사를 써야 하는데 `OnLevelBeginMakingInvisible`에 `FlushInstancedActorManagerDataForLevel`이 직접 물린다(같은 파일 `:364`에서 직접 호출도 하므로 겸용 이름이 필요하다). 규칙 1의 `Wx` 접두사도 `FPreFlushInstancedActorsData`·`FPreFlushMassEntityData`·`FOnSaveFlushComplete`·`FOnRuntimeActorResolved`·`FOnLevelPostRestored`에서 빠져 있다 — 같은 모듈의 `FWxOnMassPreSnapshot` 계열(`WxMassPersistence.h:10`)은 지키고 있어 한 모듈 안에서 규칙이 갈린다.
- **제안**: 얇은 `HandleLevelBeginMakingInvisible`을 두고 그 안에서 기존 플러시 함수를 부른다. 공개 헤더의 델리게이트 타입에 `Wx`를 붙인다.
- **확신도**: 높음

### 10. 🟢 허용 fragment 목록을 저장·복원에서 각각 다시 만든다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp:198`, 같은 파일 `:307`
- **범주**: 중복/복잡도
- **문제**: 같은 `MassFragmentsToSerialize` 순회가 두 벌인데 한쪽만 미해결 경로에 경고를 남기도록 갈라져 있어, 로그 정책이 조용히 달라진다.
- **제안**: 파일 내부 정적 헬퍼 하나로 합친다.
- **확신도**: 높음

### 11. 🟢 슬롯 로드가 동기라 큰 세이브에서 게임 스레드가 멈춘다
- **위치**: `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp:91`
- **범주**: 성능/안전
- **문제**: 기록은 `AsyncSaveGameToSlot`인데 읽기는 `LoadGameFromSlot` 동기 호출이다. 슬롯에는 방문한 모든 맵의 LSP 블롭과 Mass 스냅샷이 누적되므로(맵마다 수 MB까지) 로드 순간 히치가 그대로 드러난다. 어차피 직후에 `ServerTravel`이 이어지므로 지금은 눈에 덜 띈다.
- **제안**: 슬롯이 커지면 `AsyncLoadGameFromSlot`으로 옮긴다. 당장 바꾸지 않겠다면 대칭이 깨진 이유를 한 줄로 남긴다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**: `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxMassPersistence.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveGameSubsystem.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxPersistableActorReferenceManager.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveModule.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxPersistedMassSpawner.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxStateTreeTask_SaveGame.cpp`, `Plugins/WxSave/Source/WxSave/Public/WxSaveGame.h`, `Plugins/WxSave/Source/WxSave/Public/WxSaveWorldSubsystem.h`
- **훑은 파일**: `Plugins/WxSave/Source/WxSave/Private/WxSaveLibrary.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxPersistableActorReference.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxPersistableReferencedActorComponent.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveSettings.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxPersistableMassTrait.cpp`, 나머지 `Public/*.h` 전부, `Plugins/WxSave/Source/WxSave/WxSave.Build.cs`, `Plugins/WxSave/WxSave.uplugin`, `Config/DefaultEngine.ini`의 LSP·WxSave 섹션
- **미검토 / 한계**: LSP 속성 허용 목록이 실제 저장·복원 대상 액터(`AWxEnemyCharacter`·`AWxItemPickup`·`AWxSpawner`·`AWxWorldSettings`)에서 기대대로 동작하는지는 WxSave 밖의 코드라 이번 범위에서 뺐다. `AInstancedActorsManager::Serialize` 왕복의 엔진 내부 호환성(커스텀 버전 승격 시 구 슬롯 거동)과 Mass 스냅샷의 실제 왕복 정합성은 정적 검토만 했고 실행 검증은 하지 않았다. 저장 데이터가 걸린 BP/에셋(`BP_Player` 등)의 내부 배선은 범위 밖이다.

---
*문서 기준 커밋 `ba33d69e` · 리뷰일 2026-09-01 · 소스 25파일 — `/module-review`로 갱신*
