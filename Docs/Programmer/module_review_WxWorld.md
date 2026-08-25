# WxWorld — 코드 리뷰

> 전반적으로 건강한 모듈이다. 서버 권위·복제 추종·복원 수렴의 경계가 코드와 doc-comment 양쪽에서 일관되게 지켜지고, 프로젝트 코딩·모듈 규칙 위반은 이번 전수 확인에서 하나도 나오지 않았다(Copyright 첫 줄·`Wx` prefix·`Handle` prefix·`BlueprintCallable`·인라인 정의·람다·`WxCore` 외 Wx 의존 전부 통과). 이번 리뷰는 README 지도를 따라 `AWxDevice`·`UWxDeviceStateTreeComponent`·`UWxInteractionScannerComponent`·`AWxSpawner`의 헤더+cpp를 깊게 보고, `Device`/`Interaction`/`Spawnable`의 StateTree Task 16종과 라이브러리·세팅·모듈·Build.cs 를 훑었으며, 스캐너·스포너의 모듈 밖 호출부(`Source/WxGame`)까지 따라가 실제 도달 가능성을 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 트리에 닿지 않는 상호작용이 클라 전원에 헛재진입 연출을 일으킨다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:77`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:152`
- **범주**: 버그/정확성
- **문제**: `OnInteracted`가 `NotifyInteractionPending()`을 먼저 세우고(`:77`) 그다음 `BroadcastInteractionDelegate()`를 부르는데(`:79`), 후자가 실제로 트리에 닿았는지를 보지 않는다. 닿지 않으면 서버 트리는 아무 일도 하지 않은 채 다음 권위 틱의 `PublishAuthorityState`가 「상태가 안 바뀐 재진입」으로 오판해 `Multicast_ReenterState`를 쏘고(`Private/Device/WxDeviceStateTreeComponent.cpp:165-169`), 클라 전원이 현재 상태를 재선택해 몽타주·사운드·나이아가라·레벨시퀀스를 헛재생한다. 누를 때마다 반복된다.
  가장 확실한 도달 경로는 **빈 바인딩**이다. 남의 트리가 `EnableInteraction`(TargetKind=Actor)로 이 장치를 켜면 `SetInteractionEnabled(true)`만 불려 `bInteractionEnabled`가 서고(`WxDevice.cpp:47-51`), `InteractionBinding`은 기본값(빈 `Dispatcher`·무효 `Context`) 그대로다. 그런데 `CanInteract()`는 `bInteractionEnabled`만 보므로(`:42-45`) 스캔 후보에 오르고, 눌리면 위 경로를 그대로 탄다. 자기 트리가 '상호작용 켜기'를 한 번도 진입한 적 없는 장치가 정확히 이 상태다.
  두 번째 경로는 **떠난 상태의 컨텍스트**다. `InteractionBinding.Context`는 '상호작용 켜기' 태스크가 진입할 때 캡처한 약한 실행 컨텍스트라(`Private/Interaction/WxStateTreeTask_EnableInteraction.cpp:43`), 그 상태를 이미 떠났으면 발행이 조용히 버려진다. 끌 때 바인딩을 비우지 않는 설계(`WxDevice.cpp:130-135`)라 상태가 바뀌어도 낡은 컨텍스트가 남는다.
  같은 함정을 `NotifyDeviceInteracted`는 이미 인지해 이벤트 경로에서는 플래그를 걸지 않기로 했다(`WxDevice.cpp:117` 주석). 상호작용 경로에만 그 방어가 없다.
- **제안**: 발행이 성립했는지 확인한 뒤에만 `NotifyInteractionPending()`을 걸도록 순서를 뒤집는다 — `BroadcastInteractionDelegate()`가 「유효한 컨텍스트 + 유효한 Dispatcher 로 실제 발행했는가」를 bool 로 답하게 하고 그 값으로 가른다. 최소 조치만 하려면 `OnInteracted` 초입의 게이트(`:62`, `:68`)에 바인딩 유효성 검사를 하나 더 붙여 빈 바인딩 장치를 상호작용 후보에서 빼는 방법도 있다.
- **확신도**: 높음

### 2. 🟡 `GetPrompts()`가 선언한 인덱스 정합을 실제로는 지키지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:79`
- **범주**: 버그/정확성
- **문제**: 주석(`:81`)은 "인덱스 정합을 위해 대상이 없으면 빈 텍스트로 자리를 채운다"고 선언하는데, 코드는 `if (AActor* Actor = Weak.Get())`로 죽은 약참조를 **건너뛴다** — 자리를 채우는 것이 아니라 항목 자체가 빠진다. `SelectedIndex`는 `InRangeActors` 기준 인덱스인데 뷰모델은 이 프롬프트 배열 기준으로 하이라이트하므로, `InRangeActors`에 죽은 항목이 하나라도 섞이면 HUD가 엉뚱한 줄을 선택 표시한다.
  `UpdateInRange` 내부 호출(`:237`)은 바로 앞 루프(`:202-211`)가 죽은 항목을 걷어낸 뒤라 안전하지만, `GetPrompts()`는 public 이고 뷰모델이 임의 시점에 초기 시드로 직접 읽는다(`Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:43-44`가 `GetPrompts()`와 `GetSelectedIndex()`를 나란히 읽는다). 스캔 간격(0.1s) 사이에 in-range 액터가 파괴되면 그 창에 걸린다.
- **제안**: 주석대로 `Prompts.Add(FText::GetEmpty())`로 자리를 채우거나, 정합이 필요 없다면 주석을 코드에 맞춘다. 호출부 계약(프롬프트 배열 인덱스 = 선택 인덱스)과 맞는 것은 전자다.
- **확신도**: 높음

### 3. 🟡 두 대기 태스크가 레지스트리 인프라를 통째로 중복 구현하고, 이미 한 지점이 갈라졌다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:11-95`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:12-124`
- **범주**: 중복/복잡도
- **문제**: 익명 namespace 의 전역 대기 배열 + 재사용하지 않는 핸들 카운터 + 통보 시 역순 순회·죽은 오너 청소·`FinishTask` + `ExitState`의 선형 제거까지, 두 파일이 구조·주석 문구·변수 명명까지 사실상 같은 코드다. 대기형 태스크가 하나 늘 때마다 같은 40여 줄이 복제되고, 한쪽만 고치면 조용히 갈라진다.
  실제로 이미 갈라졌다: 로케이터 해석 컨텍스트를 `WaitSpawnersKilled`는 **자기 오너**로(`:72` `AreAllSpawnersKilled(Wait.Spawners, Owner.Get())`), `WaitForInteraction`은 **통보를 보낸 액터**로(`:55` `Wait.Target.SyncFind(Target)`) 잡는다. 후자는 자기 월드가 아닌 곳에서 온 통보로도 해석이 성립하므로, 서버·클라 월드가 한 프로세스에 공존하는 PIE 에서 서버 월드의 상호작용 통보가 클라 월드에 등록된 대기를 완료시킬 수 있다(전역 배열이 월드로 나뉘지 않는다). 실제 빌드에선 통보가 ServerOnly 어빌리티에서만 오고 추종이 상태를 되끌어와 가려지지만, 해석 기준이 다르다는 것 자체가 갈라짐의 증거다.
- **제안**: 「약한 실행 컨텍스트 + 핸들」 등록/해제/스윕을 `Private/`의 공용 헬퍼(struct 또는 템플릿) 한 곳으로 뽑고 두 태스크는 조건 판정만 넘긴다. 그 참에 해석 컨텍스트를 자기 오너로 통일하고, 등록 시 오너의 월드를 함께 담아 통보를 같은 월드로 좁힌다.
- **확신도**: 높음

### 4. 🟡 UOL 표시명 헬퍼가 3중, `Compile()` 검증이 2중으로 복제돼 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp:43-65`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp:115-137`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:106-128`
- **범주**: 중복/복잡도
- **문제**: 세 함수(`GetSpawnerLocatorDisplayName`·`GetTargetDisplayName`×2)의 본문이 빈 로케이터 문구("none"/"unset")만 빼고 줄 단위로 같다 — 액터 라벨 → 마지막 프래그먼트 payload → SubPath 뒤쪽 잘라내기 → "unresolved" 폴백. `UWxSpawnerLibrary::GetSpawnerLocatorDisplayName`은 같은 모듈의 `WITH_EDITOR` public static 이라 두 태스크가 그대로 부를 수 있는데도(`WaitSpawnersKilled`·`TriggerSpawnersByLocator`는 실제로 그렇게 쓴다) 각자 다시 썼다.
  `Compile()` 본문도 주석까지 포함해 완전히 동일하다 — `Private/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.cpp:58-78`과 `Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:127-147`. 부수적으로 `AWxDevice::PreSave`(`Private/Device/WxDevice.cpp:29-39`)와 `AWxSpawner::PreSave`(`Private/Spawnable/WxSpawner.cpp:193-205`)의 SaveId 확정 로직도 같은 패턴의 2중 복제인데, 후자에만 「왜 PreSave 인가」를 설명하는 주석이 붙어 있다.
- **제안**: 표시명은 `UWxSpawnerLibrary::GetSpawnerLocatorDisplayName` 하나로 모으고(빈 로케이터 문구는 인자나 호출부에서 처리), UOL 배열의 클래스 검증도 같은 라이브러리에 `ValidateLocatorsAre<T>` 형태로 한 번만 둔다. SaveId 확정은 두 액터가 공유할 자리가 마땅치 않으면 최소한 주석을 한쪽에 맞춘다.
- **확신도**: 높음

### 5. 🟢 서로 다른 두 Task 가 같은 `DisplayName`("스포너 발동")을 쓴다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Device/WxStateTreeTask_TriggerSpawners.h:30`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.h:34`
- **범주**: 설계/구조
- **문제**: `meta = (DisplayName = "스포너 발동", Category = "Wx")`가 두 USTRUCT 에 동일하게 걸려 있다(모듈의 다른 14개 Task 는 전부 고유하다). 노드 픽커에 같은 이름이 둘 뜨고, 저작자는 어느 쪽이 바인딩형(`TSoftObjectPtr` 배열)이고 어느 쪽이 리터럴 지정형(UOL)인지 이름만으로 구분할 수 없다. 런타임 `GetDescription`도 둘 다 "스포너 발동 (…)"으로 시작해 노드 목록에서도 갈라지지 않는다.
- **제안**: 한쪽을 `"스포너 발동 (지정)"`처럼 갈라 놓는다. doc-comment 가 서로를 지목할 때 쓰는 영문 이름(`Trigger Spawners` / `Trigger Spawners By Locator`)과 대응되게 맞추면 더 좋다.
- **확신도**: 높음

### 6. 🟢 스포너 셀이 스트리밍 아웃되면 멀리 있는 스폰 대상까지 파괴된다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:134`
- **범주**: 설계/구조
- **문제**: `EndPlay`가 이유를 가리지 않고 권위에서 `SpawnedActor`를 `Destroy()` 한다. `SpawnTarget`은 복제 스무딩 때문에 의도적으로 스폰 대상을 스포너에 attach 하지 않으므로(`:184-187` 주석), 스폰된 적은 플레이어를 따라 다른 셀로 이동할 수 있다. 그 상태에서 스포너가 놓인 WP 셀이 언로드되면 `EEndPlayReason::RemovedFromWorld`로 `EndPlay`가 돌아 전투 중인 적이 사라진다.
- **제안**: `EndPlayReason`으로 갈라 `Destroyed`/`LevelTransition`/`Quit` 등에서만 정리하고, 스트리밍 언로드에서는 소유권을 놓아주거나(또는 대상 액터를 `bIsSpatiallyLoaded=false`로 저작) 리스폰 경로가 다시 붙잡게 한다. 의도된 동작이라면 그 이유를 `EndPlay` 자리에 주석으로 남긴다 — 같은 파일의 다른 미묘한 결정들은 전부 주석이 붙어 있어 이 자리만 비어 있다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_SendEvent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp`
- **훑은 파일**: `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayAnimation.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlaySound.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_SpawnNiagara.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_RespawnSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeComponentName.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnable.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxWorldDeveloperSettings.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/WxWorldModule.cpp` 및 대응 Public 헤더 전부, 모듈 밖 호출부 `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`
- **미검토 / 한계**:
  - 이 환경엔 언리얼 엔진이 없어 빌드·PIE 실측을 하지 않았다. 특히 finding 1 의 「발행이 닿지 않는다」와 finding 3 의 PIE 월드 교차는 엔진 `FStateTreeWeakExecutionContext`의 내부 동작에 기대므로 코드 독해로만 판단했다(빈 바인딩으로 도달하는 경로는 이 모듈 코드만으로 성립한다).
  - `AWxSpawner`의 `#if WITH_EDITOR` 프리뷰·라벨 경로(`PostRegisterAllComponents`·`PostEditChangeProperty`·`UpdateEditorPreviewFromSpawnableClass`)는 읽었으나 실제 에디터 동작(T3D 붙여넣기·WP 셀 재열기 시 SaveId·라벨 거동)을 재현 검증하지는 않았다.
  - 장치 StateTree 에셋·BP(`Plugins/WxWorld/Content/`, `Content/Quest/Steps/`)의 내부 저작은 리뷰 범위 밖이라, finding 1 의 「어떤 배선이 빈 바인딩 장치를 켜는가」는 코드 가능성만 확인했고 실제 에셋에서 그 조합이 쓰이는지는 확인하지 않았다.
  - `FWxStateTreeComponentName`의 짝인 에디터 커스터마이제이션(`FWxStateTreeComponentNameCustomization`)은 `Source/WxEditor`에 있어 이번 모듈 범위 밖이다.
  - 멀티플레이 실측(늦은 조인·패킷 유실 하의 `StateTag` 수렴, `Multicast_ReenterState` 타이밍, `InteractingCharacter`와 `StateTag`의 도착 순서)은 코드 독해로만 판단했다.

---
*문서 기준 커밋 `cf3a7a0` · 리뷰일 2026-08-25 · 소스 50파일 — `/module-review`로 갱신*
