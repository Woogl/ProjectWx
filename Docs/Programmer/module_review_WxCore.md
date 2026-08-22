# WxCore — 코드 리뷰

> 9파일·400줄 남짓의 얇은 foundation 모듈이며 코드 자체는 깨끗하다. 태그 97개가 `.h` 선언 ↔ `.cpp` 정의로 완전히 짝을 이루고, `IWxInteractable`/`IWxSavable`가 액터 전용으로 정리되면서 이전 리뷰의 컴포넌트 `Find` 비결정성·중복 발견은 소멸했다. 남은 것은 지난 ActivationGroup 전환이 남긴 `Trait.*` 잔재(헤더 주석·README·GA 에셋 12개)뿐이다. 이번 리뷰는 `*.Build.cs`·`.uplugin`·전 헤더·전 cpp를 모두 읽었고, 두 인터페이스의 실제 소비처(WxWorld 스캐너, WxGame 상호작용 어빌리티, WxSave 서브시스템)까지 따라가 헤더가 약속한 계약이 지켜지는지 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 선언 소스가 사라진 `Trait.Ability.Exclusive`를 GA 에셋 12개가 아직 들고 있다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:143-189`(Ability 섹션 — `Trait` 선언 없음), `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:68-101`, `Config/DefaultGameplayTags.ini:3-11`(리다이렉트는 `Gimmick.*→Device.*`뿐); 잔재 에셋 `Content/AbilitySystem/Ability/GA_Attack_Light`, `GA_Attack_Heavy`, `GA_Attack_Air`, `GA_Attack_DodgeCounter`, `GA_Pattern_1`~`4`, `GA_Skill_2`~`4`, `GA_Skill_E` (.uasset 12개)
- **범주**: 중복/복잡도
- **문제**: 배타 액션을 ActivationGroup(`Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h:34`)으로 옮기며 `Trait_Ability_Exclusive` 네이티브 선언을 WxCore에서 지웠는데, 프로젝트에 `Trait.*`를 선언하는 곳이 코드·ini 어디에도 없어진 반면 위 12개 GA 에셋은 여전히 `Trait.Ability.Exclusive` 문자열을 태그 컨테이너에 보관한다. 엔진은 이런 미등록 태그를 에디터 로드 때 `Invalid GameplayTag ... found in property` 경고만 내고 컨테이너에 그대로 둔다(`Engine/Source/Runtime/GameplayTags/Private/GameplayTagsManager.cpp:1064-1080`, 제거는 리다이렉트 대상만 `1085-1089`). 쿠킹 빌드에서는 경고조차 없다("too late to fix it in cooked builds"). 태그 노드가 없어 계층 매칭에는 걸리지 않으므로 동작 버그는 아니지만, 에셋을 여는 사람마다 경고를 보고 "아직 Trait 마커가 의미 있나"를 의심하게 되는 죽은 데이터다. 워크로그 `2026-08-21-배타-액션-ActivationGroup-전환.md:53`이 이미 "별건, 우선순위 상향"으로 남겨 둔 항목이다.
- **제안**: (a) 12개 GA를 에디터에서 열어 AssetTags/CancelAbilitiesWithTag 잔재를 비우고 재저장한다. (b) 손으로 열기 싫으면 `Config/DefaultGameplayTags.ini`에 `+GameplayTagRedirects=(OldTagName="Trait.Ability.Exclusive",NewTagName="")` 한 줄을 넣는다 — `NewTagName`이 비면 `FGameplayTag(NAME_None)`으로 등록되고(`GameplayTagRedirectors.cpp:120,149`) 컨테이너 로드 시 "제거 + 추가 없음" 경로를 타므로(`GameplayTagsManager.cpp:1056-1062`) 자동으로 걷힌다. 이 경우 에셋을 한 번 재저장한 뒤 리다이렉트 줄을 지운다.
- **확신도**: 높음 (잔재 존재는 바이너리 문자열 검색으로 확인, 어느 컨테이너에 남았는지는 에디터로 열어보지 않음)

### 2. 🟢 Ability 섹션 머리주석과 README가 삭제된 `Trait.*` 네임스페이스로 독자를 보낸다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:148`, `Plugins/WxCore/README.md:7`, `Plugins/WxCore/README.md:35`
- **범주**: 중복/복잡도
- **문제**: 헤더 주석은 "어빌리티의 성질을 나타내는 분류 마커는 이 루트가 아니라 Trait.*에 있다"고 안내하고, README는 네임스페이스 목록에 `Trait`를 넣고 "`Trait.Exclusive` 액션 슬롯 점유"까지 설명한다. 그러나 현재 이 헤더에는 `Trait` 태그가 하나도 없다(발견 1). 이 헤더는 README가 "다른 모듈을 이해하는 색인"으로 지목한 프로젝트 유일의 태그 어휘 원본이라, 존재하지 않는 네임스페이스를 가리키는 비용이 일반 주석보다 크다. 워크로그 `2026-08-21-배타-액션-ActivationGroup-전환.md:36`은 "주석표 포함 삭제"라고 적었지만 148행은 남았다.
- **제안**: 148행을 "어빌리티 성질(배타 그룹 등)은 태그가 아니라 `EWxAbilityActivationGroup`(WxCombat)으로 선언한다"로 바꾸고, README는 `/readme-writer`로 재생성해 `Trait` 항목을 걷어낸다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`
- **확인했으나 문제 없던 항목**:
  - 플러그인 경계: `WxCore.Build.cs:11-17`은 Core/CoreUObject/Engine/GameplayTags만, `.uplugin`은 `Plugins` 의존 목록 자체가 없고, 소스에 다른 Wx 모듈 `#include`가 없다. 모든 소비 플러그인의 `LoadingPhase`가 `Default`라 네이티브 태그 등록 순서 문제도 없다.
  - 코딩 규칙: 전 파일 첫 줄 Copyright 준수(`WxGameplayTags.h/.cpp`만 UTF-8 BOM이 앞에 붙지만 UBT가 `/utf-8`을 넘겨 무해), 람다·델리게이트·`BlueprintCallable`·인라인 함수 정의 없음. `WxCollisionChannels.h:15`의 `inline constexpr`은 변수라 규칙 6 대상이 아니다.
  - `IWxInteractable` 계약이 소비처에서 실제로 지켜진다: 클라 스캐너가 `IsInteractionEnabled && CanBeInteractedBy`로 후보를 거르고(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:185`), 서버 어빌리티가 `HasAuthority` 뒤에서 같은 두 함수 + 사거리로 재검증한 뒤에만 `OnInteracted`를 부른다(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:50,75,81,88,93`). `OnInteracted`의 비권위 호출처는 없다.
  - `IWxSavable` 계약도 구현과 일치한다: `GetSaveId()` 무효 시 저장·복원 제외(`Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:313-318,387-392`), 액터·컴포넌트 전부 기본값이면 레코드를 쓰지 않을 뿐 아니라 옛 레코드까지 지워 "기본값으로 되돌아온 액터가 옛 상태로 복원"되지 않게 하고(`:354-359`), 그 경우 트랜스폼도 함께 빠지며(`:361`), 필드 복원 직후 `OnSaveRestored()`가 호출된다(`:453`).
  - `ECC_WxAttack = ECC_GameTraceChannel1`이 `Config/DefaultEngine.ini:39`(`Name="WxAttack"`, `DefaultResponse=ECR_Block`, `bTraceType=False`)와 일치하고, 헤더 주석의 "메시 Overlap·캡슐 Ignore 명시 override"도 `Source/WxGame/Character/WxCharacterBase.cpp:25,29`와 맞는다.
  - 태그 주석이 지목하는 타 모듈 클래스 14종(`WxAbility_Finisher`, `WxAnimNotifyState_ComboWindow`, `WxEffect_*`, `WxExecCalc_Damage`, `WxCharacterMovementComponent` 등)과 GE 에셋(NoCooldown/InfiniteMP/DrainDP)이 모두 실존하고, `State_Dialogue`는 주석대로 `WxAbility_Interact.cpp:37`에서 `ActivationBlockedTags`로 쓰인다.
  - C++ 무참조 태그 30개(`Device.*`, `GameplayCue.AttackTelegraph.*`, `Ability.Attack.*`/`Skill.N`/`Pattern.N`)는 설계상 에셋 소비 태그다. 그중 `Device.Elevator.*` 3개와 `Ability.Pattern.6`~`9`는 에셋 참조도 0이지만, 엘리베이터는 `Docs/SystemDesign/Object_Design.md:93`에 구현 예정으로 적힌 장치이고 Pattern 슬롯은 번호 예약이라 데드로 보지 않았다.
- **미검토 / 한계**: 발견 1의 잔재가 12개 에셋의 어느 컨테이너(AssetTags / BlockAbilitiesWithTag / CancelAbilitiesWithTag)에 남았는지는 바이너리 문자열 검색까지만 확인했고 에디터로 열어보지 않았다. 제안 (b)의 빈 `NewTagName` 리다이렉트는 엔진 소스 경로로 확인했을 뿐 이 프로젝트에서 실제 실행해 보지는 않았다.

---
*문서 기준 커밋 `bd689a19` · 리뷰일 2026-08-22 · 소스 9파일 — `/module-review`로 갱신*
