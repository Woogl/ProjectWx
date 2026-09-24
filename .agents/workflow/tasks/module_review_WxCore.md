# WxCore — 코드 리뷰

> WxCore는 순수 정의 원칙을 지키고 있어 건강하다. 엔진 외 의존이 없고, `FDefaultModuleImpl`로 등록하며, 게임플레이 로직이 없다. 태그 선언·정의 116개가 이름과 문자열까지 1:1로 맞고, `AGENTS.md` 규칙 3개도 모두 지킨다. 남은 발견은 낡은 태그 주석 두 곳뿐이다. 소스 11파일을 모두 읽었다. 태그별 사용처는 C++과 Content 문자열로 셌고, 계약 3개는 구현체와 소비자 호출 경로까지 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟢 태그 용도 주석 두 곳이 현재 사용처와 어긋난다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:251`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:180`
- **범주**: 중복/복잡도
- **문제**: 태그를 한 곳에 모으는 구조라 이 헤더 주석이 사용 범위의 기준 문서 역할을 한다. 그런데 두 곳이 실제와 다르다.
  - `UI.Layer.GameMenu` 주석(`:251`)은 용도로 "아이템 획득 알림 등"을 든다. 하지만 이 레이어는 `Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp:15`에서 등록만 된다. C++ 푸시는 Game·Menu·Modal 레이어만 쓰고, Content 에셋에도 `UI.Layer.GameMenu` 문자열이 없다. 아이템 획득 목록 `WBP_AcquiredItemList`는 `WBP_GameLayout` 안에 들어 있어 Game 레이어에서 뜬다. `Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp:96` 주석도 같은 전제를 반복한다.
  - `:180`의 "플레이어 캐릭터 전용" 묶음(`Ability.Attack.*`·`Ability.Skill.*` 등)은 소환물 어빌리티도 쓴다. `Content/Character/Minion/Abilities/Attack_Heavy/GA_Minion_Attack_Heavy.uasset`은 `WxAbility_Attack` 파생이고 `Ability.Attack.Heavy`를 든다. `Content/Character/Minion/Abilities/Skill_2/GA_Minion_Skill_2.uasset`은 `WxAbility_Skill` 파생이고 `Ability.Skill.2`를 든다.
- **제안**: 두 주석을 실제에 맞춘다. Lyra 레이어 구성에 맞춰 GameMenu를 남길 거면 용도 예시를 지우고 사용처가 없는 예약 레이어라고 적는다(WxUIManagerSubsystem 주석도 함께 고친다). 남길 이유가 없으면 태그와 `WxPrimaryGameLayout` 등록을 함께 지운다. `:180`은 "플레이어 측(플레이어·소환물)"처럼 범위를 고친다.
- **확신도**: 중간(Content 참조는 바이너리 문자열 검색으로 확인했다. GameMenu 레이어는 Lyra 대응 의도로 남긴 것일 수 있다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`(선언·정의 1:1 대조, 태그 주석과 실제 부여·구독 경로 대조), `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`(엔진 `FUniversalObjectLocator::SyncFind`가 로드 없는 탐색인지 확인, 호출부 `WITH_EDITOR` 가드 확인), `Plugins/WxCore/Source/WxCore/Public/WxSpawnable.h`, `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`. 계약 대조를 위해 구현체·소비자 쪽도 읽었다. `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`, `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Ability.cpp`와, 태그 주석 대조에 쓴 WxCombat 대미지·가드·피격·처형·소환물 경로다.
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`. 저작권 첫 줄과 헤더 인라인 정의 여부는 전 파일을 확인했다.
- **미검토 / 한계**:
  - BP·StateTree 에셋 내부는 범위 밖이다. Content 참조는 바이너리 문자열 검색으로만 셌다.
  - 빌드, 멀티플레이 실행, 인게임 동작은 검증하지 않았다.
  - 브리프에 적힌 `UWxAbilityComponent`와 `IWxSavable`은 WxCore를 포함해 현재 소스 어디에도 없어 검토 대상이 아니었다. `Intermediate`에 옛 생성물만 남아 있다.
  - 슬롯 태그 `Ability.Skill.4`·`Cooldown.Skill.4`·`Ability.Pattern.4`~`9`는 C++·Content 참조가 0건이다. 다만 2026-09-24 WxCore 정리에서 이미 후보로 남긴 것이라 다시 지적하지 않았다.
  - 범위 밖 참고: `Content/__ExternalActors__/LevelDesign/LevelInstance/SiegeCannonEmplacement01/` 아래 외부 액터 7개가 `Event.Device.Triggered`를 문자열로 물고 있다. 이 태그는 네이티브·ini 어디에도 정의되어 있지 않다.

---
*문서 기준 커밋 `f6b4af9d4` · 리뷰일 2026-09-24 · 소스 11파일 — `/module-review`로 갱신*
