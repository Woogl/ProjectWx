# WxWorld — 코드 리뷰

> 기믹 베이스(`AWxGimmick`)·공용 StateTree 노드·상호작용 컴포넌트/레지스트리·스폰 시스템까지 핵심 위험 지점을 모두 정독했다. 서버 권위 State 커밋과 복원 스냅 패턴이 일관되게 설계되어 전반적으로 건강하며, 규칙 위반 1건과 상호작용 이동 태스크의 입력 잠금 비대칭이 눈에 띈다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 `BlueprintCallable` 을 컴포넌트·서브시스템 함수에 사용 (규칙 7 위반)
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionComponent.h:61`, `:68`, `Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionRegistrySubsystem.h:48`
- **범주**: 규칙 위반
- **문제**: CLAUDE.md 규칙 7 은 "`BlueprintCallable` 지정자는 Blueprint Function Library, Blueprint Async Action 의 팩토리 함수에서만 사용한다"고 못박는다. 그러나 `UWxInteractionComponent::SetInteractionEnabled`, `SetInteractionText`, `UWxInteractionRegistrySubsystem::CycleSelection` 세 곳이 컴포넌트/로컬플레이어 서브시스템 멤버 함수에 `BlueprintCallable` 을 붙였다. (반면 `UWxSpawnerLibrary::TryRespawnAll` 은 `UBlueprintFunctionLibrary` 소속이라 규칙에 부합한다.)
- **제안**: 세 함수는 WBP 입력/디자이너 편의로 BP 노출이 필요해 보이므로, 규칙을 그대로 지키려면 노출 경로를 BP Function Library 래퍼로 옮기거나, 규칙 자체에 "컴포넌트/서브시스템의 BP 입력 진입점" 예외를 명문화하는 편이 낫다. 현 상태는 규칙 텍스트와 명백히 배치된다.
- **확신도**: 높음 (규칙 문구가 명시적)

### 2. 🟡 `MoveInteractorToTarget` 의 입력 차단/해제 비대칭 — 캐릭터 소멸 시 입력 영구 잠금 가능
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:470`(Tick), `:516`(ExitState)
- **범주**: 버그/정확성 (객체 수명주기)
- **문제**: `EnterState` 는 로컬 컨트롤 캐릭터에 `SetIgnoreMoveInput(true)` + `BlockAbilitiesWithTags` 로 입력을 차단한다(둘 다 스택 카운터). 해제는 `ExitState` 에서 `if (Character && Character->IsLocallyControlled())` 가드 하에 수행한다. 그런데 이동 중 `InteractingCharacter`(복제 `TObjectPtr`) 가 소멸하면(예: 연출 이동 중 플레이어 폰 사망) `Tick` 이 `Failed` 를 반환하고, 뒤이어 호출되는 `ExitState` 에서 `Character` 가 이미 null 이라 해제 분기를 통째로 스킵한다. 플레이어 컨트롤러의 `IgnoreMoveInput` 카운터와 PlayerState ASC 의 어빌리티 차단 태그가 +1 로 남아, 다음 폰에서도 이동·어빌리티가 잠긴 채 복구되지 않는다. 반대로 초기/복원 진입에서 `InteractingCharacter` 가 예외적으로 non-null 이면 차단 없이 `ExitState` 가 카운터를 -1 로 내려 언밸런스가 날 수 있다(헤더/구현 주석도 "대개 null" 이라 인정).
- **제안**: 진입 시 차단한 대상(Controller·ASC)을 인스턴스 데이터에 캐시해 `ExitState` 가 `Character` 유효성과 무관하게 그 캐시로 해제하거나, "차단을 실제로 걸었는지" 플래그를 인스턴스에 두고 그 플래그로만 해제하도록 짝을 맞춘다.
- **확신도**: 중간. 소프트락 자체는 심각하나, 현재 이 경로는 휴면 상태다(발견 5 참조) — 실제로 트리거되려면 기믹이 `SetInteractingCharacter` 를 배선하고 이동 중 캐릭터가 소멸해야 한다.

### 3. 🟡 `EnablePlayerInput` 이 모든 피어의 로컬 플레이어 입력을 끈다 — 공유 부작용
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:36`(`SetLocalPlayerInputEnabled`), `:124`(EnterState)
- **범주**: 설계/구조 (멀티플레이 권위/부작용 범위)
- **문제**: State 는 전 피어에 복제되므로 기믹 ST 는 서버·모든 클라에서 실행된다. `EnablePlayerInput(false)` 태스크는 각 머신에서 `GetFirstLocalPlayerController` 의 폰 입력을 끄므로, 플레이어 A 가 촉발한 컷신 상태가 클라 B 머신에서 B 자신의 입력까지 차단한다. 월드 공유 컷신(모두가 함께 관람)이면 의도된 동작이지만, 특정 플레이어 개인 상호작용(문/상자 등)에 이 태스크를 쓰면 무관한 다른 플레이어가 조작 불능이 된다.
- **제안**: 개인 연출용 상태에서는 "촉발 당사자(InteractingCharacter 의 로컬 컨트롤 여부)" 로 게이트하는 변형을 두거나, 이 태스크는 월드 공유 컷신 전용임을 노드 doc-comment 에 명시해 오용을 막는다.
- **확신도**: 낮음 (컷신 전용이라면 의도된 설계일 수 있음)

### 4. 🟢 `EnablePlayerInput::GetDescription` 이 bool 을 0/1 로 포맷
- **위치**: `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeNodes.cpp:140`
- **범주**: 중복/복잡도 (일관성)
- **문제**: `FText::Format(INVTEXT("Enable Player Input ({0})"), InstanceData->bEnable)` 는 bool 이 int 로 승격되어 에디터 노드 설명에 `true/false` 대신 `0/1` 로 표시된다. 같은 파일의 `EnableInteraction::GetDescription`(:112)은 `bEnable ? INVTEXT("true") : INVTEXT("false")` 로 정석 처리하므로 일관성이 어긋난다(에디터 전용, 런타임 영향 없음).
- **제안**: 삼항으로 `true`/`false` FText 를 넘긴다.
- **확신도**: 높음 (표기 문제, 무해)

### 5. 🟢 상호작용 이동/몽타주 경로가 휴면 — 어떤 기믹도 `SetInteractingCharacter` 를 호출하지 않음
- **위치**: `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h:56` (선언), 호출부 부재
- **범주**: 중복/복잡도 (데드 코드 — 배선 대기)
- **문제**: `SetInteractingCharacter` 와 복제 `InteractingCharacter` 프로퍼티, 이를 바인딩하는 `Wx Move Interactor To Target`·`Wx Play Interactor Montage` 태스크가 완비돼 있으나, Door/Elevator/TreasureChest/CutsceneTrigger/AlarmConsole/SpawnConsole 어느 핸들러도 `SetInteractingCharacter(InstigatorActor)` 를 호출하지 않는다. 따라서 `InteractingCharacter` 는 항상 null 로 남아 두 태스크는 현재 no-op 다. worklog(`2026-07-20-기믹-상호작용-이동-몽타주-Task.md`)가 실제 기믹 배선을 "범위 밖" 으로 명시하므로 의도된 미완 배선이다.
- **제안**: 기능 배선 시 발견 2 의 해제 비대칭을 함께 처리한다. 코드 결함은 아니며 추적용 메모.
- **확신도**: 낮음 (의도된 후속 배선)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxWorld/Source/WxWorld/Public/Gimmick/WxGimmick.h`, `Private/Gimmick/WxGimmick.cpp`, `Public/Gimmick/WxGimmickStateTreeNodes.h`, `Private/Gimmick/WxGimmickStateTreeNodes.cpp`, `Public/Interaction/WxInteractionComponent.h`, `Private/Interaction/WxInteractionComponent.cpp`, `Private/Interaction/WxInteractionRegistrySubsystem.cpp`, `Public/Spawnable/WxSpawner.h`, `Private/Spawnable/WxSpawner.cpp`
- **훑은 파일**: 기믹 구현체 6종(`WxDoor`·`WxElevator`·`WxCutsceneTrigger`·`WxTreasureChest`·`WxAlarmConsole`·`WxSpawnConsole` .cpp/.h), `WxInteractionRegistrySubsystem.h`, `WxSpawnableInterface.h`, `WxSpawnerLibrary.h/.cpp`, `WxWorldDeveloperSettings.h/.cpp`, `WxWorldModule.h/.cpp`, `WxWorld.Build.cs`, `README.md`
- **미검토 / 한계**: ST 에셋(`ST_*`)·기믹 BP 의 상태/전이·완료판정(All/Any) 구성은 C++ 밖이라 미확인 — 여러 노드의 "머무는 상태는 완료판정에서 제외" 규약 준수 여부는 에셋에 의존. `WxCore` 외 Wx 참조 없음 확인(Build.cs 는 `WxCore` 만; TreasureChest 의 `WxInventory.WxRewardTableRow` 는 `RowType` 메타 문자열 경로일 뿐 컴파일 의존 아님). Copyright 첫 줄·`Handle` prefix·`Super::` 호출 위반 없음.

---
*문서 기준 커밋 `9661edf` · 리뷰일 2026-07-21 · 소스 29파일 — `/module-review`로 갱신*
