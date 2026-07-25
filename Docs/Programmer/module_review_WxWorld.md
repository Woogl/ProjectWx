# WxWorld — 코드 리뷰

> 전반적으로 건강한 모듈이다. 서버 권위 State 커밋 → 복제 → StateTree 이벤트 진입 패턴이 기믹 6종 전부에서 일관되게 지켜지고, 상호작용 스캐너 리네임(커밋 `b5b6d9e8`)은 결합·데드 코드를 남기지 않고 깔끔히 끝났다. 이번 리뷰는 신설 `WxInteractionScannerComponent`, `WxGimmickStateTreeNodes` 전체(.h/.cpp), `WxGimmick` 베이스와 구현체 6종을 정독했고 `Spawnable/`·`System/` 계열은 훑었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 3 |
| 🟢 사소 | 4 |

## 결과

### 1. 🔴 `MoveInteractorToTarget` 의 입력 차단이 해제되지 않아 컨트롤러가 영구 잠길 수 있다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:428`(차단), `:517`(해제), `:477`(Tick 의 Failed 경로)
- **범주**: 버그/정확성
- **문제**: `EnterState` 는 `Instance.InteractingCharacter` 로 `Controller->SetIgnoreMoveInput(true)`(`:432`)와 `ASC->BlockAbilitiesWithTags`(`:436`)를 건다. 둘 다 스택 카운터라 반드시 짝이 맞아야 하지만, `ExitState`(`:521-523`)는 **그 시점에 다시 읽은** `Instance.InteractingCharacter` 가 유효하고 `IsLocallyControlled()` 일 때만 해제한다. `FStateTreeTaskBase::bShouldCopyBoundPropertiesOnExitState` 는 기본 `true` 이므로 이 인스턴스 데이터는 `ExitState` 직전에 바인딩 소스(기믹의 복제 `InteractingCharacter`)에서 **재복사**된다 — 진입 시점의 스냅샷이 아니다. 따라서
  1. 이동 중 캐릭터가 파괴되면 `Tick`(`:477-480`)이 `Failed` 를 반환하고, 이어지는 `ExitState` 는 null 가드에 걸려 해제를 건너뛴다. `SetIgnoreMoveInput` 은 **폰이 아니라 Controller** 에 걸린 카운터이므로 PC 는 살아남고, 리스폰한 새 폰의 이동 입력이 영구히 무시된다(`ResetIgnoreMoveInput()` 외엔 복구 불가).
  2. 권위 측이 `InteractingCharacter` 를 null 로 갱신하거나 캐릭터가 언포제스되어 `IsLocallyControlled()` 가 false 가 된 경우에도 동일하게 해제가 누락된다.

  `:520` 주석은 "정상 흐름은 짝이 맞는다"고 하지만 비정상 종료 경로가 가드되어 있지 않다.
- **제안**: 차단 여부와 차단 대상을 인스턴스 데이터에 `bInputBlocked` + `TWeakObjectPtr<AController>`/`TWeakObjectPtr<UAbilitySystemComponent>` 로 직접 기록하고, `ExitState` 는 바인딩된 `InteractingCharacter` 가 아니라 그 기록을 근거로 해제한다.
- **확신도**: 높음(결함 자체). 단 현재 이 태스크 경로는 휴면이라(발견 5 참조) 실제 발현은 배선 이후다.

### 2. 🟡 `EnablePlayerInput` 이 모든 피어의 로컬 플레이어 입력을 끄고, 이탈 시 복구 장치가 없다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:37`(헬퍼), `:125`(EnterState)
- **범주**: 설계/구조
- **문제**: `SetLocalPlayerInputEnabled` 는 `GEngine->GetFirstLocalPlayerController(World)`(`:39`)로 **그 머신의 첫 로컬 플레이어**를 잡는다. 기믹 StateTree 는 서버·전 클라이언트가 각자 구동하므로, 플레이어 B 가 컷신 트리거를 상호작용하면 복제 State 를 받은 플레이어 A 의 클라이언트도 Playing 상태에 진입해 A 의 입력을 끈다 — 남의 컷신에 내 조작이 막힌다. 스플릿스크린에서는 2P 이상이 아예 토글 대상에서 빠진다. 또한 이 태스크는 `ExitState` 가 없어 복구를 "다음 상태의 Enable 태스크"에만 의존하므로, 재생 중 기믹이 파괴·스트리밍 아웃되면 입력이 꺼진 채로 남는다.
- **제안**: 연출 대상 플레이어를 명시적으로 지정한다 — 기믹의 `InteractingCharacter` 를 바인딩 입력으로 받아 그 컨트롤러만 토글하고(비대상 피어는 노옵), 짝 해제를 `ExitState` 에도 둔다.
- **확신도**: 중간. 싱글플레이 전용이면 무해하며, "각 상태가 자기 입력 가용 여부를 선언한다"는 현재 규약도 의도된 설계일 수 있다.

### 3. 🟡 `SpawnNiagara` 가 초기/복원 진입을 구분하지 않아 `bPlayOnRestore` 가 죽은 파라미터다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:777-800`, 계약은 `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h:505`, `:513-514`
- **범주**: 버그/정확성
- **문제**: 헤더는 "초기 진입이면 기본적으로 재생하지 않고, `bPlayOnRestore` 면 복원/시작 진입에서도 재생한다"고 명시하지만, `EnterState` 구현에는 `IsInitialOrRestoreEntry` 호출도 `Instance.bPlayOnRestore` 참조도 없다 — 무조건 재생한다. 동형 태스크인 `PlaySound` 는 `:738-742` 에서 정확히 그 게이트를 수행하며, 모듈 전체에서 `bPlayOnRestore` 를 읽는 곳은 `:739` 단 한 곳뿐이다. 결과: 발동용 1회성 FX 가 세이브 복원·레벨 스트리밍 인·레이트조인마다 다시 터지고, 디자이너에게 노출된 `bPlayOnRestore` 체크박스는 아무 효과가 없다.
- **제안**: `PlaySound::EnterState` 와 동일하게 함수 첫머리에 `if (IsInitialOrRestoreEntry(Context, Transition) && !Instance.bPlayOnRestore) { return EStateTreeRunStatus::Succeeded; }` 를 넣는다.
- **확신도**: 높음.

### 4. 🟡 세 태스크가 `bConsideredForCompletion = false` 를 문서화만 하고 실제로 설정하지 않는다
- **위치**: 생성자 `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:76`(`EnableInteraction`), `:119`(`EnablePlayerInput`), `:771`(`SpawnNiagara`) / 계약은 `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h:73`, `:108`, `:516`
- **범주**: 버그/정확성
- **문제**: 세 헤더 주석 모두 "순간 side-effect 라 상태 완료를 구동하지 않는다(`bConsideredForCompletion=false`; 정지 leaf 가 즉시 완료→재선택 루프에 빠지지 않도록)"고 계약을 선언하지만, 세 생성자는 `bShouldCallTick = false` 만 설정한다. `FStateTreeTaskBase` 의 해당 플래그는 기본값이 `true` 이고(UE 5.8 `StateTreeTaskBase.h:34`) 노드를 에셋에 추가하는 순간 그 기본값이 직렬화되므로, 실제로는 세 태스크 모두 완료 판정에 포함된 채 author 된다. 셋 다 `EnterState` 에서 즉시 `Succeeded` 를 반환하므로 이들만 든 leaf 상태는 진입 즉시 완료 → 루트 재선택으로 이어져, 주석이 막으려던 thrash 가 그대로 발생한다(특히 `EnablePlayerInput` 은 재선택마다 `DisableInput`/`EnableInput` 을 재실행한다). 실제로 설정하는 곳은 `PlaySound` 생성자 `:729` 하나뿐이다.
- **제안**: 세 생성자에 `PlaySound` 와 동일하게 `#if WITH_EDITORONLY_DATA` / `bConsideredForCompletion = false;` / `#endif` 를 추가하고, 기존 ST 에셋의 해당 노드 인스턴스에 이미 굳어 있는 `true` 를 점검한다.
- **확신도**: 높음(불일치 사실). 실제 루프 발생 여부는 에셋 author 에 달렸다.

### 5. 🟢 상호작용 이동/몽타주 경로가 여전히 휴면 — 어떤 기믹도 `SetInteractingCharacter` 를 호출하지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h:57`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmick.cpp:60`
- **범주**: 중복/복잡도
- **문제**: 직전 리뷰의 판정이 그대로 유효하다. 저장소 전체에서 `SetInteractingCharacter` 의 호출부는 0 개이며(정의·선언·주석만 존재), Door/Elevator/TreasureChest/AlarmConsole/SpawnConsole/CutsceneTrigger 의 `OnInteracted` 어디에서도 호출하지 않는다. 따라서 복제 프로퍼티 `InteractingCharacter`(`WxGimmick.h:113`)는 항상 null 이고, 이를 바인딩하는 `Move Interactor To Target`·`Play Interactor Montage` 두 태스크는 현재 no-op 다. 발견 1 이 아직 실전 결함이 되지 않는 이유이기도 하다.
- **제안**: 배선 계획이 살아 있으면 그대로 두되 발견 1 을 배선 **전에** 고친다. 계획이 없다면 두 태스크와 `InteractingCharacter` 복제 프로퍼티를 함께 정리한다.
- **확신도**: 높음(사실). 의도된 미배선일 수 있다.

### 6. 🟢 README 가 스캐너 리네임 이후 갱신되지 않아 존재하지 않는 타입·경로를 안내한다
- **위치**: `Plugins/WxWorld/README.md:10`, `:28`, `:39`, `:41`, `:48`
- **범주**: 중복/복잡도
- **문제**: README 는 `UWxInteractionRegistryComponent` 와 `Source/WxWorld/Public/Interaction/WxInteractionRegistryComponent.h`(`:28`, `:48`)를 가리키지만 그 타입·파일은 없다(현재는 `UWxInteractionScannerComponent`). 더 문제는 `:41` 의 "새 상호작용 대상" 절차로, 이미 삭제된 `UWxInteractionComponent` 를 붙이고 `SetCollisionVolume` 을 쓰라고 안내한다 — 실제 규약은 대상 메시의 `ECC_WxInteractable` 응답을 `ECR_Overlap` 으로 두는 것이다(`Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxDoor.cpp:20` 등). 모듈 오리엔테이션 맵이 미래 세션을 잘못된 방향으로 보낸다.
- **제안**: `/readme-writer` 로 재생성한다.
- **확신도**: 높음.

### 7. 🟢 스캐너가 폰 부재 시 조기 리턴해 이전 하이라이트·프롬프트를 정리하지 않는다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:126-131`
- **범주**: 버그/정확성
- **문제**: `ScanAndPush` 는 상호작용 불가 판정 시엔 `UpdateInRange({})` 로 목록·외곽선을 명시 정리하지만(`:137-141`), 폰이 없을 땐 아무것도 하지 않고 반환한다. 그 사이 `InRangeMeshes` 와 `SelectedIndex` 가 그대로 남아 선택 메시의 Custom Depth 외곽선이 켜진 채 유지되고, HUD 리스트도 마지막 상태로 굳는다. 사망 직후 `State.Dead` 게이트가 먼저 목록을 비우는 흐름에서는 문제가 없지만, 태그 없이 폰이 사라지는 경로(폰 교체, 언포제스, 레벨 전환 대기)에서는 잔상이 남는다.
- **제안**: 조기 리턴 전에 `UpdateInRange({})` 를 호출해 두 경로의 정리 동작을 일치시킨다.
- **확신도**: 중간.

### 8. 🟢 WxWorld 만 전용 로그 카테고리가 없어 `LogTemp` 로 경고를 낸다
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:414`, `:847`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:187`, 선언 부재는 `Plugins/WxWorld/Source/WxWorld/Public/WxWorldModule.h:8-13`
- **범주**: 설계/구조
- **문제**: `WxAI`·`WxCombat`·`WxQuest`·`WxSave` 는 모두 모듈 헤더에 `DECLARE_LOG_CATEGORY_EXTERN(LogWx*, Log, All)` 를 두는데 `WxWorld` 만 없다. 결과적으로 기믹·스포너의 진단 경고 3 건이 `LogTemp` 로 섞여 나가 필터링·추적이 어렵다.
- **제안**: `WxWorldModule.h/.cpp` 에 `LogWxWorld` 를 선언·정의하고 세 호출부를 옮긴다.
- **확신도**: 높음.

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmickStateTreeNodes.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmick.cpp`, 기믹 구현체 6종 `.cpp`(`WxDoor`·`WxElevator`·`WxAlarmConsole`·`WxCutsceneTrigger`·`WxSpawnConsole`·`WxTreasureChest`)
- **훑은 파일**: `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawner.h`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/Spawnable/WxSpawnableInterface.h`, `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxSpawnerLibrary.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/System/WxWorldDeveloperSettings.cpp`, `Plugins/WxWorld/Source/WxWorld/Public/WxWorldModule.h`, `Plugins/WxWorld/Source/WxWorld/WxWorld.Build.cs`, `Plugins/WxWorld/README.md`, 기믹 구현체 헤더 6종, 교차 확인용 `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`·`Source/WxGame/Controller/WxPlayerController.cpp`
- **직전 리뷰 지적 재확인**: `BlueprintCallable` 규칙 7 위반은 **해소**됐다 — 위반 대상이던 `UWxInteractionComponent`·`UWxInteractionRegistrySubsystem` 이 삭제됐고, 남은 `BlueprintCallable` 은 `Plugins/WxWorld/Source/WxWorld/Public/System/WxSpawnerLibrary.h:174`(`UBlueprintFunctionLibrary`) 하나뿐이라 규칙에 부합한다. `EnablePlayerInput::GetDescription` 의 bool → `0/1` 포맷도 **해소**됐다(`WxGimmickStateTreeNodes.cpp:141` 이 삼항으로 텍스트를 낸다). 나머지 두 지적(발견 1·2)은 그대로 유효해 다시 실었다.
- **기믹 권위 모델 점검(이상 없음)**: 구현체 6종의 `OnInteracted` 는 전부 `CommitGimmickState` 만 사용하고, `AWxGimmick::CommitGimmickState`(`WxGimmick.cpp:49`)의 `HasAuthority()` 가드 → `OnRep_GimmickState` 직접 호출(RepNotify 관용구) → 클라는 복제 State 의 `OnRep` 으로만 ST 진입하는 흐름이 일관된다. 유일한 직접 `State` 대입은 `AWxCutsceneTrigger::BeginPlay:24`·`OnWxSaveRestored:35` 인데 둘 다 `HasAuthority()` 가드 하에 ST 시작/재시작 직전에만 쓰이므로 규약 위반이 아니다.
- **모듈 규칙 점검**: `WxWorld.Build.cs` 의존은 엔진 모듈 + `WxCore` 뿐 — 타 Wx 플러그인 참조 없음 ✅. 소스 27 파일 전부 `// Copyright Woogle. All Rights Reserved.` 로 시작하며 `Wx` prefix·`Super::` 호출·`Handle` prefix 규칙 위반은 발견하지 못했다.
- **미검토 / 한계**: 발견 4 의 실제 영향(재선택 루프 발생 여부)은 ST 에셋(`ST_Door`·`ST_Elevator`·`ST_CutsceneTrigger` 등)에 직렬화된 노드별 `bConsideredForCompletion` 값에 달려 있는데, `.uasset` 내부 값은 확인하지 않았다. 마찬가지로 발견 3 의 체감 영향도 실제 에셋에서 `SpawnNiagara` 가 어느 상태에 놓였는지에 따라 달라진다. `Spawnable/`·`System/` 계열은 직전 리뷰 이후 무변경이라 훑는 수준으로만 봤다.

---
*문서 기준 커밋 `90aa0e6d` · 리뷰일 2026-07-24 · 소스 27파일 — `/module-review`로 갱신*
