# WxCore — 코드 리뷰

> 실행 로직이 에디터 전용 표시명 헬퍼(`WxLocatorUtils.cpp`) 하나뿐인 선언 위주 foundation이라 여전히 건강하다. 결함·규칙 위반은 없고, 공용 계약 문구의 빈칸 두 곳(그중 하나는 이번에 추가된 `Event.Ability.ActivationStateChanged`)만 남았다. 이번 리뷰는 13개 소스 전부와 `*.Build.cs`·`*.uplugin`을 읽었다. 태그 선언·정의 115쌍의 일치를 스크립트로 대조했고, 공용 인터페이스·태그 doc-comment를 저장소 전반(`Plugins/`, `Source/`)의 실제 발행·소비 코드와 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🟢 사소 | 2 |

## 결과

### 1. 🟢 `IWxInteractable::OnInteracted`·`GetInteractionPrompt`에 호출 맥락과 `Interactor`의 정체가 없다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:36`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:38`
- **범주**: 설계/구조
- **문제**: 바로 위 `CanInteract`는 "클라 표시 게이트와 서버 발동 검증이 같은 답을 받는다"(`WxInteractable.h:30-33`)고 호출 맥락을 밝힌다. 반면 `OnInteracted`와 `GetInteractionPrompt`에는 계약 문구가 없다. 실제 규칙은 소비 코드에만 있다.
  - `OnInteracted`의 유일한 호출부는 `ServerOnly` 어빌리티다(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:18`). 이 어빌리티가 `CanInteract`(:73)와 사거리 검증(:79)을 통과시킨 뒤 아바타 폰을 넘겨 부른다(:59, :84).
  - `GetInteractionPrompt`는 로컬 컨트롤러에서만 도는 스캐너(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:34-38`, :80)와 로컬 UI VM(`Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:43`)만 부른다.

  그래서 구현체마다 계약을 따로 추측한다.
  - `AWxItemPickup`은 "서버 권위에서만 호출된다"는 주석만 달고 권위 검사는 하지 않는다. `Interactor`가 폰인 경우와 PlayerController인 경우를 모두 처리하는데(`Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:76`, :88-89), PlayerController 분기는 현재 호출부로는 도달할 수 없다.
  - `AWxDialogueActor`→`UWxDialogueComponent::StartDialogueWith`는 폰만 받는다(`Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:22`).
  - `AWxDevice`는 권위와 `CanInteract`를 다시 검사하고 캐릭터로 캐스트한다(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:26`, :37, :43).
  - `AWxEnemyCharacter`는 "프롬프트는 로컬 표시"라는 전제를 자기 주석에 다시 적고 `GetPlayerPawn(this, 0)`에 기댄다(`Source/WxGame/Character/WxEnemyCharacter.cpp:136-137`).

  새 도메인 액터가 `OnInteracted`에 UI 열기 같은 클라 로직을 넣으면, 스탠드얼론이나 리슨 서버 호스트에서는 정상으로 보이고 원격 클라에서만 조용히 빠진다. 테스트로 잡기 어려운 형태다.
- **제안**: `OnInteracted`에 "서버에서, `CanInteract`·사거리 검증 통과 후에만 호출된다. `Interactor`는 아바타 폰이다"를, `GetInteractionPrompt`에 "로컬 표시용으로만 호출된다"를 한 줄씩 붙인다. 계약이 정해지면 `AWxItemPickup`의 PlayerController 분기는 걷어낼 수 있다.
- **확신도**: 중간

### 2. 🟢 `Event.Ability.ActivationStateChanged`의 문구가 실제 발행 범위보다 넓다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:70-71`
- **범주**: 설계/구조
- **문제**: 이번 커밋(`28fe02f1`)에서 추가된 태그의 계약은 "로컬 발동 조건 변경 알림"이다. 하지만 발행처는 `UWxAbilityBase::SetActionPhase` 한 곳뿐이다(`Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp:67-82`, 태그 사용은 :78이 유일). 즉 실제로 싣는 것은 ActionPhase 전이뿐이다. 발동 가능 여부를 바꾸는 나머지 신호는 이 이벤트에 실리지 않는다.
  - 유일한 구독자 `UWxViewModel_Ability`도 태그 변화(`Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp:31`), 쿨다운 GE 적용(:27-28), 코스트 어트리뷰트(:258, :628-633)를 따로 구독한다.
  - 페이로드에는 어느 어빌리티의 단계가 바뀌었는지도 없다(`WxAbilityBase.cpp:77-79`).

  이름과 문구만 보고 새 소비자(다른 UI·입력 힌트 등)가 "발동 조건 변경" 전부를 이 이벤트 하나로 받으려 하면, 태그·쿨다운·코스트 변화에서 갱신이 빠진다. 이 파일의 다른 이벤트 태그는 대부분 발행자를 적어 두는데, 이 태그에는 그것도 없다.
- **제안**: 문구를 "UWxAbilityBase의 ActionPhase 전이에서만 발행한다. 태그·쿨다운·코스트 변화는 싣지 않는다" 수준으로 좁힌다. 태그 이름은 승인된 결정(`.codex/worklog/2026-09-15-ActionPhase-이벤트-UI-갱신.md`)이라 유지하고 문구만 고친다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**:
  - `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`
    - 선언 115개와 정의 115개의 식별자 일치를 대조했다. 식별자의 `_`→`.` 변환과 태그 문자열의 일치도 확인했다.
    - 태그별 C++ 사용처를 집계하고 주요 doc-comment의 발행자·소비자 서술을 대조했다(Effect·State·Event·Damage·SetByCaller).
  - `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`
  - `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`
  - `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h`, `Plugins/WxCore/Source/WxCore/Private/Minion/WxMinion.cpp`
  - `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`
    - 호출부 6곳이 모두 `#if WITH_EDITOR` 안에 있음을 확인했다.
    - 엔진 헤더(`UniversalObjectLocator.h:101-115`)로 `SyncFind()`가 로드를 유발하지 않음을 확인했다.
  - `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`: `Config/DefaultEngine.ini:39`의 `ECC_GameTraceChannel1`·`ECR_Block`·`WxAttack`과 대조했다.
  - `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`: Wx 플러그인 의존이 없음을 확인했다.
- **훑은 파일**:
  - WxCore 자체: `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/README.md`
  - 계약 대조용 소비 코드:
    - 상호작용: `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxNpc.cpp`
    - 어빌리티 UI 이벤트: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`
    - 소환물: `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`
    - 엔진 대조: `UAbilitySystemComponent::HandleGameplayEvent`(UE 5.8)
- **미검토 / 한계**:
  - 정적 리뷰라 빌드·PIE는 실행하지 않았다.
  - BP·에셋 내부는 범위 밖이다. 다만 C++에서 참조가 0인 태그는 `Content/`에서 문자열 존재 여부만 확인했다.
  - 발견으로 싣지 않은 판단:
    - 예약 슬롯 태그 `Ability.Skill.3`, `Ability.Pattern.5`·`Ability.Pattern.9`는 C++·`Content/` 어디에서도 참조되지 않는다. `Ability.Skill.4`, `Ability.Pattern.6`~`8`은 C++만 확인했고 참조가 없다. 슬롯 확장용 사전 정의라 결함으로 보지 않았다.
    - `Event.Ability.ActivationStateChanged`는 `HandleGameplayEvent`의 부모 태그 순회를 타므로, `Event`·`Event.Ability`를 트리거로 둔 어빌리티가 있으면 함께 발동한다. C++에는 그런 트리거가 없어 제외했다(BP 트리거는 미확인).
    - `Source/WxGame/Character/WxNpc.cpp:40-44`의 `CanInteract`는 권위 측에만 있는 등록부로 답해서 `WxInteractable.h:30-33` 계약에서 벗어난다. 구현체 쪽 문제라 WxGame 리뷰에서 다룰 일이다.

---
*문서 기준 커밋 `7d1d0374` · 리뷰일 2026-09-15 · 소스 13파일 — `/module-review`로 갱신*
