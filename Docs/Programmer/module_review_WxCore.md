# WxCore — 코드 리뷰

> 실행 로직이 에디터 전용 표시명 헬퍼(`WxLocatorUtils.cpp`) 하나뿐인 선언 위주 foundation이라 여전히 건강하다. 결함·규칙 위반은 없고, 공용 계약 문구가 비었거나 실제 코드와 어긋난 세 곳만 남았다. 그중 `State.Ragdoll`의 소비처 서술은 이번에 새로 찾은 낡은 문구다. 커버리지: 13개 소스 전부와 `*.Build.cs`·`*.uplugin`을 읽었다. 이전 리뷰(`7d1d0374`) 이후 바뀐 `Effect.HitStop` 문구를 구현과 대조했고, 태그 선언·정의 115쌍을 스크립트로 대조했으며, 태그 doc-comment를 저장소 전반의 발행·소비 코드와 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟢 `IWxInteractable::OnInteracted`·`GetInteractionPrompt`에 호출 맥락과 `Interactor`의 정체가 없다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:36`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:38`
- **범주**: 설계/구조
- **문제**: 바로 위 `CanInteract`는 "클라 표시 게이트와 서버 발동 검증이 같은 답을 받는다"(`WxInteractable.h:30-33`)고 호출 맥락을 밝힌다. 반면 `OnInteracted`와 `GetInteractionPrompt`에는 계약 문구가 없다. 실제 규칙은 소비 코드에만 있다.
  - `OnInteracted`의 유일한 호출부는 `ServerOnly` 어빌리티다(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:18`). 이 어빌리티가 `CanInteract`(:73)와 사거리 검증(:79)을 통과시킨 뒤 아바타를 넘겨 부른다(:59, :84).
  - `GetInteractionPrompt`는 로컬 컨트롤러에서만 도는 스캐너(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:34-38`, :80)만 부른다. 로컬 UI VM(`Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:43`)도 그 스캐너를 거친다.

  그래서 구현체마다 계약을 따로 추측한다.
  - `AWxItemPickup`은 "서버 권위에서만 호출된다"는 주석만 달고 권위 검사는 하지 않는다. `Interactor`가 폰인 경우와 PlayerController인 경우를 모두 처리하는데(`Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:76`, :88-89), PlayerController 분기는 현재 호출부로는 도달할 수 없다.
  - `UWxDialogueComponent::StartDialogueWith`는 폰만 받는다(`Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:22`).
  - `AWxDevice`는 권위와 `CanInteract`를 다시 검사하고 캐릭터로 캐스트한다(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:26`, :37, :43).
  - `AWxEnemyCharacter`는 "프롬프트는 로컬 표시"라는 전제를 자기 주석에 다시 적고 `GetPlayerPawn(this, 0)`에 기댄다(`Source/WxGame/Character/WxEnemyCharacter.cpp:136-137`).

  새 도메인 액터가 `OnInteracted`에 UI 열기 같은 클라 로직을 넣으면, 스탠드얼론이나 리슨 서버 호스트에서는 정상으로 보이고 원격 클라에서만 조용히 빠진다. 테스트로 잡기 어려운 형태다.
- **제안**: `OnInteracted`에 "서버에서, `CanInteract`·사거리 검증 통과 후에만 호출된다. `Interactor`는 아바타 폰이다"를, `GetInteractionPrompt`에 "로컬 표시용으로만 호출된다"를 한 줄씩 붙인다. 계약이 정해지면 `AWxItemPickup`의 PlayerController 분기는 걷어낼 수 있다.
- **확신도**: 중간

### 2. 🟢 `State.Ragdoll` 문구의 "타게팅 프리셋이 IgnoreTags로 사용한다"가 사실이 아니다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:30`
- **범주**: 설계/구조
- **문제**: C++에서 이 태그를 쓰는 곳은 두 곳뿐이다. 발행은 `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp:116`, 구독은 `Source/WxGame/Character/WxCharacterBase.cpp:58`이다. 타게팅 쪽 제외 조건은 `Ability.Death`다(`Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnPointComponent.cpp:16`).
  - 문구는 `Event.Ragdoll`→`State.Ragdoll` 이관 커밋(`8e145265d`)에서 들어왔다. 같은 날 커밋 `0cdb18ce5`("타게팅 프리셋에서 옛 Ragdoll 태그를 걷어내고 태그 리다이렉트를 제거")가 프리셋의 옛 태그를 새 이름으로 바꾸지 않고 지웠는데, 문구는 그대로 남았다.
  - 지금은 `Content/`·플러그인 `Content/`·`Config/` 어디에도 `State.Ragdoll` 문자열이 없다. 프리셋(`Content/Character/Template/Shared/Targeting/TP_*`)의 이름 테이블에는 `Ability.Death`만 있다.

  문구를 믿는 사람은 두 가지로 잘못 판단할 수 있다. 첫째, 프리셋의 `Ability.Death`를 `State.Ragdoll`과 중복이라 보고 걷어낼 수 있다. 그러면 죽은 적이 파괴될 때까지 공격·가드 프리셋의 타게팅 후보로 남을 수 있다. 둘째, 사망이 아닌 래그돌에 이 태그를 재사용하면서 타게팅에서 자동으로 빠진다고 기대할 수 있다.
- **제안**: 문구에서 ", 타게팅 프리셋이 IgnoreTags로 사용한다"를 지운다.
- **확신도**: 높음

### 3. 🟢 `Event.Ability.ActivationStateChanged`의 문구가 실제 발행 범위보다 넓다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:70-71`
- **범주**: 설계/구조
- **문제**: 이 태그의 계약은 "로컬 발동 조건 변경 알림"이다. 하지만 발행처는 `UWxAbilityBase::SetActionPhase` 한 곳뿐이다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:67-83`, 태그 사용은 :78이 유일). 즉 실제로 싣는 것은 ActionPhase 전이뿐이고, 발동 가능 여부를 바꾸는 나머지 신호는 이 이벤트에 실리지 않는다.
  - 유일한 구독자 `UWxViewModel_Ability`도 태그 변화(`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:31`), 쿨다운 GE 적용(:27-28), 코스트 어트리뷰트(:258, :628-633)를 따로 구독한다.
  - 페이로드에는 어느 어빌리티의 단계가 바뀌었는지도 없다(`WxAbilityBase.cpp:77-79`).

  이름과 문구만 보고 새 소비자(다른 UI·입력 힌트 등)가 "발동 조건 변경" 전부를 이 이벤트 하나로 받으려 하면, 태그·쿨다운·코스트 변화에서 갱신이 빠진다. 이 파일의 다른 이벤트 태그는 대부분 발행자를 적어 두는데, 이 태그에는 그것도 없다.
- **제안**: 문구를 "UWxAbilityBase의 ActionPhase 전이에서만 발행한다. 태그·쿨다운·코스트 변화는 싣지 않는다" 수준으로 좁힌다. 태그 이름은 승인된 결정(`.codex/worklog/2026-09-15-ActionPhase-이벤트-UI-갱신.md`)이라 유지하고 문구만 고친다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**:
  - `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`
    - 선언 115개와 정의 115개의 식별자 집합·순서 일치, 식별자의 `_`→`.` 변환과 태그 문자열의 일치를 스크립트로 확인했다.
    - 변경분인 `Effect.HitStop` 문구를 `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_HitStop.cpp`, 무기·투사체 적용부(`WxWeaponBase.cpp:245-246`, `WxProjectileBase.cpp:182-183`)와 대조했다. "GE 수명은 월드 시간을 따른다"는 엔진 `GameplayEffect.cpp:4482-4485`(월드 `TimerManager` 등록)로 확인했다. 문구는 현재 구현과 일치한다.
    - 태그별 C++ 사용처를 집계하고 doc-comment의 발행자·소비자 서술을 대조했다(State·Effect·Event·Damage·Ability·Cooldown·SetByCaller·UI). C++ 참조가 0이거나 소비처가 에셋에 있다고 적힌 태그는 `Content/` 이름 테이블의 문자열 존재를 확인했다.
  - `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`
  - `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`
  - `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h`, `Plugins/WxCore/Source/WxCore/Private/Minion/WxMinion.cpp`
  - `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`
  - `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`: `Config/DefaultEngine.ini:39`의 `ECC_GameTraceChannel1`·`ECR_Block`·`WxAttack`, 그리고 메시 Overlap·캡슐 Ignore 오버라이드(`Source/WxGame/Character/WxCharacterBase.cpp:26`, :30)와 대조했다.
  - `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`: Wx 플러그인 의존이 없음을 확인했다.
- **훑은 파일**:
  - WxCore 자체: `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/README.md`
  - 계약 대조용 소비 코드:
    - 상호작용: `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxNpc.cpp`
    - 전투 태그: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_SuperArmor.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnPointComponent.cpp`
    - 어빌리티 UI 이벤트: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`
    - 소환물: `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`
- **미검토 / 한계**:
  - 정적 리뷰라 빌드·PIE는 실행하지 않았다.
  - BP·에셋 내부는 범위 밖이다. 에셋 쪽 근거는 이름 테이블의 문자열 존재 여부까지만 봤고, 어느 필드에 물려 있는지는 보지 않았다.
  - 발견으로 싣지 않은 판단:
    - 예약 슬롯 태그 `Ability.Skill.3`·`Ability.Skill.4`, `Ability.Pattern.5`~`Ability.Pattern.9`는 C++·`Content/` 어디에서도 참조되지 않는다. 슬롯 확장용 사전 정의라 결함으로 보지 않았다.
    - `UI.Layer.GameMenu`는 레이어 등록(`Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp:15`) 말고는 C++·`Content/`에서 이 레이어로 푸시하는 곳이 없다. 문구의 "아이템 획득 알림"은 아직 비어 있는 자리로 보인다. 레이어 사용은 WxUI 소관이라 제외했다.
    - `Event.Ability.ActivationStateChanged`는 `HandleGameplayEvent`의 부모 태그 순회를 타므로, `Event`·`Event.Ability`를 트리거로 둔 어빌리티가 있으면 함께 발동한다. C++에는 그런 트리거가 없어 제외했다(BP 트리거는 미확인).
    - `Source/WxGame/Character/WxNpc.cpp:40-44`의 `CanInteract`는 권위 측에만 있는 등록부로 답해서 `WxInteractable.h:30-33` 계약에서 벗어난다. 구현체 쪽 문제라 WxGame 리뷰에서 다룰 일이다.

---
*문서 기준 커밋 `e0106372a` · 리뷰일 2026-09-16 · 소스 13파일 — `/module-review`로 갱신*
