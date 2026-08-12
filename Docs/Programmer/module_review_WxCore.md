# WxCore — 코드 리뷰

> foundation 모듈답게 여전히 깨끗하다 — 🔴 결함은 없고, 실행 코드라 부를 것은 `WxInteractable.cpp` 의 세 함수(약 30줄)가 전부이며 나머지는 태그 선언·상수·계약 인터페이스다. 소스 9파일(.h/.cpp)을 전부 읽었고, Gameplay Tag 97개는 선언↔정의 집합을 스크립트로 대조(불일치·중복 0)했으며, 계약이 실제로 지켜지는지는 소비처(WxWorld 스캐너·기믹 ST 컴포넌트, WxGame 상호작용 어빌리티·캐릭터·CMC, WxDialogue 대화 컴포넌트, WxCombat 어트리뷰트셋, WxSave 월드 서브시스템) cpp 와 UE 5.8 엔진 소스까지 내려가 교차 확인했다. 직전 리뷰(`f7620119`) 이후 이 모듈의 변경은 태그 3건 추가·2건 삭제뿐이라, 아래 발견 중 4건은 그때와 같은 자리에 그대로 남아 있다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 4 |

## 결과

### 1. 🟡 `IsMeshInRange` 의 `ensure` 는 노리는 오설정에 닿지 못하고, 정작 정상 사용법에서 오발화하며, 진짜 실패 조건은 통과시킨다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:36` (계약 문구는 `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:40-43`)
- **범주**: 버그/정확성
- **문제**: 이 한 줄의 가드가 세 방향으로 어긋나 있다.
  1. **노리는 케이스에 도달하지 못한다.** 헤더 `:43` 은 "전제가 깨지면 진입부 ensure 로 드러낸다"고 약속하지만, `IsMeshInRange` 의 유일한 호출부는 서버 어빌리티 한 곳이고(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:78` — 전수 확인), 그 경로에 들어오려면 클라 스캐너가 이미 후보로 잡은 메시여야 한다. 후보는 `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:166` 의 `OverlapMultiByObjectType` 결과에서만 나오는데, 월드 오버랩은 쿼리 콜리전이 켜진 컴포넌트만 돌려준다. 즉 "디자이너가 콜리전을 안 켠 영역 메시"는 후보 단계에서 조용히 탈락해 이 ensure 에 닿지 않는다.
  2. **발화 가능한 경로가 정당한 off-switch 다.** 헤더 `:42` 는 콜리전 내리기를 상호작용 off-switch 로 안내하고, `UWxDialogueComponent::SetInteractionEnabled` 가 실제로 그 방식을 쓴다(`Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:45`). 그래서 이 컴포넌트는 ensure 를 피하려고 `IsInteractionMeshActive` 에 콜리전 검사를 손수 복제해 넣었고, 주석이 그 이유를 직접 증언한다(`WxDialogueComponent.cpp:22-24`). 같은 off-switch 를 쓰면서 이 방어를 잊은 구현체는 정상 플레이 중 ensure 를 맞는다 — WxCore 의 진단 장치가 소비 도메인에 방어 코드를 강제하는 형태로 새어 나가 있다.
  3. **판정식과 검사식이 다르다.** 실제 실패 조건은 "쿼리 콜리전 off" 가 아니라 "충돌 바디 부재"다. `USkeletalMeshComponent::OverlapComponent` 는 `Bodies` 배열을 순회하므로(엔진 `SkeletalMeshComponentPhysics.cpp:3080`) 피직스 애셋 없는 스켈레탈은 쿼리 콜리전이 켜져 있어도 항상 false 인데, 지금 검사는 이 흔한 오설정을 통과시킨다. 반대로 `CollisionEnabled=PhysicsOnly` 면 `IsQueryCollisionEnabled()` 는 false 라 ensure 가 발화하지만, 지오메트리 오버랩은 셰이프의 쿼리 플래그를 보지 않으므로(엔진 `PhysInterface_Chaos.cpp:1114` `Overlap_GeomInternal` — Complex/Simple 플래그와 `IsShapeBoundToBody` 만 본다) 판정은 정상적으로 true 를 답한다. 메시지의 "항상 false" 서술이 이 케이스에선 사실과 반대다.
- **제안**: ensure 를 영역이 등록·초기화되는 시점(구현체 `BeginPlay`, 기믹의 영역 등록 경로)으로 옮겨 실제 설정 누락을 잡고, 조건을 "쿼리 콜리전 on" 이 아니라 "충돌 바디 존재"(스켈레탈은 `GetPhysicsAsset()`, 그 외는 `GetBodyInstance()` 유효성)로 바꾼다. 옮기지 않는다면 `IsMeshInRange` 의 ensure 를 제거하고, 헤더 `:40-43` 의 약속 문구를 사실에 맞게 정정하면서 "콜리전으로 상호작용을 끄는 구현체는 `IsInteractionMeshActive` 에서도 같은 판정을 해야 한다"를 계약으로 명시한다.
- **확신도**: 높음 (호출부 전수 확인 + 엔진 5.8 소스 대조. `WxDialogueComponent` 의 우회 주석이 문제를 직접 증언한다)

### 2. 🟡 `Find` 가 인자 메시를 보지 않고 소유 액터만으로 구현체를 고른다 — 구현체가 둘이면 답이 피어마다 갈릴 수 있다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:18`, `:24`
- **범주**: 설계/구조
- **문제**: `Find(Mesh)` 는 (a) 소유 액터가 계약을 구현했으면 무조건 그 구현을, (b) 아니면 `FindComponentByInterface` 가 돌려주는 **첫 번째** 컴포넌트를 답한다 — 어느 갈래도 인자 `Mesh` 가 그 구현체의 영역인지 검사하지 않는다. 계약은 "한 액터에 상호작용 영역이 여럿"을 정면으로 지원하지만(`WxInteractable.h:61`·`:77`), 그 영역들이 **한 구현체 소속일 때만** 성립한다는 제약이 코드·주석 어디에도 없다. 결과는 조용한 실패다. 계약 컴포넌트를 둘 붙이면 두 번째 컴포넌트의 영역이 첫 번째에게 질의돼 `IsInteractionMeshActive` 가 false 를 답하고(`Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp:103` 의 `InteractionRegions.Contains`) 스캐너 후보에서 빠진다. C++ 로 계약을 구현한 액터(`Source/WxGame/Character/WxEnemyCharacter.h:56-59`)에 계약 컴포넌트를 붙이면 컴포넌트 구현은 영원히 호출되지 않는다. 더 나쁜 것은 (b) 갈래의 **순서 비결정성**이다 — `AActor::FindComponentByInterface` 는 `GetComponents()` 를 순회해 첫 매치에서 break 하고(엔진 `Actor.cpp:4080-4097`), 그 컨테이너는 `TSet<TObjectPtr<UActorComponent>>` 다(엔진 `Actor.h:4331`). 순회 순서가 보장되지 않으므로 서버와 클라가 서로 다른 구현체를 고를 수 있고, 이는 이 인터페이스가 스스로 내건 "양쪽이 같은 답에 수렴해야 한다"는 계약(`WxInteractable.h:54-55`·`:70-71`)과 정면으로 충돌한다.
- **제안**: 제약을 코드로 드러낸다 — `Find` 가 후보(소유 액터 + 계약 컴포넌트 전부)를 순회하며 `IsInteractionMeshActive(Mesh)` 가 true 인 것을 고르게 하면 호출부 변경 없이 제약과 비결정성이 함께 사라진다. 현 동작을 유지한다면 "액터당 구현체 하나"를 `Find` 주석에 명시하고, 개발 빌드에서 구현체가 둘 이상이면 `ensure` 로 드러낸다.
- **확신도**: 중간 (현재 콘텐츠에는 해당 조합이 없어 증상은 아직 없다. `Plugins/WxDialogue/Source/WxDialogue/Public/WxDialogueComponent.h:19` 가 이 한계를 이미 알려진 것으로 기록해 두었으나, BP 액터에 컴포넌트 둘을 붙이는 조합은 코드 수정 없이 만들어진다)

### 3. 🟢 계약 안에서 같은 "영역 메시"가 두 타입으로 오간다 — 구현체가 매번 캐스트한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:48`·`:57` vs `:35`·`:63`·`:73`·`:79`
- **범주**: 설계/구조
- **문제**: 영역 메시를 물을 땐 `const UPrimitiveComponent*`(`:48`, `:57`), 되받을 땐 `const UActorComponent*`(`:35`, `:63`, `:73`, `:79`)로 타입이 갈린다. 실제 호출부는 예외 없이 `UPrimitiveComponent*` 를 넘기므로(`WxInteractionScannerComponent.cpp:86`·`:184`·`:194`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:65`·`:72`·`:78`·`:85`·`:90`) 넓은 타입에서 얻는 이득이 없고, 메시가 필요한 구현체만 손해를 본다 — `WxGimmickStateTreeComponent.cpp:116`·`:123` 이 `const_cast<UPrimitiveComponent*>(Cast<UPrimitiveComponent>(Source))` 를 두 번 반복한다. 2026-08-07·2026-08-11 리뷰에서도 같은 지적이 있었고 코드는 그대로다.
- **제안**: `Source` 계열 파라미터를 `const UPrimitiveComponent*` 로 좁힌다(호출부 변경 없음, 구현체의 캐스트 제거). 넓은 타입이 "영역이 장차 비-프리미티브 컴포넌트가 될 수 있다"는 의도적 여지라면 그 이유를 주석에 남겨 다음 사람이 좁히려다 되돌리지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 4. 🟢 `State.Dead` 태그 주석이 부여 주체와 인과를 뒤집고, 존재하지 않는 제거 경로를 서술한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:18`
- **범주**: 버그/정확성
- **문제**: 주석은 "HandleDeath 시 ASC에 부여되며, 부활 시 제거"라고 적었지만 실제 순서는 정반대다. 부여는 HP 가 0 이 될 때 `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:170` 이 하고, `HandleDeath` 는 그 태그 변화를 받아 도는 **반응**이다(구독 `Source/WxGame/Character/WxCharacterBase.cpp:72-73`, 콜백 `:207-211`). 사망 어빌리티도 이 태그를 TriggerTag 로 받는다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp:26`). 이 헤더가 프로젝트 사망 계약의 사실상 단일 서술처라(README 가 "시스템 간 계약을 읽는 지도"로 안내한다) 인과가 뒤집힌 서술은 "HandleDeath 를 부르는 쪽이 태그를 준다"는 오해를 그대로 재생산한다. 덧붙여 "부활 시 제거"에 해당하는 `RemoveLooseGameplayTag(WxGameplayTags::State_Dead)` 는 C++ 어디에도 없다(전수 검색 0건).
- **제안**: "HP 가 0 이 되면 `UWxCombatAttributeSet` 이 부여하고, 각 머신이 이 태그를 받아 `HandleDeath`·사망 어빌리티를 돈다"로 정정한다. 부활 경로가 실제로 없다면 그 문구는 지운다. 같은 파일 `:42` 의 `State_Guard` 는 태그 97개 중 유일하게 주석이 없으니 함께 채운다.
- **확신도**: 높음 (부여·반응 경로 전수 확인. "부활 시 제거"만 C++ 경로 0건이라 BP 경로가 있다면 확인 필요)

### 5. 🟢 액터→컴포넌트 구현체 조회 규약이 두 계약에서 서로 다른 자리에 산다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:9-25` vs 중복 구현 `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:88-102` (`Plugins/WxCore/Source/WxCore/Public/WxSavable.h` 에는 대응 함수가 없다)
- **범주**: 중복/복잡도
- **문제**: "액터가 직접 구현했으면 그것, 아니면 `FindComponentByInterface`" 라는 동일 규약이 두 벌 있다 — `IWxInteractable::Find` 는 계약과 함께 WxCore 에 있고, 같은 규약의 `UWxSaveWorldSubsystem::FindSavable` 은 소비 도메인인 WxSave 에 있으며 인터페이스 타입만 다른 사실상 동일 코드다(주석 문구까지 대응된다). 두 계약 모두 WxCore 소유이고 컴포넌트 갈래를 쓰는 이유(호스트 액터를 순수 BP 로 두기)도 같은데 조회 지점만 갈려 있어, 세 번째 계약을 추가할 때 따를 기준이 없다. 발견 2 의 비결정 순회 문제도 두 곳에 각각 복제돼 있어 고칠 때 둘 다 손봐야 한다.
- **제안**: `IWxSavable::Find(AActor*)` 를 WxCore 에 두고 WxSave 가 그것을 호출하도록 통일한다(또는 반대 방향으로). 어느 쪽이든 두 계약이 같은 모양이면 되고, 통일해 두면 발견 2 의 수정도 한 곳에서 끝난다.
- **확신도**: 낮음(의도된 설계일 수 있음 — WxSave 만 액터 단위 순회를 하므로 소비처 보관이 자연스럽다는 반론이 가능하다)

### 6. 🟢 `FWxCoreModule` 은 빈 껍데기인데 README 는 "부트스트랩(Native Tag 등록 등)" 으로 소개한다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp:6-7` vs `Plugins/WxCore/README.md:26`
- **범주**: 설계/구조
- **문제**: `StartupModule`·`ShutdownModule` 은 둘 다 빈 본문이다. Native Tag 는 `UE_DEFINE_GAMEPLAY_TAG` 의 정적 인스턴스가 스스로 등록하므로 모듈이 할 일이 없는 것이 정상인데, README 의 핵심 타입 표는 이 클래스를 "모듈 부트스트랩 (Native Tag 등록 등)" 으로 적어 두어 미래 세션이 등록 코드를 찾다 헤매게 만든다. 반대로 이 자리에서 할 값어치가 있는 일 하나는 비어 있다 — `ECC_WxAttack = ECC_GameTraceChannel1`(`Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h:16`) 이 `Config/DefaultEngine.ini:39` 의 `WxAttack` 등록과 1:1 로 맞는다는 전제는 헤더 주석이 "일치해야 한다"고 요구할 뿐 아무 데서도 검증되지 않는다. ini 에 커스텀 채널이 하나 더 앞서 추가되면 상수가 조용히 다른 채널을 가리키고 프로젝트 전역 피격 판정이 경고 없이 죽는다.
- **제안**: README 표의 설명을 실제(빈 부트스트랩)에 맞게 고친다. 함께, `StartupModule` 에서 `UCollisionProfile::Get()->ReturnChannelNameFromContainerIndex(ECC_WxAttack)` 이 `TEXT("WxAttack")` 인지 `ensureMsgf` 로 대조하면 그 전제가 깨지는 즉시 드러난다.
- **확신도**: 중간 (README 불일치는 사실 확인 완료. 채널 검증은 현재 정합하므로 예방 제안이다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` + `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` + `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`(선언↔정의 스크립트 전수 대조), `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **교차 확인(리뷰 대상 아님, 계약 준수 판정용)**: `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Gimmick/WxGimmickStateTreeComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxCharacterMovementComponent.cpp`, `Source/WxGame/Character/WxEnemyCharacter.h`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp`, `Config/DefaultEngine.ini`, 엔진 5.8 `Actor.h`·`Actor.cpp`·`PrimitiveComponent.cpp`·`SkeletalMeshComponentPhysics.cpp`·`BodyInstance.cpp`·`PhysInterface_Chaos.cpp`·`CollisionProfile.h`
- **이번 리뷰에서 문제없음을 확인한 항목**:
  - 모듈 경계 — `WxCore.Build.cs` 는 엔진 모듈 4개(`Core`/`CoreUObject`/`Engine`/`GameplayTags`)뿐이고 `WxCore.uplugin` 의 플러그인 의존은 0. foundation 규칙 준수.
  - CLAUDE.md 코딩 규칙 — 첫 줄 Copyright 10/10(`.Build.cs` 포함), `Wx` prefix 일관, `BlueprintCallable`·람다·`FORCEINLINE`·인라인 함수 정의 0건, 델리게이트 콜백 자체가 없어 `Handle` prefix 대상이 없다. 위반 0.
  - Gameplay Tag — 선언 97 = 정의 97, 이름 집합 완전 일치, 중복 0. `Config/DefaultGameplayTags.ini` 가 없고 WxCore 밖 Wx 모듈의 `UE_DECLARE/DEFINE_GAMEPLAY_TAG`·`RequestGameplayTag` 도 0건이라 "태그는 여기서만" 규약이 유지된다. 직전 리뷰 이후 삭제된 `Event.HitStop`·`SetByCaller.HitStop` 은 헤더·cpp 양쪽에서 짝지어 빠졌다.
  - 신규 `Movement.InAir` 주석 — "CMC 가 낙하 모드 진입·이탈에 맞춰 각 머신에서 부여/제거" 서술이 `Source/WxGame/Character/WxCharacterMovementComponent.cpp:66` 의 `SetLooseGameplayTagCount(..., IsFalling() ? 1 : 0)` 구현과 일치한다.
  - 콜리전 채널 — `ECC_WxAttack = ECC_GameTraceChannel1` 이 `Config/DefaultEngine.ini:39` 의 유일한 커스텀 채널 등록(`WxAttack`, `DefaultResponse=Block`)과 현재 1:1 로 맞고, 헤더 주석이 서술한 런타임 override(`Source/WxGame/Character/WxCharacterBase.cpp:24`·`:28`)도 코드와 맞다(정합성 검증 부재는 발견 6 참조).
- **미검토 / 한계**: `Gimmick.*`·`Quest.Fail`·`Ability.Pattern.*`·`Ability.Skill.1~4`·`GameplayCue.AttackTelegraph.*` 는 C++ 참조가 0인 전량 데이터 구동 태그라 데드 여부를 확정하지 못했다(에셋 내부는 범위 밖). 발견 1 의 ensure 발화/미발화 시나리오는 호출부 전수 확인과 엔진 소스 정독으로만 판정했고 PIE 실측은 하지 않았다. BP/WBP 내부 구조는 범위 밖.

---
*문서 기준 커밋 `ebe6cffd` · 리뷰일 2026-08-12 · 소스 9파일 — `/module-review`로 갱신*
