# WxCore — 코드 리뷰

> 실행 로직이라고는 에디터 전용 표시명 헬퍼(`WxLocatorUtils.cpp`)뿐인 선언 위주 foundation이라 여전히 매우 건강하다. 결함과 규칙 위반은 없고, 공용 인터페이스 계약에 빈칸이 하나 남아 있다. 직전 리뷰(`231068b`) 이후 이 모듈의 C++ 변경은 주석 정리(`eda01fd`·`b9e948d`)뿐이었다. 그래서 이번에는 13개 소스를 전부 다시 읽고, 태그 선언·정의 114쌍의 일치, `*.Build.cs`·`*.uplugin` 의존 경계, CLAUDE.md 코딩 규칙 전수를 확인했으며, 정리된 doc-comment가 실제 소비 코드(WxCombat·WxWorld·WxAI·WxUI·WxInventory·WxDialogue·WxGame)와 맞는지 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟢 `IWxInteractable` 계약에 `OnInteracted`·`GetInteractionPrompt`의 호출 맥락과 `Interactor`의 정체가 없다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:36`
- **범주**: 설계/구조
- **문제**: `CanInteract`는 "클라 표시 게이트와 서버 발동 검증이 같은 답을 받는다"(`WxInteractable.h:30-34`)고 호출 맥락을 밝힌다. 그런데 바로 아래 `OnInteracted`(:36)와 `GetInteractionPrompt`(:38)에는 그런 계약이 없다. 실제 호출 규칙은 이렇다. `OnInteracted`는 `ServerOnly` 어빌리티가 `CanInteract`와 사거리 검증을 통과시킨 뒤, 아바타 폰을 넘겨 서버에서만 부른다(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:18,59,73,79,84`). `GetInteractionPrompt`는 로컬 컨트롤러의 스캐너만 부른다(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:35,80`). 이 규칙이 WxCore가 아니라 소비 코드에만 있어서 구현체마다 따로 추측하고 있다.
  - `AWxItemPickup`은 `Interactor`가 폰인지 PlayerController인지 몰라 두 경우를 모두 처리한다(`Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:88-89`). PlayerController 쪽 분기는 현재 호출부로는 도달하지 않는다.
  - `UWxDialogueComponent`는 폰만 받는다(`Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:22`).
  - `AWxDevice`는 캐릭터로 캐스트하고, 권위와 `CanInteract`를 다시 검사한다(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:26,37,43`).
  - `AWxEnemyCharacter`는 "프롬프트는 로컬 표시"라는 전제를 자기 주석에 다시 적어 둔다(`Source/WxGame/Character/WxEnemyCharacter.cpp:136`).

  새 도메인 액터가 `OnInteracted`에 UI 열기 같은 클라 로직을 넣으면 스탠드얼론이나 리슨 서버 호스트 입장에서는 정상으로 보인다. 원격 클라에서만 조용히 빠지므로 테스트로 잡기 어렵다.
- **제안**: `OnInteracted`에는 "서버에서만, `CanInteract`와 사거리 검증을 통과한 뒤 호출된다. `Interactor`는 상호작용한 아바타 폰이다"를, `GetInteractionPrompt`에는 "로컬 표시용으로만 호출된다"를 한 줄씩 붙인다. 계약이 정해지면 `AWxItemPickup`의 PlayerController 분기는 걷어낼 수 있다.
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**:
  - `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`
  - `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h`, `Plugins/WxCore/Source/WxCore/Private/Minion/WxMinion.cpp`
  - `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`
  - `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`
  - `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`
  - `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`
- **훑은 파일**:
  - WxCore 자체: `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
  - 계약 대조용 소비 코드: `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Source/WxEditor/WxActorLocatorCustomization.cpp`, `Config/DefaultEngine.ini`
- **미검토 / 한계**:
  - 정적 리뷰라 빌드와 PIE는 실행하지 않았다.
  - 엔진 소스가 없어 `FUniversalObjectLocator::SyncFind()`의 내부 비용은 다시 확인하지 못했다. 다만 `FWxLocatorUtils`는 선언 자체가 `WITH_EDITOR` 안에 있어서(`WxLocatorUtils.h:12-18`) 런타임 경로에서는 호출될 수 없다.
  - BP 내부는 범위 밖이다. 예를 들어 유일한 소환물 에셋 `BP_Minion`이 `GetMaxCountPerMaster`(기본 1)를 오버라이드하는지, 에셋 안에서 태그를 어떻게 쓰는지는 보지 않았다.
  - 발견으로 싣지 않은 판단:
    - 직전 리뷰가 지적한 README의 `Movement.*` 누락은 `Plugins/WxCore/README.md:27-33`에 그대로 있다. 코드가 아니라 문서 문제라 제외했다.
    - `Source/WxGame/Character/WxNpc.cpp:40-44`의 `CanInteract`는 권위 측에만 있는 등록부로 답한다. "클라 표시와 서버 검증이 같은 답을 받는다"는 계약에서 벗어나지만, 이는 WxGame 리뷰에서 다룰 일이다.
    - 예약 슬롯 태그(`Ability.Skill.3/4`, `Ability.Pattern.5`~`.9`)를 참조하는 곳이 없는 것은 직전 판정대로 결함이 아니다.

---
*문서 기준 커밋 `9d8cb2dd` · 리뷰일 2026-09-14 · 소스 13파일 — `/module-review`로 갱신*
