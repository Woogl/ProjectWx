# WxWorld — 코드 리뷰

> 전반적으로 건강한 모듈이다. 서버 권위·복제 추종·복원 수렴의 경계가 코드와 doc-comment 양쪽에서 일관되게 지켜지고, 규칙 위반은 사실상 없다(Copyright·prefix·BlueprintCallable·FORCEINLINE 전수 확인 통과, `WxCore` 외 Wx 의존 없음). 이번 리뷰는 README 지도를 따라 `AWxDevice`·`UWxDeviceStateTreeComponent`·`UWxInteractionScannerComponent`·`AWxSpawner`의 헤더+cpp를 깊게 보고, `Device`/`Interaction`/`Spawnable`의 StateTree Task 15종과 라이브러리·세팅·모듈을 전수 훑었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 4 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 반응하지 않는 상태의 상호작용이 클라에만 상태 재진입을 일으킨다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:77`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:152`
- **범주**: 버그/정확성
- **문제**: `OnInteracted`가 `NotifyInteractionPending()`을 먼저 세우고 그다음 `BroadcastInteractionDelegate()`를 부르는데, 후자의 성공 여부를 보지 않는다. `InteractionBinding.Context`는 '상호작용 켜기' 태스크가 진입할 때 캡처한 약한 실행 컨텍스트라, 그 상태를 이미 떠났으면 엔진이 `GetActivePathInfo()` 무효로 판단해 발행을 조용히 버린다(`FStateTreeWeakExecutionContext::BroadcastDelegate`는 이 경우 false 반환). 그런데 `bPendingInteractResolve`는 이미 서 있으므로, 다음 권위 틱에서 `PublishAuthorityState`가 「상태가 안 바뀐 재진입」으로 오판해 `Multicast_ReenterState`를 쏜다(`Private/Device/WxDeviceStateTreeComponent.cpp:140-144`). 결과적으로 서버는 아무것도 하지 않았는데 클라만 현재 상태를 재선택해 몽타주·사운드·나이아가라를 헛재생한다.
  구체 시나리오: 상태 A가 상호작용을 켜고, 이를 듣는 전이 없이 상태 B(연출 재생)로 넘어간다. `SetInteractionBinding(false, ...)`를 배선하지 않았으면 B에서도 `bInteractionEnabled`가 true로 남아 플레이어가 다시 누를 수 있고, 누를 때마다 클라 전원이 B를 재진입한다.
  같은 함정을 `NotifyDeviceInteracted`는 이미 인지해 이벤트 경로에서는 플래그를 걸지 않기로 했다(`WxDevice.cpp:117` 주석). 상호작용 경로에는 그 방어가 없다.
- **제안**: `BroadcastInteractionDelegate()`가 엔진의 bool 반환을 그대로 돌려주게 하고, 그것이 true일 때만 `NotifyInteractionPending()`을 걸도록 순서를 뒤집는다. (`Dispatcher`가 아예 비어 있을 때도 true가 오므로, 「살아 있는 컨텍스트인가」만 가리고 싶으면 발행 전에 컨텍스트 유효성을 함께 본다.)
- **확신도**: 중간

### 2. 🟡 `GetPrompts()`가 선언한 인덱스 정합을 실제로는 지키지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:79`
- **범주**: 버그/정확성
- **문제**: 주석은 "인덱스 정합을 위해 대상이 없으면 빈 텍스트로 자리를 채운다"고 선언하는데, 코드는 `if (AActor* Actor = Weak.Get())`로 죽은 약참조를 **건너뛴다** — 자리를 채우는 것이 아니라 항목 자체가 빠진다. `SelectedIndex`는 `InRangeActors` 기준 인덱스이고 뷰모델은 이 프롬프트 배열 기준으로 하이라이트하므로, `InRangeActors`에 죽은 항목이 하나라도 섞이면 HUD가 엉뚱한 줄을 선택 표시한다.
  `UpdateInRange` 내부 호출은 바로 앞 루프(`:202-211`)가 죽은 항목을 걷어낸 뒤라 안전하지만, `GetPrompts()`는 public이고 README대로 뷰모델이 임의 시점에 초기 시드로 직접 읽는다(`Public/Interaction/WxInteractionScannerComponent.h:50`). 스캔 간격(0.1s) 사이에 in-range 액터가 파괴되면 그 창에 걸린다.
- **제안**: 주석대로 `Prompts.Add(FText::GetEmpty())`로 자리를 채우거나, 정합이 필요 없다면 주석을 코드에 맞춘다. 전자가 호출부 계약과 일치한다.
- **확신도**: 높음

### 3. 🟡 두 대기 태스크가 레지스트리 인프라를 통째로 중복 구현한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:11-95`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:12-124`
- **범주**: 중복/복잡도
- **문제**: 익명 namespace의 전역 `TArray<...Wait>` + 재사용하지 않는 핸들 카운터 + 통보 시 역순 순회·죽은 오너 청소·`FinishTask` + `ExitState`의 선형 제거까지, 두 파일이 구조·주석 문구·변수 명명까지 사실상 같은 코드다. 대기형 태스크가 하나 더 늘 때마다 같은 40여 줄이 복제되고, 한쪽만 고치면 조용히 갈라진다.
  실제로 이미 갈라진 지점이 하나 있다: 대상 해석 컨텍스트를 `WaitSpawnersKilled`는 자기 오너로(`:72`), `WaitForInteraction`은 통보를 보낸 액터로(`:55`) 잡는다. 후자는 자기 월드가 아닌 곳에서 온 통보로도 해석이 성립할 수 있는 형태다.
- **제안**: 「약한 실행 컨텍스트 + 핸들」 등록/해제/스윕을 한 곳(예: `Private/`의 공용 헬퍼 struct 또는 템플릿)으로 뽑고 두 태스크가 조건 판정만 넘기게 한다. 그 참에 해석 컨텍스트를 자기 오너로 통일한다.
- **확신도**: 높음

### 4. 🟡 UOL 표시명 헬퍼가 3중, `Compile()` 검증이 2중으로 복제돼 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp:43`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp:115`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp:106`
- **범주**: 중복/복잡도
- **문제**: 세 함수(`GetSpawnerLocatorDisplayName`·`GetTargetDisplayName`×2)의 본문이 빈 로케이터 문구("none"/"unset")만 빼고 바이트 단위로 같다 — 액터 라벨 → 마지막 프래그먼트 payload → SubPath 뒤쪽 잘라내기 → "unresolved" 폴백. `UWxSpawnerLibrary::GetSpawnerLocatorDisplayName`은 이미 같은 모듈의 `WITH_EDITOR` public static이라 두 태스크가 그대로 부를 수 있는데도 각자 다시 썼다.
  `Compile()` 본문(`Private/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.cpp:58-78`과 `Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:127-147`)도 주석까지 포함해 완전히 동일하다. 부수적으로 `AWxDevice::PreSave`(`Private/Device/WxDevice.cpp:29-39`)와 `AWxSpawner::PreSave`(`Private/Spawnable/WxSpawner.cpp:193-205`)의 SaveId 확정 로직도 같은 패턴의 2중 복제다.
- **제안**: 표시명은 `UWxSpawnerLibrary::GetSpawnerLocatorDisplayName` 하나로 모으고(빈 로케이터 문구는 인자나 호출부 처리), UOL 배열의 클래스 검증도 그 라이브러리에 `ValidateLocatorsAre<T>` 형태로 한 번만 둔다.
- **확신도**: 높음

### 5. 🟢 타이머에 바인딩되는 콜백에 `Handle` prefix가 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:42`
- **범주**: 규칙 위반
- **문제**: `ScanAndPush`가 `FTimerManager::SetTimer`로 델리게이트에 바인딩되는데 이름에 `Handle` prefix가 없다(CLAUDE.md 코딩 규칙 4). 같은 프로젝트의 선례는 prefix를 붙인다 — `Plugins/WxCombat/.../WxAbility_Groggy.cpp:76`의 `HandleMontagePollTick`, `:78`의 `HandleGroggySafetyTimeout`.
- **제안**: 폴링 진입점을 `HandleScanTimer` 등으로 바꾸고, 실제 스캔 본문이 필요하면 그 안에서 `ScanAndPush`를 부른다(뷰모델·`BeginPlay`의 즉시 1회 호출도 같은 함수를 쓸 수 있다).
- **확신도**: 높음

### 6. 🟢 서로 다른 두 Task 가 같은 `DisplayName`("스포너 발동")을 쓴다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Device/WxStateTreeTask_TriggerSpawners.h:30`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.h:34`
- **범주**: 설계/구조
- **문제**: `meta = (DisplayName = "스포너 발동", Category = "Wx")`가 두 USTRUCT에 동일하게 걸려 있다. 노드 픽커에서 같은 이름 항목이 둘 뜨고, 저작자는 어느 쪽이 바인딩형(`TSoftObjectPtr` 배열)이고 어느 쪽이 리터럴 지정형(UOL)인지 이름만으로 구분할 수 없다. 두 헤더의 doc-comment는 서로를 이름으로 지목해 안내하고 있어(`"'Trigger Spawners By Locator'"` / `"'Trigger Spawners'"`) 표시명과 어긋난다.
- **제안**: 한쪽을 `"스포너 발동 (지정)"`처럼 갈라 놓는다. doc-comment가 쓰는 영문 이름과 일치시키면 더 좋다.
- **확신도**: 높음

### 7. 🟢 스포너 셀이 스트리밍 아웃되면 멀리 있는 스폰 대상까지 파괴된다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:134`
- **범주**: 설계/구조
- **문제**: `EndPlay`가 이유를 가리지 않고 권위에서 `SpawnedActor`를 `Destroy()` 한다. `SpawnTarget`은 복제 스무딩 때문에 의도적으로 스폰 대상을 스포너에 attach 하지 않으므로(`:184-187` 주석), 스폰된 적은 플레이어를 따라 다른 셀로 이동할 수 있다. 그 상태에서 스포너가 놓인 WP 셀이 언로드되면 `EEndPlayReason::RemovedFromWorld`로 `EndPlay`가 돌아 전투 중인 적이 사라진다.
- **제안**: `EndPlayReason`으로 갈라 `Destroyed`/`LevelTransition`/`Quit` 등에서만 정리하고, 스트리밍 언로드에서는 소유권을 놓아주거나(또는 대상 액터를 `bIsSpatiallyLoaded=false`로 저작) 리스폰 경로가 다시 붙잡게 한다. 의도된 동작이라면 그 이유를 `EndPlay` 자리에 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDevice.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_SendEvent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_ComponentMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_SplineMove.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayAnimation.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayLevelSequence.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_ApplyGameplayEffectToInteractor.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_PlaySound.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_SpawnNiagara.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_TriggerSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeTask_RespawnSpawners.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_TriggerSpawnersByLocator.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxStateTreeComponentName.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxWorldDeveloperSettings.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawnable.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/WxWorldModule.cpp` 및 대응 Public 헤더 전부
- **미검토 / 한계**:
  - `AWxSpawner`의 `#if WITH_EDITOR` 프리뷰 경로(`PostRegisterAllComponents`·`UpdateEditorPreviewFromSpawnableClass`)는 읽었으나 실제 에디터 동작(T3D 붙여넣기·WP 셀 재열기 시 SaveId·라벨 거동)을 재현 검증하지는 않았다.
  - 장치 StateTree 에셋·BP(`Plugins/WxWorld/Content/`, `Content/Quest/Steps/`)의 내부 저작은 리뷰 범위 밖이라, 「어떤 상태가 상호작용을 켠 채 남는가」 같은 findings 1의 전제는 코드 가능성만 확인했고 실제 에셋에서 발생하는지는 확인하지 않았다.
  - `FWxStateTreeComponentName`의 짝인 에디터 커스터마이제이션(`FWxStateTreeComponentNameCustomization`)은 `Source/WxEditor`에 있어 이번 모듈 범위 밖이다.
  - 멀티플레이 실측(늦은 조인·패킷 유실 하의 `StateTag` 수렴, `Multicast_ReenterState` 타이밍)은 코드 독해로만 판단했다.

---
*문서 기준 커밋 `13b45192` · 리뷰일 2026-08-25 · 소스 50파일 — `/module-review`로 갱신*
