# WxCore — 코드 리뷰

> 9파일·500줄의 얇은 foundation 모듈이고 상태는 매우 건강하다. 직전 리뷰의 두 지적 중 `State.Groggy` 미선언 소비는 해소됐고(`WBP_Nameplate_Enemy`가 이제 `Ability.Groggy`를 든다), 직전 커밋 `cf3a7a0`의 `HitReact.*` → `Event.Hit.*` 계층 이관도 코드·에셋 잔재 0건으로 깨끗하게 끝났다. 이번 리뷰는 `*.Build.cs`·`.uplugin`·전 헤더·전 cpp를 통독했고, 네이티브 태그 102개를 선언↔정의·심볼↔문자열로 기계 대조한 뒤 C++ 참조와 `Content/`·`Plugins/*/Content/` 패키지 문자열로 양방향 교차 검증했으며, 두 계약 인터페이스의 구현체 4종과 소비처(WxWorld 스캐너·ST 태스크, WxGame 상호작용 어빌리티)를 따라가 계약 준수를 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 `IWxInteractable::CanInteract()`에 주체 인자가 없어 서버 권위 검증이 주체별로 정확하지 않다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:31`
- **범주**: 설계/구조
- **문제**: `CanInteract()`는 인자가 없어 "누가" 상호작용하려는지를 계약이 전달하지 못한다. 그런데 이 함수는 클라 표시 게이트(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:180`)이자 **서버 권위 자격 검증**(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:75`)의 단일 소스다. 주체 상대 자격이 필요한 구현체는 주체를 스스로 추측할 수밖에 없어, `AWxEnemyCharacter::CanInteract()`의 뒤잡 후방 원뿔 판정이 `UGameplayStatics::GetPlayerPawn(this, 0)`으로 0번 플레이어를 조회한다(`Source/WxGame/Character/WxEnemyCharacter.cpp:136` → `:43-61`, 조회는 `:45`). 싱글·리슨호스트에서는 정확하지만 데디케이티드 멀티에서는 2번째 이후 플레이어의 뒤잡을 서버가 0번 플레이어 위치로 판정한다 — 자기 뒤에 아무도 없는 적을 뒤잡하거나, 정당한 뒤잡이 거부될 수 있다.
- **제안**: 계약을 `CanInteract(const AActor* Interactor) const`로 되돌린다. 발동 경로는 이미 같은 함수 스코프에 아바타를 들고 있고(`WxAbility_Interact.cpp:61`의 `Avatar`, `:86`에서 `OnInteracted`에 넘기는 그 값), 표시 경로는 로컬 폰을 넘기면 되어 두 머신의 답이 같아진다. 데디케이티드 멀티를 목표로 하지 않기로 확정했다면 반대로 헤더에 "주체 상대 자격은 계약이 지원하지 않는다"를 명시해 다음 구현체가 같은 추측을 되풀이하지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음) — 인터페이스 단순화의 대가로 `.claude/worklog/2026-08-22-적-후방-백스탭-복원.md`의 「후속 과제」에 이미 남겨 둔 항목이다. 여기 다시 적는 이유는 그 후속 과제가 사실상 WxCore 계약 변경이라 WxCore 쪽 결정 없이는 움직이지 않기 때문이다.

### 2. 🟢 `SetByCaller.Recovery.UP`/`.MP`는 소비처가 0인 데드 태그이고, 주석이 존재하지 않는 GE를 지목한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:194-198`, 정의 `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:111-112`
- **범주**: 중복/복잡도
- **문제**: 이 두 태그는 C++ 참조 0건, 패키지 문자열 0건으로 프로젝트 전체에서 소비처가 없다. C++ 미참조 태그 38개 중 36개는 전부 에셋이 소비하는 Device·Cue·Ability 식별 태그임을 확인했고, 코드·에셋 모두 0건인 것은 이 둘과 슬롯 번호 예약인 `Ability.Pattern.6`~`9`뿐이다. 게다가 두 주석이 지목하는 `WxEffect_RecoverResource`는 클래스로도 에셋으로도 존재하지 않는다 — `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/`에 GE 20종이 있으나 회복 계열은 `WxEffect_HealPercent`(HP)·`WxEffect_RegenSP`(SP)뿐이고, 리포지터리 전체에서 `RecoverResource` 문자열의 유일한 히트가 이 헤더 자신이다. 이 헤더는 도메인 간 프로토콜의 사전이라 "쓰이는 키"와 "예약 키"가 섞이면 다음 사람이 없는 GE를 찾아 헤맨다.
- **제안**: 두 선언·정의를 지우고 UP/MP 회복 GE를 실제로 만들 때 다시 넣는다. 예약으로 남길 거라면 주석을 "아직 소비처 없음(회복 GE 미구현)"으로 바꿔 없는 클래스를 지목하지 않게 한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`, `Plugins/WxCore/README.md`, `Config/DefaultGameplayTags.ini`, `Config/DefaultEngine.ini`, `Config/DefaultGame.ini`, `Wx.uproject`
- **확인했으나 문제 없던 항목**:
  - **`HitReact.*` → `Event.Hit.*` 이관(직전 커밋 `cf3a7a0`)의 완결성**: 제거된 8개 `HitReact.*` 태그를 코드·전 패키지에서 역방향 검색한 결과 잔재 0건이다(히트는 `GA_HitReact_C`·`WxAbility_HitReact.h` 같은 클래스명뿐). `DT_Damage`의 `HitReactTag` 열도 `Event.Hit.Normal`/`.KnockBack`/`.KnockUp`으로 이미 이관됐고, `FWxDamageTableRow::HitReactTag`의 `meta=(Categories="Event.Hit")`가 새 계층을 가리킨다. `Config/DefaultGameplayTags.ini`에 리다이렉트 없이 지웠지만 소비처가 없어 무해하다.
  - **부모 매칭 규약의 실제 준수**: `WxGameplayTags.h:57`이 요구하는 "부모 매칭 API로 구독"이 지켜진다 — `WxAbility_Guard.cpp:89`은 `WaitGameplayEvent(..., bOnlyMatchExact=false)`, `WxAIPerceptionComponent.cpp:270`은 `AddGameplayEventTagContainerDelegate({Event.Hit})`로 둘 다 자식 이벤트를 받는 경로다. 반대로 `WxAbility_HitReact`는 트리거를 자식 7개에만 등록해(`:37-43`) 반응 없는 평타(`Event.Hit` 부모)에는 뜨지 않는다 — 헤더가 설명한 의도와 일치한다.
  - **태그 정합**: 선언 102개 ↔ 정의 102개 완전 일치(양쪽 차집합 0). 심볼명의 `_`를 `.`로 바꾼 값이 태그 문자열과 102건 모두 일치하고, 중복 심볼·중복 문자열 0건. `WxGameplayTags::` 접두 참조를 기계 수집해 대조한 결과 **C++에서 쓰이나 선언되지 않은 태그는 0건**이다.
  - **미선언 태그 역방향 대조**: 전 패키지에서 프로젝트 루트 네임스페이스(`State|Effect|Movement|Event|Device|GameplayCue|Damage|Ability|SetByCaller|UI`) 모양의 문자열을 뽑아 카탈로그와 차집합을 냈고, 남은 항목은 전부 Niagara 파라미터(`State.Lifetime`·`Movement.PositionDelta` 등)·클래스명(`UI.WxHUDLayout` 등)·바이너리 노이즈였다. **실제 미선언 소비 0건.**
  - **직전 리뷰 지적의 해소**: `State.Groggy`는 더 이상 어디에도 없다 — `Content/UI/Widget/WBP_Nameplate_Enemy.uasset`이 `(TagName="Ability.Groggy")`를 들고 있어 코드(`WxEnemyCharacter.cpp:94`·`:130`이 `Ability_Groggy`로 앞잡/뒤잡을 가름)와 일치한다.
  - **어빌리티 식별 태그 규약**: `WxGameplayTags.h:144-148`이 선언한 "AssetTags와 ActivationOwnedTags 양쪽에 정확히 하나"를 GA 14종 전부가 지킨다(`WxAbility_*.cpp` 기계 대조).
  - **GameplayCue 커버리지**: 큐 태그 9개가 전부 소비처를 갖는다 — C++ 노티파이 5종(`WxCueNotify_Hit`·`_PerfectGuard`·`_Exceed`·`_GhostTrail`·`_DamageFloater`)과 BP 큐 에셋이 모두 `/Game/AbilitySystem/Cue` 아래에 있고, `Config/DefaultGame.ini:21`의 `GameplayCueNotifyPaths`·`:25`의 `DirectoriesToAlwaysCook`가 그 경로를 덮는다.
  - **플러그인 경계**: `WxCore.Build.cs:11-17`은 Core/CoreUObject/Engine/GameplayTags만 참조하고, `WxCore.uplugin`에는 `Plugins` 의존 목록 자체가 없으며, 소스에 다른 Wx 모듈 `#include`가 하나도 없다. `Wx.uproject:39-40`에서 정상 활성. DAG 최하단 규칙 준수.
  - **코딩 규칙**: 10파일(cs 포함) 전부 첫 줄 Copyright 준수(`WxGameplayTags.h/.cpp`만 UTF-8 BOM이 앞서지만 UBT가 `/utf-8`을 넘겨 무해). 람다·델리게이트 콜백·`BlueprintCallable`·`FORCEINLINE`/인라인 함수 정의가 모듈 전체에 0건이고, `WxCollisionChannels.h:15`의 `inline constexpr`은 변수라 규칙 6 대상이 아니다. `Wx` prefix도 전 타입 준수.
  - **`IWxInteractable` 계약 준수**: 순수 가상 2개(`OnInteracted`/`GetInteractionPrompt`)를 구현체 4종(`AWxDialogueActor`·`AWxDevice`·`AWxItemPickup`·`AWxEnemyCharacter`)이 모두 채우고, 기본 구현 2개는 필요한 쪽만 override한다. `OnInteracted`의 유일한 호출처는 `CanInteract()` + 사거리 재검증을 지난 `WxAbility_Interact.cpp:86`이고, ST 태스크는 대상이 계약 미구현일 때 경고를 남긴다(`WxStateTreeTask_EnableInteraction.cpp:80`).
  - **`IWxSavable` 계약**: 액터 전용 규약이 지켜지고(구현체는 `AWxDevice`·`AWxSpawner`), 헤더가 약속한 무효 `GetSaveId()` 제외·전부 기본값이면 미기록·복원 직후 `OnSaveRestored()` 호출이 `WxSaveWorldSubsystem`에 그대로 구현돼 있다.
  - **콜리전 상수**: `ECC_WxAttack = ECC_GameTraceChannel1`이 `Config/DefaultEngine.ini:39`(`Name="WxAttack"`)와 일치하고 — 등록된 `DefaultChannelResponses`가 이 한 줄뿐이라 순서 어긋남이 성립하지 않는다 — 헤더가 말하는 "메시 Overlap·캡슐 Ignore" 오버라이드가 `Source/WxGame/Character/WxCharacterBase.cpp:25`·`:29`에 실존하고, "투사체는 WxProjectile 프리셋"도 `DefaultEngine.ini:40`과 일치한다.
  - **모듈 진입점**: `FWxCoreModule`의 Startup/Shutdown이 빈 구현인 것은 정상이다 — 네이티브 태그는 `UE_DEFINE_GAMEPLAY_TAG`의 정적 초기화가 등록하므로 모듈이 할 일이 없다.
- **미검토 / 한계**: 발견 1의 데디케이티드 멀티 오판정은 코드 경로 추적으로만 확인했고 실제 멀티 세션에서 재현해 보지 않았다. 에셋 대조는 UTF-8 리터럴 문자열 검색 기준이라, 태그를 런타임에 조립하는 BP 노드가 있다면 놓칠 수 있다(C++ 쪽에는 그런 조립이 없음을 확인했다). BP/WBP 내부 그래프 구조는 리뷰 범위 밖이다. 이 환경에는 엔진이 없어 컴파일 검증은 하지 않았다.

---
*문서 기준 커밋 `cf3a7a0` · 리뷰일 2026-08-25 · 소스 9파일 — `/module-review`로 갱신*
