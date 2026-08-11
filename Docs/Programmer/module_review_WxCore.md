# WxCore — 코드 리뷰

> foundation 모듈답게 여전히 매우 깨끗하다 — 🔴 결함은 없고, 실행 코드라 할 것은 `WxInteractable.cpp` 의 세 함수(약 30줄)가 전부이며 나머지는 선언·상수·계약이다. 소스 9파일(.h/.cpp)을 전부 통독했고, Gameplay Tag 97개는 선언↔정의 집합 대조(불일치 0)와 변수명↔문자열 치환 규칙 전수 검사(위반 0)를 스크립트로 확인했으며, 계약 준수는 소비처(WxWorld 스캐너·기믹 ST 컴포넌트, WxGame 상호작용 어빌리티·캐릭터, WxDialogue 대화 컴포넌트, WxCombat 어트리뷰트셋, WxSave 월드 서브시스템)의 cpp 까지 내려가 교차 확인했다. 남은 5건은 전부 인터페이스 계약 설계와 주석 정확성에 몰려 있다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 `IsMeshInRange` 의 `ensure` 는 노리는 오설정에 도달하지 못하고, 정작 정당한 사용법에서만 발화한다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:36` (계약 문구는 `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:40-43`)
- **범주**: 버그/정확성
- **문제**: 두 가지가 겹쳐 있다.
  1. **노리는 케이스에 도달할 수 없다.** 헤더는 "쿼리 콜리전이 꺼지면 상호작용이 통째로 사라지며, 개발 빌드는 진입부 ensure 로 드러낸다"고 약속하지만, `IsMeshInRange` 의 유일한 호출부는 서버 어빌리티 한 곳이고(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:78`), 그 경로에 들어오려면 클라 스캐너가 후보로 잡아 선택된 메시여야 한다. 후보는 `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:166` 의 `OverlapMultiByObjectType` 결과에서만 나오는데(`:174-198`), 월드 오버랩은 쿼리 콜리전이 켜진 컴포넌트만 돌려준다. 즉 ensure 가 잡으려던 "디자이너가 콜리전을 안 켠 영역 메시"는 후보 단계에서 탈락해 이 함수에 닿지 않는다. 기믹 영역은 콜리전과 무관하게 `InteractionRegions` 등록만으로 활성이 되므로(`Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:159`·`:103`) 이 오설정은 실제로 발생 가능한데, 증상은 여전히 "프롬프트가 안 뜬다" 한 줄뿐이다.
  2. **발화 가능한 유일한 경로가 정당한 off-switch 다.** 같은 헤더 `:42` 는 "영역 메시를 NoCollision 으로 내리는 것은 그 대상의 상호작용을 통째로 끄는 것과 같다"고 적어 콜리전 내리기를 off-switch 로 안내하고, `UWxDialogueComponent::SetInteractionEnabled` 가 실제로 그 방식을 쓴다(`Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:53`). 그래서 이 컴포넌트는 ensure 를 피하려고 `IsInteractionMeshActive` 에 콜리전 검사를 손수 복제해 넣었고, 주석이 그 이유를 직접 증언한다(`WxDialogueComponent.cpp:22-24`). 같은 off-switch 를 쓰면서 이 방어를 잊은 구현체는 서버 검증 순서상 `IsInteractionMeshActive` 통과 → `IsMeshInRange` 진입으로 정상 플레이 중 ensure 를 맞는다. WxCore 의 진단 장치가 소비처에 방어 코드를 강제하는 형태로 새어 나가 있다.
- **제안**: ensure 를 영역이 등록·초기화되는 시점(구현체 `BeginPlay`, 기믹의 영역 등록 경로)으로 옮기면 실제 설정 누락을 잡는다. 옮기지 않는다면 `IsMeshInRange` 의 ensure 를 제거하고, 헤더 `:40-43` 의 "전제가 깨지면 ensure 로 드러난다"는 문구를 사실에 맞게 정정하면서 "콜리전으로 상호작용을 끄는 구현체는 `IsInteractionMeshActive` 에서도 같은 판정을 해야 한다"를 계약으로 명시한다.
- **확신도**: 높음 (`IsMeshInRange` 호출부 전수 확인 — `WxAbility_Interact.cpp:78` 하나뿐이고, `WxDialogueComponent` 의 우회 주석이 문제를 직접 증언한다)

### 2. 🟡 `Find` 가 인자 메시를 보지 않고 소유 액터만으로 구현체를 고른다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:18`, `:24`
- **범주**: 설계/구조
- **문제**: `Find(Mesh)` 는 (a) 소유 액터가 계약을 구현했으면 무조건 그 구현을, (b) 아니면 `FindComponentByInterface` 가 돌려주는 **첫 번째** 컴포넌트를 답한다 — 어느 갈래도 인자 `Mesh` 가 그 구현체의 영역인지는 검사하지 않는다. 계약은 "한 액터에 상호작용 영역이 여럿"을 정면으로 지원하는데(`WxInteractable.h:61`·`:77`), 그 여러 영역이 **한 구현체 소속일 때만** 성립한다는 제약이 코드·주석 어디에도 없다. 결과로 두 가지 조용한 실패가 생긴다. 한 액터에 계약 컴포넌트를 둘 붙이면 두 번째 컴포넌트의 영역이 첫 번째에게 질의돼 `IsInteractionMeshActive` 가 false 를 답하고(`WxGimmickStateTreeComponent.cpp:103` 의 `InteractionRegions.Contains`) 스캐너 후보에서 빠진다(`WxInteractionScannerComponent.cpp:184-198`). 마찬가지로 C++ 로 계약을 구현한 액터(`Source/WxGame/Character/WxEnemyCharacter.h:25`)에 계약 컴포넌트(`UWxDialogueComponent`)를 붙이면 컴포넌트 구현은 영원히 호출되지 않는다. 실패가 닫히는 방향이라 보안 구멍은 아니지만, 경고·ensure 한 줄 없이 "그 영역만 상호작용이 안 되는" 증상으로만 드러나 원인 추적이 어렵다.
- **제안**: 제약을 코드로 드러낸다 — `Find` 가 후보(소유 액터 + 계약 컴포넌트 전부)를 순회하며 `IsInteractionMeshActive(Mesh)` 가 true 인 것을 고르게 하면 호출부 변경 없이 제약 자체가 사라진다. 유지한다면 "액터당 구현체 하나"를 `Find` 주석에 명시하고 개발 빌드에서 둘 이상이면 `ensure` 로 드러낸다.
- **확신도**: 중간 (현재 콘텐츠에는 해당 조합이 없어 실제 증상은 없다 — 의도된 단순화일 수 있으나 근거가 코드에 없다)

### 3. 🟢 계약 안에서 같은 "영역 메시"가 두 타입으로 오간다 — 구현체가 매번 캐스트한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:48`·`:57` vs `:35`·`:63`·`:73`·`:79`
- **범주**: 설계/구조
- **문제**: 영역 메시를 물을 땐 `const UPrimitiveComponent*`(`:48`, `:57`), 되받을 땐 `const UActorComponent*`(`:35`, `:63`, `:73`, `:79`)로 타입이 갈린다. 실제 호출부는 예외 없이 `UPrimitiveComponent*` 를 넘기므로(`WxInteractionScannerComponent.cpp:86`·`:184`·`:194`, `WxAbility_Interact.cpp:65`·`:72`·`:78`·`:85`·`:90`) 넓은 타입에서 얻는 이득이 없고, 메시가 필요한 구현체만 손해를 본다 — `WxGimmickStateTreeComponent.cpp:116`·`:123` 이 `const_cast<UPrimitiveComponent*>(Cast<UPrimitiveComponent>(Source))` 를 두 번 반복한다. 직전 리뷰(2026-08-07)에서도 같은 지적이 있었고 코드는 그대로다.
- **제안**: `Source` 계열 파라미터를 `const UPrimitiveComponent*` 로 좁힌다(호출부 변경 없음, 구현체의 캐스트 제거). 넓은 타입이 "영역이 장차 비-프리미티브 컴포넌트가 될 수 있다"는 의도적 여지라면 그 이유를 주석에 남겨 다음 사람이 좁히려다 되돌리지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 4. 🟢 액터→컴포넌트 구현체 조회 규약이 두 계약에서 서로 다른 자리에 산다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxSavable.h:27`(대응 함수 없음) vs `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:9-25`, 중복 구현은 `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:88-102`
- **범주**: 중복/복잡도
- **문제**: "액터가 직접 구현했으면 그것, 아니면 `FindComponentByInterface`" 라는 동일한 조회 규약이 두 벌 있다 — `IWxInteractable::Find` 는 계약과 함께 WxCore 에 있고, 같은 규약의 `UWxSaveWorldSubsystem::FindSavable` 은 소비 도메인인 WxSave 에 있으며 인터페이스 타입만 다른 사실상 동일 코드다(주석 문구까지 대응된다). 두 계약 모두 WxCore 소유이고 컴포넌트 갈래를 쓰는 이유(호스트 액터를 순수 BP 로 두기)도 같은데 조회 지점만 갈려 있어, 세 번째 계약을 추가할 때 따를 기준이 없다. 발견 2 의 "액터당 구현체 하나" 제약도 두 곳에 각각 복제돼 있다.
- **제안**: `IWxSavable::Find(AActor*)` 를 WxCore 에 두고 WxSave 가 그것을 호출하도록 통일한다(또는 반대 방향으로). 어느 쪽이든 두 계약이 같은 모양이면 되고, 통일해 두면 발견 2 의 수정도 한 곳에서 끝난다.
- **확신도**: 낮음(의도된 설계일 수 있음 — WxSave 만 액터 단위 순회를 하므로 소비처 보관이 자연스럽다는 반론이 가능하다)

### 5. 🟢 `State.Dead` 태그 주석이 부여 주체와 인과를 뒤집어 서술한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:18`
- **범주**: 버그/정확성
- **문제**: 주석은 "HandleDeath 시 ASC 에 부여되며, 부활 시 제거"라고 적었지만 실제 순서는 정반대다. 부여는 `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:168` 이 HP 가 0 이 될 때 하고, `HandleDeath` 는 그 태그 변화를 받아 도는 **반응**이다(등록 `Source/WxGame/Character/WxCharacterBase.cpp:72-73`, 호출 `:207-211`). 사망 어빌리티도 이 태그를 TriggerTag 로 받는다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp:26`). 이 헤더가 프로젝트 사망 계약의 사실상 단일 서술처라(README 도 "흐름의 어휘가 전부 여기 주석에 있다"고 안내) 인과가 뒤집힌 서술은 "HandleDeath 를 부르는 쪽이 태그를 준다"는 오해를 그대로 재생산한다. 덧붙여 "부활 시 제거"에 해당하는 `RemoveLooseGameplayTag(WxGameplayTags::State_Dead)` 는 C++ 어디에도 없다.
- **제안**: "HP 가 0 이 되면 `UWxCombatAttributeSet` 이 부여하고, 각 머신이 이 태그를 받아 `HandleDeath`·사망 어빌리티를 돈다"로 정정한다. 부활 경로가 실제로 없다면 그 문구도 지운다. 같은 파일 `:42` 의 `State_Guard` 는 97개 태그 중 유일하게 주석이 없으니 함께 채운다.
- **확신도**: 높음 (부여·반응 경로 전수 확인. "부활 시 제거"만 C++ 경로 0건이라 BP 경로가 있다면 확인 필요)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` + `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` + `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`(선언↔정의·치환 규칙 스크립트 전수 대조), `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **교차 확인(리뷰 대상 아님, 계약 준수·데드 코드 판정용)**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Config/DefaultEngine.ini`, 엔진 `PrimitiveComponent.cpp`·`SkeletalMeshComponentPhysics.cpp`(`OverlapComponent` 오버라이드 주장 검증)
- **이번 리뷰에서 문제없음을 확인한 항목**:
  - 모듈 경계 — `WxCore.Build.cs` 는 엔진 모듈 4개(`Core`/`CoreUObject`/`Engine`/`GameplayTags`)뿐이고 `WxCore.uplugin` 의 Plugins 의존은 0. foundation 규칙 준수.
  - CLAUDE.md 코딩 규칙 — 첫 줄 Copyright 10/10(Build.cs 포함), `Wx` prefix 일관, `BlueprintCallable`·람다·`FORCEINLINE`·인라인 함수 정의 0건, 델리게이트 콜백 자체가 없어 `Handle` prefix 대상 없음.
  - Gameplay Tag — 선언 97 = 정의 97, 이름 집합 완전 일치, 변수명↔문자열 치환 규칙 위반 0. 프로젝트에 `Config/DefaultGameplayTags.ini` 가 없고 WxCore 밖 Wx 모듈의 `UE_DECLARE/DEFINE_GAMEPLAY_TAG` 도 0건이라 선언 독점이 유지된다.
  - 콜리전 채널 — 헤더의 `ECC_WxAttack = ECC_GameTraceChannel1` 이 `Config/DefaultEngine.ini:39` 의 유일한 커스텀 채널 등록(`WxAttack`, `DefaultResponse=Block`)과 1:1 로 일치하고, 주석이 서술한 런타임 override(`WxCharacterBase.cpp:24`·`:28`)와 투사체 프리셋 사용(`WxProjectileBase.cpp:23`)도 실제 코드와 맞다.
  - `IsMeshInRange` 주석의 엔진 동작 주장 — "스켈레탈은 오버라이드가 피직스 애셋의 모든 바디를 훑는다"는 서술은 UE 5.8 `USkeletalMeshComponent::OverlapComponent`(`SkeletalMeshComponentPhysics.cpp:3080`)와 일치한다.
  - 태그 데드 코드 — 97개 모두 C++ 또는 콘텐츠에서 참조가 잡혀 명백한 데드 태그는 없다(에셋 쪽 근거의 한계는 아래 참조).
- **미검토 / 한계**: `Gimmick.*`(9개)·`Quest.Fail`·`Ability.Pattern.*`·`Ability.Skill.1~4`·`GameplayCue.AttackTelegraph.*` 는 C++ 참조가 0인 전량 데이터 구동 태그라, 사용 여부를 `.uasset` 바이너리 문자열 매칭으로만 확인했다 — 부분 문자열이 걸릴 수 있어 "미사용이 아니다"의 약한 근거로만 썼고 어느 에셋이 실제로 쓰는지는 확정하지 못했다. 발견 1 의 ensure 발화/미발화 시나리오는 정적 분석과 소비처 주석 근거로만 확인했고 PIE 실측은 하지 않았다. BP/WBP 내부 구조는 범위 밖.

---
*문서 기준 커밋 `f7620119` · 리뷰일 2026-08-11 · 소스 9파일 — `/module-review`로 갱신*
