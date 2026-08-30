# WxWorld — 코드 리뷰

> 장치 상태의 서버 권위·복제 수렴 패턴과 상호작용 로컬리티 경계는 여전히 이 모듈에서 가장 잘 다듬어진 부분이고, 지난 리뷰의 입력 차단 대상 오판은 수정되었다. 남은 결함은 통보 기반 대기와 클라 표시 계층에 몰려 있다. 이번 리뷰는 README·Build.cs·uplugin·Public/Private 헤더 전부를 훑고, 장치 상태머신·상호작용 스캐너·스포너 수명·대기 등록부와 StateTree 태스크 15종의 구현까지 내려가 봤다(연계 확인을 위해 WxGame 의 `WxAbility_Interact`·`WxViewModel_InteractionList`, WxSave 의 복원 경로도 함께 읽었다).

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 3 |
| 🟢 사소 | 1 |

## 결과

### 1. 🔴 이미 처치된 스포너가 스트리밍 인 되면 처치 대기 태스크가 영구 대기한다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp:76`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:102`
- **범주**: 버그/정확성
- **문제**: 이 태스크는 진입 시 1회 평가(`AreAllSpawnersKilled`) 뒤 `MarkKilled()` 통보에서만 재평가한다. 그런데 `bIsKilled` 는 세이브 슬롯으로도 참이 된다 — `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:481` 의 `HandleLevelAddedToWorld` 가 스트리밍 인 한 셀의 액터를 역직렬화하고 `OnSaveRestored()` 를 부른다. `AWxSpawner::OnSaveRestored` 는 남은 인스턴스를 정리할 뿐 `NotifySpawnerKilled` 를 부르지 않으므로, 진입 시점에 언로드였던 스포너가 뒤늦게 `bIsKilled=true` 로 로드되는 경로에서는 완료 통보가 영영 오지 않는다. 헤더가 "진입 시점에 스포너가 언로드여도 된다"고 명시한 조립(레벨 밖 퀘스트 ST 가 원거리 셀의 스포너를 지목)과 세이브 재시작이 겹치면 퀘스트 스텝이 그대로 멈춘다.
- **제안**: `AWxSpawner::OnSaveRestored`(그리고 필요하면 `BeginPlay` 의 `bIsKilled` 조기 반환 지점)에서도 `FWxStateTreeTask_WaitSpawnersKilled::NotifySpawnerKilled(this)` 를 부른다. 통보는 술어 재평가일 뿐이라 멱등하며, 등록이 없으면 무동작이다.
- **확신도**: 높음

### 2. 🟡 하이라이트 해제가 숨겨진 프리미티브를 건너뛰어 외곽선이 남는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:287`
- **범주**: 버그/정확성
- **문제**: `SetActorHighlighted()` 는 켜기·끄기를 가리지 않고 `IsVisible()` 이 거짓인 프리미티브를 건너뛴다. 선택된 상태에서 메시가 숨겨지면(장치 연출로 몸통 교체, LOD 토글, `SetVisibility(false)` 로 감추는 픽업 등) `SetRenderCustomDepth(false)` 가 적용되지 않고, 나중에 다시 보일 때 선택되지도 않은 메시가 이전 stencil 외곽선을 그대로 들고 나타난다. 가시성 필터의 근거는 주석대로 "켜 봐야 외곽선에 기여하지 않는다"이므로 끄는 쪽에는 필요가 없다.
- **제안**: 필터를 `bHighlighted` 인 경우에만 적용한다(끌 때는 모든 `UPrimitiveComponent` 에 `SetRenderCustomDepth(false)`).
- **확신도**: 높음

### 3. 🟡 링크된 서브트리의 상태 Tag 는 발행·저장되지만 추종·복원할 수 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp:215`, 같은 파일 `:245`, `:267`
- **범주**: 설계/구조
- **문제**: `GetActiveStateTag()` 는 `Context.GetActiveFrames()` 를 마지막 프레임부터 훑으므로 링크된(Linked Asset) 서브트리의 상태 Tag 까지 잡아 `StateTag` 로 발행하고 세이브에도 담는다. 반면 `HasState()`·`RequestState()` 는 `StateTreeRef.GetStateTree()` 즉 루트 에셋에서만 핸들을 찾는다. 그래서 링크 서브트리에 Tag 를 단 장치는 (a) 클라 추종에서 `RequestState` 가 조용히 실패해 매 틱 어긋난 채로 남고, (b) 권위 복원에서는 `FollowStateTag` 의 `!HasState` 경로가 "복원 상태를 에셋에서 찾지 못했다"로 복원을 포기한다 — 세이브에 담긴 그 상태로 되돌아갈 수 없다. 발행 쪽만 프레임 전체를 보고 소비 쪽은 루트만 보는 비대칭이 원인이다.
- **제안**: 발행 쪽을 루트 프레임으로 좁혀 계약을 "루트 에셋의 Tag 만 상태 키"로 못 박거나, 반대로 `HasState`/`RequestState` 를 활성 프레임 기준으로 확장한다. 전자를 택하면 링크 서브트리에 Tag 를 달았을 때 컴파일/런타임 경고를 남겨 저작 실수를 드러내는 편이 좋다.
- **확신도**: 중간(링크 에셋을 쓰지 않는 것이 전제인 의도된 범위일 수 있다)

### 4. 🟡 상호작용자가 재생 도중 사라지면 몽타주 태스크가 실패로 빠져 장치를 멈출 수 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp:45`
- **범주**: 버그/정확성
- **문제**: 진입 시 당사자·몽타주가 없으면 `Succeeded` 인데(같은 파일 `:29`), 재생 중 `InteractingCharacter` 가 없어지면 `Failed` 를 반환한다. 사망·언포제스·파괴는 정상 수명 경로이며, 헤더 `Public/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.h:29` 는 "폴링은 대상이 사라진 것까지 종료로 본다"고 계약을 적어 두고 있어 구현이 그 계약과 어긋난다. 실패 전이를 저작하지 않은 상태에서는 트리가 실패로 종료되고, `AWxDevice::OnInteracted`(`Private/Device/WxDevice.cpp:66`)가 멈춘 트리를 걸러내므로 그 장치는 그 세션 동안 다시 눌리지 않는다.
- **제안**: 당사자 소실을 진입 경로와 같이 `Succeeded` 로 처리한다(필요하면 Verbose 로그만 남긴다).
- **확신도**: 높음

### 5. 🟢 `GetPrompts()` 가 죽은 약참조를 건너뛰어 선택 인덱스와 어긋난다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:79`
- **범주**: 버그/정확성
- **문제**: 주석은 "인덱스 정합을 위해 대상이 없으면 빈 텍스트로 자리를 채운다"라고 하지만, 실제로는 `Weak.Get()` 이 널이면 항목 자체를 추가하지 않는다(빈 텍스트 폴백은 `IWxInteractable` 캐스트 실패에만 걸린다). 스캔 사이(≤`ScanInterval`)에 대상이 파괴된 상태에서 `UWxViewModel_InteractionList::Initialize`(`Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:43`)가 초기 시드로 `GetPrompts()`/`GetSelectedIndex()` 를 함께 읽으면 배열이 한 칸 당겨져 선택과 문구가 어긋난다. 다음 스캔이 덮으므로 증상은 짧지만, 코드와 주석이 어긋난 채 남아 있다.
- **제안**: 널 약참조에도 `FText::GetEmpty()` 를 넣어 주석대로 자리를 보존한다(한 줄).
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Device/WxDeviceStateTreeComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxStateTreeTask_WaitSpawnersKilled.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_PlayInteractorMontage.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/StateTreeTask/WxStateTreeTask_EnablePlayerInput.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_WaitForInteraction.cpp`
- **훑은 파일**: `Plugins/WxWorld/README.md`, `Plugins/WxWorld/WxWorld.uplugin`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/Source/WxWorld/Public/` 전체 헤더, `Private/StateTreeTask/` 의 나머지 태스크(ComponentMove·SplineMove·PlayAnimation·PlayLevelSequence·PlaySound·SpawnNiagara·SendEvent·ApplyGameplayEffectToInteractor·RespawnSpawners)·`Private/Spawnable/WxStateTreeTask_TriggerSpawners.cpp`·`Private/System/`·`Private/Device/WxDeviceComponentName.cpp`, 연계 확인용으로 `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`
- **규칙 점검(위반 없음)**: 51개 소스 전부 `// Copyright Woogle. All Rights Reserved.` 로 시작하고, `FORCEINLINE`·헤더 인라인 정의는 `GetInstanceDataType()` 15곳과 템플릿 등록부 1곳뿐이며 모두 규칙 6 예외 사유 주석이 붙어 있다. `Build.cs`·`uplugin` 의 Wx 의존은 `WxCore` 하나이고, `BlueprintCallable` 은 `UWxSpawnerLibrary`(BP Function Library) 한 곳뿐이다. 람다 2곳(정렬 술어·등록부 술어)은 대체 수단이 없는 자리다.
- **미검토 / 한계**: StateTree 에셋의 실제 태그·전이 조립과 BP 이벤트 그래프는 범위 밖이다. 네트워크 수렴·월드 파티션 스트리밍·세이브 복원 순서는 코드 독해로만 검토했고 다중 클라이언트 PIE 로 재현하지는 않았다(샌드박스에 엔진 없음). 에디터 프리뷰(`PostRegisterAllComponents`)와 `PreSave` 의 SaveId 확정 경로는 에디터 실행 검증 없이 읽기만 했다. 3번 발견은 링크 에셋을 실제로 쓰는 장치가 있는지 확인하지 못했다.

---
*문서 기준 커밋 `b47e709` · 리뷰일 2026-08-30 · 소스 51파일 — `/module-review`로 갱신*
