# WxCore — 코드 리뷰

> 선언 위주의 foundation이라 실행 결함과 규칙 위반은 없다. 손볼 곳은 소환물 계약 `IWxMinion`에 몰려 있다. C++ 기본값이 BP 구현체에는 닿지 않고, 도플갱어가 추가되면서 한 주인이 두 종류를 소환하게 됐는데 계약은 여전히 종류를 구분하지 않는다. 커버리지: 소스 13개 전부와 `*.Build.cs`·`*.uplugin`을 읽었다. 이전 리뷰(`4096004a4`) 이후 변경분(`Event.CommandMinionAbility` 제거, 주석 정리)은 저장소 전역의 C++·설정·에셋 이름 테이블과 대조했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 `IWxMinion::GetMaxCountPerMaster`의 C++ 기본값 1이 BP 구현체에는 적용되지 않는다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/Minion/WxMinion.cpp:7`, `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h:24-30`
- **범주**: 버그/정확성
- **문제**: 현재 구현체인 `BP_Minion`·`BP_Doppelganger`는 둘 다 BP다(부모는 `AWxEnemyCharacter`). `IWxMinion`을 상속하는 C++ 클래스는 저장소에 없다. 이 구성에서는 `_Implementation`이 호출되지 않는다.
  - BP에 인터페이스를 추가하면 엔진이 반환값이 있는 인터페이스 함수마다 BP 함수 그래프를 만든다(UE 5.8 `Engine/Source/Editor/UnrealEd/Private/Kismet2/BlueprintEditorUtils.cpp:6551`).
  - `Execute_GetMaxCountPerMaster`는 그 BP 함수를 먼저 찾아 실행한다. 네이티브 `_Implementation` 분기는 `GetNativeInterfaceAddress`가 있을 때만 탄다(`Plugins/WxCore/Intermediate/Build/Win64/UnrealEditor/Inc/WxCore/UHT/WxMinion.gen.cpp:50-58`).
  - 그래서 상한을 저작하지 않은 BP는 반환 기본값 0을 낸다(같은 파일 :34의 `ReturnValue(0)`). 계약상 0은 "개수 제한 없음"이다(`WxMinion.h:25`).

  결과적으로 새 소환물 BP가 이 값을 빠뜨리면 `.cpp`가 의도한 "하나만 유지"가 아니라 무제한 소환이 된다. 소비 측은 이 값을 그대로 집행한다(`Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp:53-55`). 같은 인터페이스의 `IsAggroIgnored`는 BP 기본값과 C++ 기본값이 둘 다 `false`라 우연히 맞는다. 헤더 주석도 "미구현 BP는 false"라고 BP를 기준으로 적었다(`WxMinion.h:32`). 두 BP가 현재 상한을 저작했는지는 BP 내부라 확인하지 못했다.
- **제안**: `GetMaxCountPerMaster_Implementation`이 BP 미저작 값과 같은 0을 반환하게 맞추고, 헤더에 "미저작 BP는 0(제한 없음)"을 적는다. 1이 의도라면 BP 구현체가 반환값을 반드시 저작해야 한다고 헤더에 명시한다.
- **확신도**: 높음(메커니즘은 엔진·생성 코드로 확인, 현재 BP 값은 미확인)

### 2. 🟡 소환물 계약이 종류를 구분하지 않는데 한 주인이 역할이 다른 두 종류를 소환한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h:10`, `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h:26`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:22-26`
- **범주**: 설계/구조
- **문제**: 계약 문구는 "소환물 종류가 자기 상한을 선언한다"(`WxMinion.h:10`)이지만 집행은 종류를 가리지 않는다.
  - `SpawnMinion`은 새로 소환하는 클래스의 상한을 주인의 전체 소환물 목록에 적용하고, 가장 오래된 것부터 파괴한다(`Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp:52-62`). 이때 쓰는 `CollectMinions`에는 클래스 필터가 없다(:105-118).
  - `State.Minion.Active`도 종류와 무관하게 켜진다. `FindActiveMinion`을 클래스 없이 호출하기 때문이다(:203).

  이전 리뷰는 구현체가 `BP_Minion` 하나뿐이라 이 문제를 제외했다. 지금은 `BP_HGTest`가 스킬1 몽타주(`AM_HGTest_Skill_1`)로 `BP_Minion`을, 궁극기 몽타주(`AM_HGTest_Ultimate`)로 `BP_Doppelganger`를 소환한다. 근거는 두 몽타주의 이름 테이블과 `.codex/worklog/2026-09-16-doppelganger.md:6`이다. `GA_HGTest_Ultimate`는 `State.Minion.Active`를 참조하지 않으므로 미니언이 살아 있어도 발동된다. 그 결과 두 가지 일이 생긴다.
  - 미니언을 소환한 뒤 궁극기를 쓰면, `BP_Doppelganger`의 상한이 양수일 때 명령용 미니언이 가장 오래된 소환물로 파괴된다(상한 기본값 문제는 1번 참고).
  - 도플갱어만 살아 있어도 `State.Minion.Active`가 켜진다. 그러면 스킬1 소환이 막히고(작업 기록 `.codex/worklog/2026-09-16-doppelganger.md:26`에 관측됨), 같은 입력을 쓰는 스킬은 명령받을 미니언이 없는데도 "소환물 있음" 쪽으로 갈린다.
- **제안**: 로스터를 종류별로 볼지 먼저 정한다.
  - 종류별이 맞다면 `SpawnMinion`의 정리 대상을 `IsA(MinionClass)`로 좁히고, 도플갱어가 `State.Minion.Active`에 기여할지도 함께 정한다.
  - 도플갱어가 미니언을 대체하는 전체 로스터가 의도라면 `WxMinion.h:10`과 `State.Minion.Active` 주석에 "종류 무관"을 명시한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

### 3. 🟢 `IWxInteractable::OnInteracted`·`GetInteractionPrompt`에 호출 맥락과 `Interactor`의 정체가 적혀 있지 않다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:36`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:38`
- **범주**: 설계/구조
- **문제**: 바로 위 `CanInteract`는 호출 맥락을 밝힌다(`WxInteractable.h:30-33`). 반면 나머지 두 함수에는 계약 문구가 없고, 실제 규칙은 소비 코드에만 있다.
  - `OnInteracted`의 유일한 호출부는 `ServerOnly` 어빌리티다(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:18`). `CanInteract`(:73)와 사거리 검증(:79)을 통과한 뒤 아바타를 넘긴다(:84).
  - `GetInteractionPrompt`는 로컬 컨트롤러에서만 도는 스캐너만 부른다(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:34-38`, :80).

  그래서 구현체마다 계약을 따로 추측한다.
  - `AWxItemPickup`은 "서버 권위에서만 호출된다"를 자기 주석에 다시 적는다(`Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:76`). 또 현재 호출부로는 도달할 수 없는 PlayerController 분기까지 처리한다(:88-89).
  - `AWxDevice`는 권위와 `CanInteract`를 다시 검사한 뒤 캐릭터로 캐스트한다(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:26`, :37, :43).
  - `UWxDialogueComponent::StartDialogueWith`는 폰만 받는다(`Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp:22`).
  - `AWxEnemyCharacter`는 "프롬프트는 로컬 표시"라는 전제를 자기 주석에 다시 적고 `GetPlayerPawn(this, 0)`에 기댄다(`Source/WxGame/Character/WxEnemyCharacter.cpp:136-137`).

  새 구현체가 `OnInteracted`에 UI 열기 같은 클라 로직을 넣으면, 스탠드얼론이나 리슨 서버 호스트에서는 정상으로 보이고 원격 클라에서만 조용히 빠진다.
- **제안**: 각 함수에 한 줄씩 붙인다.
  - `OnInteracted`: "서버에서, `CanInteract`·사거리 검증을 통과한 뒤에만 호출된다. `Interactor`는 아바타 폰이다."
  - `GetInteractionPrompt`: "로컬 표시용으로만 호출된다."

  계약이 정해지면 `AWxItemPickup`의 PlayerController 분기는 걷어낼 수 있다.
- **확신도**: 중간

### 4. 🟢 `Ability.*` 구획 주석의 "플레이어 캐릭터 전용"·"적 캐릭터 전용"이 실제 사용과 맞지 않는다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:182`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:212`
- **범주**: 설계/구조
- **문제**: `BP_Minion`·`BP_Doppelganger`는 `AWxEnemyCharacter` 파생이다(에셋 이름 테이블, `.codex/worklog/2026-09-16-doppelganger-bt.md:15`). 그런데도 "플레이어 캐릭터 전용" 구획의 태그를 쓴다.
  - `GA_Minion_Attack_Heavy`·`GA_Minion_Skill_2`는 `Ability.Attack.Heavy`·`Ability.Skill.2`를 참조한다.
  - `GA_Doppelganger_Attack_*`는 `Ability.Attack.Light`·`.Heavy`·`.Air`·`.DodgeCounter`를 참조한다.
  - 도플갱어는 애초에 플레이어 킷 전체를 복제하려고 만든 캐릭터다.

  README는 이 헤더를 태그 계약의 색인으로 지목한다. 그래서 구획 이름을 캐릭터 클래스 제한으로 읽은 사람은, 적 계열 소환물에 이 태그를 주는 현재 구성을 오용으로 오판하게 된다.
- **제안**: 구획 이름을 캐릭터 클래스가 아니라 킷 기준으로 고친다(예: "플레이어 킷(플레이어와 그 소환물)", "적 패턴 킷").
- **확신도**: 중간

## 검토 범위
- **깊게 본 파일**:
  - `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`
    - 스크립트로 확인했다: 선언 114개와 정의 114개의 식별자 집합·순서가 일치하고, 식별자의 `_`→`.` 변환이 태그 문자열과 일치한다.
    - 태그별 C++ 사용처를 집계했다. 주석이 지목한 발행·소비자(`UWxAbility_Death`, `AWxCharacterBase`, `WxEffect_*`, `WxHitStopComponent`, `UWxAbilityBase`, `UWxExecCalc_Damage`, `UWxAbility_Ultimate`, `WxDialogueSessionComponent`, `UWxHUDLayout` 등)를 현재 코드와 대조했다. C++ 참조가 0인 태그는 `Content/`·플러그인 `Content/`·`Config/`에 이름 문자열이 있는지 확인했다.
    - 제거된 `Event.CommandMinionAbility`: C++·`Config/`·`Content/`·플러그인 `Content/`에 잔존 참조가 없다. 함께 삭제된 `WxAnimNotify_CommandMinion` 클래스를 참조하는 에셋도 없어 리다이렉트가 필요 없다. 남은 언급은 `.codex/`·`.claude/` 작업 기록뿐이다.
    - 이전 리뷰의 발견 2(`Effect.Invincible` 수명 주인 문구)는 `24f3e9d8`에서 정정되어 뺐다.
  - `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h`, `Plugins/WxCore/Source/WxCore/Private/Minion/WxMinion.cpp`
    - 생성 썽크(`WxMinion.gen.cpp`)를 엔진의 `FBlueprintEditorUtils::ImplementNewInterface`(`BlueprintEditorUtils.cpp:6500`) 및 `UClass::FindFunctionByName`(`Engine/Source/Runtime/CoreUObject/Private/UObject/Class.cpp:7073`)과 대조했다.
    - 호출부로 `WxMinionSubsystem.cpp`와 `Plugins/WxAI/Source/WxAI/Private/WxBTService_UpdateTargetActor.cpp:77-78`을 봤다.
  - `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`
  - `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`: 구현체(`UWxAbilityBase`, `UWxEffectComponent_Table`, `AWxCharacterBase`)와 소비자(WxUI 뷰모델, `Source/WxEditor/WxUIDataThumbnailRenderer.cpp`)를 확인했다.
  - `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`: 외부 호출부 7곳이 모두 `WITH_EDITOR` 안에 있어 비에디터 빌드에서 링크가 깨지지 않는다.
  - `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`: `Config/DefaultEngine.ini:39-40`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp:133`, `Source/WxGame/Character/WxCharacterBase.cpp:26`·:30과 대조했다.
  - `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`
    - Wx 모듈 의존이 없다.
    - 전 파일 첫 줄이 저작권 문구다.
    - 헤더에 인라인 함수 정의가 없다(`inline constexpr` 변수 1개뿐).
- **훑은 파일**:
  - WxCore 자체: `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/README.md`
  - 계약 대조용 소비 코드:
    - 상호작용: `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp`, `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp`, `Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueComponent.cpp`, `Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxNpc.cpp`
    - 소환물: `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp`, `Plugins/WxCombat/Source/WxCombat/Public/Minion/WxMinionSubsystem.h`, `Plugins/WxAI/Source/WxAI/Private/WxBTService_ObserveMasterAbility.cpp`, `Plugins/WxAI/Source/WxAI/Public/WxBTDecorator_MasterAbility.h`
    - 작업 기록: `.codex/worklog/2026-09-16-doppelganger.md`, `.codex/worklog/2026-09-16-doppelganger-bt.md`
- **미검토 / 한계**:
  - 정적 리뷰라 빌드·PIE는 실행하지 않았다.
  - BP·에셋 내부는 범위 밖이다. 에셋 쪽 근거는 이름 테이블에 문자열이 있는지까지만 봤다. 그래서 다음 두 가지는 확인하지 못했다.
    - 1번: 두 소환물 BP가 상한을 저작했는지.
    - 2번: `GA_HGTest_*`가 `State.Minion.Active`를 차단 조건으로 쓰는지, 필요 조건으로 쓰는지.
  - 발견으로 싣지 않은 판단:
    - 예약 슬롯 태그 `Ability.Skill.3`·`.4`와 `Ability.Pattern.4`~`.9`는 에셋 참조가 없다. `Ability.Skill.3`·`.4`의 짝인 `Cooldown.Skill.3`·`.4`는 `UWxEffect_Cooldown_Skill_3/4`로 C++에 있다. `Ability.Pattern.4`는 `92ad73cf` 이후 새로 참조가 끊겼다. 슬롯 확장용 사전 정의로 보고 결함으로 치지 않았다.
    - `UI.Layer.GameMenu`는 레이어 등록(`Plugins/WxUI/Source/WxUI/Private/System/WxPrimaryGameLayout.cpp:15`)과 메뉴 판정 주석(`WxUIManagerSubsystem.cpp:106`) 외에 이 레이어로 푸시하는 곳이 없다. 그래서 주석의 "아이템 획득 알림 등"은 현재로서는 예정 용도다. 레이어 사용은 WxUI 소관이라 제외했다.
    - `Plugins/WxAI/Source/WxAI/Private/WxBTService_ObserveMasterAbility.cpp:114`는 네이티브 `WxGameplayTags::Ability` 대신 `RequestGameplayTag(TEXT("Ability"))` 문자열 조회를 쓴다. WxAI 리뷰 몫으로 남겼다.
    - `Source/WxGame/Character/WxNpc.cpp:40-44`의 `CanInteract`는 권위 측에만 있는 등록부로 답해서 `WxInteractable.h:30-33` 계약에서 벗어난다. 확정된 설계의 구현체 쪽 사항이라 WxGame 리뷰 몫으로 남겼다.
    - `Docs/Programmer/module_review_WxCombat.md:109`의 훑은 파일 목록에는 삭제된 `CommandMinion` 노티파이가 남아 있다. WxCombat 리뷰를 갱신할 때 정리할 몫이다.

---
*문서 기준 커밋 `5eb1a754` · 리뷰일 2026-09-17 · 소스 13파일 — `/module-review`로 갱신*
