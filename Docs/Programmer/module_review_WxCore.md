# WxCore — 코드 리뷰

> 9파일·450줄 남짓의 얇은 foundation 모듈이고, 코드 자체는 깨끗하다. 네이티브 태그 102개가 `.h` 선언 ↔ `.cpp` 정의로 빠짐없이 짝을 이루고 심볼명(`_`→`.`)과 태그 문자열이 102건 전부 일치하며 중복 문자열도 없다. 의존은 Core/CoreUObject/Engine/GameplayTags뿐이라 플러그인 경계 침범이 없고, 코딩 규칙 위반도 0건이다. 남은 지적은 전부 지난 리팩터링(`Trait.*` 삭제·`IWxInteractable` 단순화·Gimmick→Device 개명)이 남긴 잔재다. 이번 리뷰는 `*.Build.cs`·`.uplugin`·전 헤더·전 cpp를 통독했고, 태그 102개를 C++·에셋 참조까지 기계적으로 대조했으며, 두 계약 인터페이스의 실제 소비처(WxWorld 스캐너, WxGame 상호작용 어빌리티, WxSave 서브시스템)를 따라가 계약 준수를 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 삭제된 `Trait.*` 네임스페이스가 태그 카탈로그 주석·README·GA 에셋 3개에 잔재로 남아 있다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:151`, `Plugins/WxCore/README.md:35`; 잔재 에셋 `Content/AbilitySystem/Ability/GA_Skill_2.uasset`, `Content/AbilitySystem/Ability/GA_Skill_3.uasset`, `Content/AbilitySystem/Ability/GA_Skill_4.uasset`
- **범주**: 중복/복잡도
- **문제**: 배타 액션을 `EWxAbilityActivationGroup`(WxCombat)으로 옮기면서 `Trait_Ability_Exclusive` 네이티브 선언이 WxCore에서 사라졌고, 프로젝트 전체를 훑어도 `Trait.`를 선언하는 코드·ini가 하나도 없다(유일한 히트가 `WxGameplayTags.h:151` 주석 자신이다). 그런데 (a) Ability 섹션 머리주석은 아직 "어빌리티의 성질을 나타내는 분류 마커는 이 루트가 아니라 Trait.\*에 있다"고 안내하고, (b) README도 `Ability.*` 설명에 "(성질 분류는 별도 `Trait.*`)"를 달고 있으며, (c) 위 GA 에셋 3개는 여전히 `Trait.Ability.Exclusive` 문자열을 패키지에 보관한다. 엔진은 미등록 태그를 에디터 로드 시 `Invalid GameplayTag ... found in property` 경고로만 알리고 컨테이너에 남겨 두므로 동작 버그는 아니다. 다만 이 헤더는 README가 "도메인 간 프로토콜의 사전"으로 지목한 프로젝트 유일의 태그 어휘 원본이라, 존재하지 않는 네임스페이스를 가리키는 비용이 일반 주석보다 크다 — 에셋을 여는 사람마다 "아직 Trait 마커가 살아 있나"를 되묻게 된다.
- **제안**: `WxGameplayTags.h:151`을 "어빌리티 성질(배타 그룹 등)은 태그가 아니라 `EWxAbilityActivationGroup`(WxCombat)으로 선언한다"로 정정하고, README는 `/readme-writer`로 재생성한다. GA 3개는 에디터에서 열어 `AssetTags`의 잔재를 지우고 재저장한다(직전 리뷰 시점 12개 → 현재 3개로 이미 손으로 걷어내는 중이라, 리다이렉트 우회 없이 마저 끝내는 편이 간단하다).
- **확신도**: 높음 (에셋 잔재는 패키지 문자열 검색까지만 확인, 에디터로 열어 보지는 않았다)

### 2. 🟡 `IWxInteractable::CanInteract()`에 주체 인자가 없어 서버 권위 검증이 주체별로 정확하지 않다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:31`
- **범주**: 설계/구조
- **문제**: `CanInteract()`는 인자가 없어 "누가" 상호작용하려는지를 계약이 전달하지 못한다. 그런데 이 함수는 클라 표시 게이트(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:180`)이자 **서버 권위 자격 검증**(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:75`)의 단일 소스다. 주체 상대 자격이 필요한 구현체는 주체를 스스로 추측할 수밖에 없어, `AWxEnemyCharacter::CanInteract()`의 뒤잡 후방 원뿔 판정이 `UGameplayStatics::GetPlayerPawn(this, 0)`으로 0번 플레이어를 조회한다(`Source/WxGame/Character/WxEnemyCharacter.cpp:167` → `:74-92`). 싱글·리슨호스트에서는 정확하지만 데디케이티드 멀티에서는 2번째 이후 플레이어의 뒤잡을 서버가 0번 플레이어 위치로 판정한다 — 자기 뒤에 아무도 없는 적을 뒤잡하거나, 정당한 뒤잡이 거부될 수 있다.
- **제안**: 계약을 `CanInteract(const AActor* Interactor) const`로 되돌린다. 발동 경로는 이미 같은 함수 스코프에 아바타를 들고 있고(`WxAbility_Interact.cpp:61`의 `Avatar`, `:86`에서 `OnInteracted`에 넘기는 그 값), 표시 경로는 로컬 폰을 넘기면 되어 두 머신의 답이 같아진다. 데디케이티드 멀티를 목표로 하지 않기로 확정했다면 반대로 헤더에 "주체 상대 자격은 계약이 지원하지 않는다"를 명시해 다음 구현체가 같은 추측을 되풀이하지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음) — `5f13671d` 단순화의 대가로 `.claude/worklog/2026-08-22-적-후방-백스탭-복원.md`의 「후속 과제」에 이미 남겨 둔 항목이다. 여기 적는 이유는 그 후속 과제가 사실상 WxCore 계약 변경이라서다.

### 3. 🟢 `SetInteractionEnabled` 주석이 이제 없는 `IsInteractionEnabled`를 가리킨다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:39`
- **범주**: 중복/복잡도
- **문제**: 주석이 "켜고 끄는 수단은 구현체가 정한다 — `IsInteractionEnabled` 의 답이 그에 맞게 바뀌기만 하면 된다"고 적혀 있으나, 인터페이스 단순화로 `IsInteractionEnabled`/`CanBeInteractedBy`는 `CanInteract()` 하나로 합쳐져 사라졌다. 프로젝트 전체에 그 이름의 함수 선언·정의가 없다(남은 히트는 이 주석과 아래 주석 둘뿐). 계약 헤더의 서술이라 구현체를 새로 만드는 사람이 없는 함수를 찾게 된다. 같은 잔재가 `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:29` 주석에도 있다.
- **제안**: 두 주석의 `IsInteractionEnabled`를 `CanInteract`로 바꾼다.
- **확신도**: 높음

### 4. 🟢 `Gimmick.* → Device.*` 태그 리다이렉트 9줄이 전부 죽었고, 그중 하나는 존재하지 않는 태그를 가리킨다
- **위치**: `Config/DefaultGameplayTags.ini:2-11` (특히 `:7`)
- **범주**: 중복/복잡도
- **문제**: 이 파일은 WxCore가 소유한 태그 어휘의 유일한 코드 밖 소스인데, `Content/`·`Plugins/*/Content/` 어느 패키지에도 `Gimmick.` 문자열이 남아 있지 않다 — 리다이렉트 9줄 전부가 대상 없이 상주 중이다. 게다가 7행은 `Gimmick.Elevator.Moving`을 `Device.Elevator.Moving`으로 옮기는데, `Device.Elevator.Moving`은 WxCore에 선언이 없다(`Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:47-49`는 `Inactive`/`1F`/`2F`뿐이고, 프로젝트 전체에서 `Elevator.Moving`을 아는 곳은 이 ini 한 줄이다). 즉 옛 문자열을 든 에셋이 혹시 나타나더라도 미등록 태그로 리다이렉트되어 문제가 그대로 남는다. "태그는 `WxGameplayTags.h/.cpp` 쌍으로만"이라는 규약 아래 ini에 남은 유일한 예외 블록이라, 죽은 채로 두면 다음 사람이 "여기도 태그 소스인가"를 되묻게 된다.
- **제안**: 리다이렉트 블록을 삭제한다(전 에셋 이관 완료). 삭제가 불안하면 최소한 7행만이라도 지운다 — 유효한 목적지가 없는 리다이렉트다.
- **확신도**: 높음 (모듈 디렉터리 밖이지만 WxCore 태그 네임스페이스에 직접 걸린 항목이라 여기 적는다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`, `Plugins/WxCore/README.md`
- **확인했으나 문제 없던 항목**:
  - **태그 정합**: 선언 102개 ↔ 정의 102개 완전 일치(양쪽 차집합 0). 심볼명의 `_`를 `.`로 바꾼 값이 태그 문자열과 102건 모두 일치하고, 중복 문자열도 없다.
  - **직전 리뷰 이후 델타**: `State_ComboWindow`·`GameplayCue_Burn` 삭제, `GameplayCue_GhostTrail` 추가(103→102). 삭제 두 건은 에셋 잔재가 0이라 깨끗하고, 신규 큐는 `Content/AbilitySystem/Cue/GC_GhostTrail.uasset`으로 존재하며 `Config/DefaultGame.ini:21`의 `GameplayCueNotifyPaths`와 `:25`의 `DirectoriesToAlwaysCook`가 모두 `/Game/AbilitySystem/Cue`를 덮어 쿠킹 누락 위험도 없다.
  - **데드 태그**: C++ 미참조 38개는 전부 설계상 에셋 전용 소비 태그다 — `Device.*` 15개(StateTree 상태값, 헤더가 명시), `GameplayCue.AttackTelegraph.*` 4개(큐 노티파이), `SetByCaller.Recovery.*` 2개(GE 모디파이어), `Ability.*` 식별 태그 17개(GA 에셋 `AssetTags`/`ActivationOwnedTags`). `Ability.Pattern.6`~`9`는 패턴 슬롯 번호 예약이라 데드로 보지 않았다.
  - **플러그인 경계**: `WxCore.Build.cs:11-17`은 Core/CoreUObject/Engine/GameplayTags만 참조하고, `WxCore.uplugin`에는 `Plugins` 의존 목록 자체가 없으며, 소스에 다른 Wx 모듈 `#include`가 하나도 없다. DAG 최하단 규칙 준수.
  - **코딩 규칙**: 10파일(cs 포함) 전부 첫 줄 Copyright 준수(`WxGameplayTags.h/.cpp`만 UTF-8 BOM이 앞서지만 UBT가 `/utf-8`을 넘겨 무해). 람다·델리게이트 콜백·`BlueprintCallable`·`FORCEINLINE`/인라인 함수 정의가 모듈 전체에 0건이다. `WxCollisionChannels.h:15`의 `inline constexpr`은 변수이므로 규칙 6 대상이 아니다. `Wx` prefix도 전 타입 준수.
  - **`IWxInteractable` 계약 준수**: 순수 가상 2개(`OnInteracted`/`GetInteractionPrompt`)를 구현체 4종(`AWxDialogueActor`·`AWxDevice`·`AWxItemPickup`·`AWxEnemyCharacter`)이 모두 채우고, 기본 구현 2개는 필요한 쪽만 override한다. `OnInteracted`의 유일한 호출처는 `CanInteract()` + 사거리 재검증을 지난 `WxAbility_Interact.cpp:86`이고 비권위 호출처는 없다.
  - **`IWxSavable` 계약**: 액터 전용 규약이 지켜지고(구현체는 `AWxDevice`·`AWxSpawner` 등 액터만), "무효 `GetSaveId()`는 저장·복원에서 제외"라는 헤더의 약속이 구현과 일치한다(`Plugins/WxSave/Source/WxSave/Private/WxSaveWorldSubsystem.cpp:311-316`, `:385-390` — 양쪽 다 `IsValid()` 실패 시 경고 후 조기 반환).
  - **콜리전 상수**: `ECC_WxAttack = ECC_GameTraceChannel1`이 `Config/DefaultEngine.ini:39`(`Name="WxAttack"`, `DefaultResponse=ECR_Block`, `bTraceType=False`)와 일치하고, 헤더 주석이 말하는 "캐릭터 메시는 Overlap, 캡슐은 Ignore" 오버라이드가 `Source/WxGame/Character/WxCharacterBase.cpp:25`·`:29`에 실존하며, "투사체는 WxProjectile 프리셋"도 `DefaultEngine.ini:40`과 `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:23`에서 확인된다.
  - **모듈 진입점**: `FWxCoreModule`의 Startup/Shutdown이 빈 구현인 것은 정상이다 — 네이티브 태그는 `UE_DEFINE_GAMEPLAY_TAG`의 정적 초기화가 등록하므로 모듈이 할 일이 없다.
- **미검토 / 한계**: 발견 1의 GA 에셋 잔재가 실제로 `AssetTags` 컨테이너 안에 있다는 것은 패키지 문자열 검색까지만 확인했고 에디터로 열어보지는 않았다. 발견 2의 데디케이티드 멀티 오판정은 코드 경로 추적으로만 확인했고 실제 멀티 세션에서 재현해 보지 않았다. BP/WBP·StateTree 에셋 내부 구조는 리뷰 범위 밖이라 태그 문자열 존재 여부까지만 대조했다.

---
*문서 기준 커밋 `13b45192` · 리뷰일 2026-08-25 · 소스 9파일 — `/module-review`로 갱신*
