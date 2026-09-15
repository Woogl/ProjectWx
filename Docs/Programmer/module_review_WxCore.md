# WxCore — 코드 리뷰

> 실행 로직은 에디터 전용 표시명 헬퍼(`WxLocatorUtils.cpp`) 하나뿐이고 나머지는 선언이라, 이 foundation 모듈은 건강하다. 결함·규칙 위반은 없다. 공용 계약 문구가 비었거나 코드와 어긋난 곳이 두 군데 있고, 그중 `Effect.Invincible`의 수명 주인 서술은 스킬 컷신 재설계(`9604e2d13`) 뒤 새로 낡은 문구다. 커버리지: 소스 13개 전부와 `*.Build.cs`·`*.uplugin`을 읽었다. 이전 리뷰(`e0106372a`) 이후 변경분(`Event.Ability.ActionPhaseChanged` 개명, `State.Ragdoll` 문구 정정)을 구현과 대조했고, 태그 doc-comment 전반을 저장소의 발행·소비 코드와 교차 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟢 `IWxInteractable::OnInteracted`·`GetInteractionPrompt`에 호출 맥락과 `Interactor`의 정체가 없다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:36`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:38`
- **범주**: 설계/구조
- **문제**: 바로 위 `CanInteract`는 호출 맥락을 밝힌다(`WxInteractable.h:30-33`). 나머지 둘에는 계약 문구가 없고, 실제 규칙은 소비 코드에만 있다.
  - `OnInteracted`의 유일한 호출부는 `ServerOnly` 어빌리티다(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:18`). `CanInteract`(:73)와 사거리 검증(:79)을 통과한 뒤 아바타를 넘긴다(:84). 클라 스캐너도 `CanInteract`에 폰을 넘긴다(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:175`).
  - `GetInteractionPrompt`는 로컬 컨트롤러에서만 도는 스캐너만 부른다(`WxInteractionScannerComponent.cpp:34-38`, :80).

  그래서 구현체마다 계약을 따로 추측한다.
  - `AWxItemPickup`은 `Interactor`가 PlayerController인 분기까지 처리한다(`Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:88-89`). 현재 호출부로는 도달할 수 없는 분기다.
  - `AWxDevice`는 권위와 `CanInteract`를 다시 검사하고 캐릭터로 캐스트한다(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:26`, :37, :43).
  - `UWxDialogueComponent::StartDialogueWith`는 폰만 받는다(`Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:22`).
  - `AWxEnemyCharacter`는 "프롬프트는 로컬 표시"라는 전제를 자기 주석에 다시 적고 `GetPlayerPawn(this, 0)`에 기댄다(`Source/WxGame/Character/WxEnemyCharacter.cpp:136-137`).

  새 구현체가 `OnInteracted`에 UI 열기 같은 클라 로직을 넣으면 스탠드얼론이나 리슨 서버 호스트에서는 정상으로 보이고, 원격 클라에서만 조용히 빠진다. 테스트로 잡기 어려운 형태다.
- **제안**: `OnInteracted`에는 "서버에서, `CanInteract`·사거리 검증을 통과한 뒤에만 호출된다. `Interactor`는 아바타 폰이다"를, `GetInteractionPrompt`에는 "로컬 표시용으로만 호출된다"를 한 줄씩 붙인다. 계약이 정해지면 `AWxItemPickup`의 PlayerController 분기는 걷어낼 수 있다.
- **확신도**: 중간

### 2. 🟢 `Effect.Invincible` 문구의 "컷신 태스크"가 수명 주인을 잘못 가리킨다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:36`
- **범주**: 설계/구조
- **문제**: 문구는 "구간을 연 쪽(노티파이 구간·컷신 태스크·처형의 활성 구간)이 수명을 쥔다"이다. 그런데 커밋 `9604e2d13`(스킬 컷신을 GameState 공용 컴포넌트로 재설계) 이후 컷신 무적의 수명은 `UWxSkillCutsceneComponent`가 쥔다.
  - 부여는 `Start`의 `ApplyGameplayEffectToSelf`가 한다(`Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp:184`). 회수는 `RestoreServerState`(:542)가 하고, 이를 `Finish`(:557)와 `EndPlay`(:577)가 부른다.
  - `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`는 컴포넌트에 `Reserve`·`Start`·`Cancel`을 요청할 뿐이고(:30, :34, :71), 무적 GE를 참조하지 않는다.

  README는 이 태그 헤더를 "시스템 지도"이자 첫 진입점으로 지목한다. 컷신 뒤 무적이 남거나 빠지는 문제를 쫓는 사람이 문구를 믿고 태스크를 열면 GE 핸들을 찾지 못한다. 수명이 어빌리티 태스크가 아니라 컴포넌트 세션 종료(`Finish`)에 묶여 있다는 점도 놓친다.
- **제안**: 괄호 안의 "컷신 태스크"를 "스킬 컷신 컴포넌트"로 바꾼다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**:
  - `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`
    - 스크립트로 확인한 것: 선언 115개와 정의 115개의 식별자 집합·순서 일치, 식별자의 `_`→`.` 변환과 태그 문자열의 일치.
    - 태그별 C++ 사용처를 집계했다. doc-comment가 이름으로 지목한 발행자·소비자(`UWxAbility_Death`, `AWxCharacterBase`, `WxEffect_*`, `WxHitStopComponent`, `UWxAbilityBase::SetActionPhase`, `UWxExecCalc_Damage`, `UWxEffect_Cooldown`, `WxAbility_Interact`, `UWxHUDLayout` 등)를 현재 코드와 대조했다.
    - C++ 참조가 0인 태그는 `Content/`·플러그인 `Content/`·`Config/`에서 이름 문자열이 있는지 확인했다.
    - 개명된 `Event.Ability.ActivationStateChanged`는 에셋·설정에 옛 이름 문자열이 남아 있지 않아 리다이렉트가 필요 없음을 확인했다.
  - `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`
  - `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h`, `Plugins/WxCore/Source/WxCore/Private/Minion/WxMinion.cpp`: 호출부가 BlueprintNativeEvent를 `Execute_`로만 부르는지 확인했다(`WxMinionSubsystem.cpp:45`, `WxBTService_UpdateTargetActor.cpp:78`).
  - `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`
  - `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`: 엔진 `UniversalObjectLocator.h`의 `SyncFind`가 로드 없이 찾기만 하는지 확인했다.
  - `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`: `Config/DefaultEngine.ini:39`, `WxProjectileBase.cpp:28` 프리셋, `Source/WxGame/Character/WxCharacterBase.cpp:26`·:30 오버라이드와 대조했다.
  - `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`: Wx 플러그인 의존이 없음을 확인했다.
- **훑은 파일**:
  - WxCore 자체: `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/README.md`
  - 계약 대조용 소비 코드:
    - 상호작용: `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxNpc.cpp`
    - 전투 태그: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Cutscene/WxSkillCutsceneComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`
    - UI: `Plugins/WxUI/Source/WxUI/Private/Widget/WxHUDLayout.cpp`
    - 소환물: `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp`, `Plugins/WxAI/Source/WxAI/Private/WxAIBehaviorComponent.cpp`
- **미검토 / 한계**:
  - 정적 리뷰라 빌드·PIE는 실행하지 않았다.
  - BP·에셋 내부는 범위 밖이다. 에셋 쪽 근거는 이름 테이블에 문자열이 있는지까지만 봤다. 가드 어빌리티처럼 BP에만 있는 발행자(`Effect.GuardReduction`)는 문구를 검증하지 못했다.
  - 발견으로 싣지 않은 판단:
    - 예약 슬롯 태그 `Ability.Skill.3`·`Ability.Skill.4`와 `Ability.Pattern.5`~`Ability.Pattern.9`는 C++·`Content/` 어디에서도 참조되지 않는다. 슬롯 확장용 사전 정의로 보고 결함으로 치지 않았다.
    - `UI.Layer.GameMenu`는 레이어 등록(`Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp:15`) 외에 이 레이어로 푸시하는 곳이 없다. 레이어 사용은 WxUI 소관이라 제외했다.
    - `Source/WxGame/Character/WxNpc.cpp:40-44`의 `CanInteract`는 권위 측에만 있는 등록부로 답해서 `WxInteractable.h:30-33` 계약에서 벗어난다. 확정된 설계의 구현체 쪽 사항이라 WxGame 리뷰 몫으로 남겼다.
    - `IWxMinion::GetMaxCountPerMaster`는 새로 소환하는 클래스의 값을 주인의 모든 종류 소환물에 적용한다(`Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp:44-54`). 종류가 섞이면 소환 순서에 따라 결과가 달라지지만, 현재 구현체가 `BP_Minion` 하나뿐이라 제외했다.

---
*문서 기준 커밋 `4096004a4` · 리뷰일 2026-09-16 · 소스 13파일 — `/module-review`로 갱신*
