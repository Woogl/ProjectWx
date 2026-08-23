# WxCore — 코드 리뷰

> 9파일·450줄 남짓의 얇은 foundation 모듈이고, 코드 자체는 여전히 깨끗하다. 태그 103개가 `.h` 선언 ↔ `.cpp` 정의로 빠짐없이 짝을 이루고 심볼명↔문자열도 전부 일치하며, 플러그인 의존은 Core/CoreUObject/Engine/GameplayTags뿐이라 경계 침범이 없다. 남은 것은 지난 두 번의 리팩터링(ActivationGroup 전환·`IWxInteractable` 단순화)이 남긴 잔재뿐이다. 이번 리뷰는 `*.Build.cs`·`.uplugin`·전 헤더·전 cpp를 통독했고, 태그 103개 전부를 코드·에셋 참조까지 기계적으로 대조했으며, 두 인터페이스의 실제 소비처(WxWorld 스캐너·ST 태스크, WxGame 상호작용 어빌리티·적 캐릭터, WxDialogue·WxInventory 구현체)를 따라가 계약이 지켜지는지 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 선언 소스가 사라진 `Trait.Ability.Exclusive`를 GA 에셋 3개가 아직 들고 있다
- **위치**: `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:76-109`(Ability 섹션 — `Trait` 정의 없음), `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:153-198`; 잔재 에셋 `Content/AbilitySystem/Ability/GA_Skill_2.uasset`, `GA_Skill_3.uasset`, `GA_Skill_4.uasset`
- **범주**: 중복/복잡도
- **문제**: 배타 액션을 ActivationGroup(`Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h`)으로 옮기며 `Trait_Ability_Exclusive` 네이티브 선언을 WxCore에서 지웠는데, 프로젝트 어디에도 `Trait.*`를 선언하는 코드·ini가 없는 상태에서 위 3개 GA 에셋만 아직 `Trait.Ability.Exclusive` 문자열을 `AbilityTags` 컨테이너에 보관한다(패키지 네임 테이블에 `Trait.Ability.Exclusive`와 `AbilityTags`가 함께 존재, 최근 재저작된 `GA_Skill_1`에는 없음). 엔진은 미등록 태그를 에디터 로드 때 `Invalid GameplayTag ... found in property` 경고만 내고 컨테이너에 그대로 두며, 쿠킹 빌드에서는 경고조차 없다. 태그 노드가 없어 계층 매칭에는 걸리지 않으므로 동작 버그는 아니지만, 에셋을 여는 사람마다 경고를 보고 "아직 Trait 마커가 의미 있나"를 의심하게 되는 죽은 데이터다.
- **제안**: 3개 GA를 에디터에서 열어 `AbilityTags`(현 `AssetTags`)에서 잔재를 지우고 재저장한다. 직전 리뷰 시점 12개에서 3개로 줄어든 것을 보면 이미 손으로 걷어내는 중이므로, 리다이렉트 우회 없이 마저 끝내는 편이 간단하다.
- **확신도**: 높음 (잔재·컨테이너는 패키지 문자열 검색으로 확인, 에디터로 열어 보지는 않음)

### 2. 🟡 `IWxInteractable::CanInteract()`에 주체가 없어 서버 권위 검증이 주체별로 정확하지 않다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:31`
- **범주**: 설계/구조
- **문제**: `CanInteract()`는 인자가 없어 "누가" 상호작용하려는지를 계약이 전달하지 못한다. 그런데 이 함수는 클라 표시 게이트(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:180`)이자 **서버 권위 자격 검증**(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:75`)의 단일 소스다. 주체 상대 자격이 필요한 구현체는 주체를 스스로 추측할 수밖에 없어, `AWxEnemyCharacter::CanInteract()`의 뒤잡 후방 원뿔 판정이 `UGameplayStatics::GetPlayerPawn(this, 0)`으로 0번 플레이어를 조회한다(`Source/WxGame/Character/WxEnemyCharacter.cpp:167`, `:74-92`). 싱글·리슨호스트에서는 정확하지만 데디케이티드 멀티에서는 2번째 이후 플레이어의 뒤잡을 서버가 0번 플레이어 위치로 판정한다 — 자기 뒤에 아무도 없는 적을 뒤잡하거나, 정당한 뒤잡이 거부될 수 있다.
- **제안**: 계약을 `CanInteract(const AActor* Interactor) const`로 되돌리면 표시 경로는 로컬 폰, 발동 경로는 어빌리티 아바타를 넘겨 두 머신의 답이 같아진다. 데디케이티드 멀티를 목표로 하지 않기로 확정했다면 반대로 헤더에 "주체 상대 자격은 계약이 지원하지 않는다"를 명시해 다음 구현체가 같은 추측을 되풀이하지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음) — `5f13671d` 단순화의 대가로 `.claude/worklog/2026-08-22-적-후방-백스탭-복원.md`의 「후속 과제」에 이미 명시적으로 남겨 둔 항목이다. 여기 적는 이유는 그 후속 과제가 사실상 WxCore 계약 변경이라서다.

### 3. 🟢 Ability 섹션 머리주석과 README가 삭제된 `Trait.*` 네임스페이스로 독자를 보낸다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:158`, `Plugins/WxCore/README.md:7`, `Plugins/WxCore/README.md:35`
- **범주**: 중복/복잡도
- **문제**: 헤더 주석은 "어빌리티의 성질을 나타내는 분류 마커는 이 루트가 아니라 Trait.*에 있다"고 안내하고, README는 네임스페이스 목록에 `Trait`를 넣고 "`Trait.Exclusive` 액션 슬롯 점유"까지 설명한다. 그러나 이 헤더에 `Trait` 태그는 하나도 없고 프로젝트 전체에도 선언이 없다(발견 1). 이 헤더는 README가 "다른 모듈을 이해하는 색인"으로 지목한 프로젝트 유일의 태그 어휘 원본이라, 존재하지 않는 네임스페이스를 가리키는 비용이 일반 주석보다 크다. 직전 리뷰에서도 지적됐고 아직 남아 있다.
- **제안**: 158행을 "어빌리티 성질(배타 그룹 등)은 태그가 아니라 `EWxAbilityActivationGroup`(WxCombat)으로 선언한다"로 정정하고, README는 `/readme-writer`로 재생성해 `Trait` 항목을 걷어낸다.
- **확신도**: 높음

### 4. 🟢 `SetInteractionEnabled` 주석이 이제 없는 `IsInteractionEnabled`를 가리킨다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:39`
- **범주**: 중복/복잡도
- **문제**: 주석이 "켜고 끄는 수단은 구현체가 정한다 — `IsInteractionEnabled` 의 답이 그에 맞게 바뀌기만 하면 된다"고 적혀 있으나, `5f13671d`의 인터페이스 단순화로 `IsInteractionEnabled`/`CanBeInteractedBy`는 `CanInteract()` 하나로 합쳐져 사라졌다. 프로젝트 전체에 그 이름의 함수는 없다. 계약 헤더의 서술이라 구현체를 새로 만드는 사람이 없는 함수를 찾게 된다. 같은 잔재가 `Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:29` 주석에도 있다.
- **제안**: 두 주석의 `IsInteractionEnabled`를 `CanInteract`로 바꾼다.
- **확신도**: 높음

### 5. 🟢 `Gimmick.* → Device.*` 태그 리다이렉트 9줄이 전부 죽었고, 그중 하나는 존재하지 않는 태그를 가리킨다
- **위치**: `Config/DefaultGameplayTags.ini:2-11`
- **범주**: 중복/복잡도
- **문제**: 이 파일은 WxCore가 소유한 태그 어휘의 유일한 코드 밖 소스인데, `Content/`·`Plugins/*/Content/` 어느 패키지에도 `Gimmick.` 문자열이 남아 있지 않다 — 리다이렉트 9줄 전부가 대상 없이 상주 중이다. 게다가 7행은 `Gimmick.Elevator.Moving`을 `Device.Elevator.Moving`으로 옮기는데, `Device.Elevator.Moving`은 `Moving` 상태 삭제로 이미 WxCore에서 선언이 사라졌다(`Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:48-50`은 `Inactive`/`1F`/`2F`뿐). 즉 옛 문자열을 든 에셋이 혹시 나타나더라도 미등록 태그로 리다이렉트되어 문제가 그대로 남는다. "태그는 `WxGameplayTags.h/.cpp` 쌍으로만"이라는 규칙 아래 ini에 남은 유일한 예외 블록이라, 죽은 채로 두면 다음 사람이 "여기도 태그 소스인가"를 되묻게 된다.
- **제안**: 리다이렉트 블록을 삭제한다(이미 전 에셋이 이관 완료). 삭제가 불안하면 최소한 7행만이라도 지운다 — 유효한 목적지가 없는 리다이렉트다.
- **확신도**: 높음 (모듈 디렉터리 밖이지만 WxCore 태그 네임스페이스에 직접 걸린 항목이라 여기 적는다)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`
- **확인했으나 문제 없던 항목**:
  - **태그 정합**: 선언 103개 ↔ 정의 103개가 완전 일치(양쪽 차집합 0). 심볼명의 `_`를 `.`로 바꾼 값이 태그 문자열과 103건 모두 일치하고, 중복 문자열도 없다.
  - **데드 태그**: C++ 미참조 36개 중 에셋 참조까지 0인 것은 `Ability.Pattern.6`~`9` 4개뿐이며, 이는 패턴 슬롯 번호 예약이라 데드로 보지 않았다. `Device.*` 15개는 설계상 StateTree 에셋 전용 소비 태그로 전부 에셋이 참조하고 있다(`Device.Piston.*` 포함).
  - **플러그인 경계**: `WxCore.Build.cs:11-17`은 Core/CoreUObject/Engine/GameplayTags만, `WxCore.uplugin`은 `Plugins` 의존 목록이 비었고, 소스에 다른 Wx 모듈 `#include`가 없다. 소비 플러그인 8종이 모두 `WxCore`를 `.uplugin` 의존으로 선언하고 `LoadingPhase`가 전부 `Default`라 네이티브 태그 등록 순서 문제도 없다.
  - **코딩 규칙**: 9파일 전부 첫 줄 Copyright 준수(`WxGameplayTags.h/.cpp`만 UTF-8 BOM이 앞서지만 UBT가 `/utf-8`을 넘겨 무해), 람다·델리게이트 콜백·`BlueprintCallable`·인라인 함수 정의가 하나도 없다. `WxCollisionChannels.h:15`의 `inline constexpr`은 변수라 규칙 6 대상이 아니다. `Wx` prefix도 전 타입 준수.
  - **`IWxInteractable` 계약 준수**: 순수 가상 2개(`OnInteracted`/`GetInteractionPrompt`)는 구현체 4종(`AWxDialogueActor`·`AWxDevice`·`AWxItemPickup`·`AWxEnemyCharacter`)이 모두 채우고, 기본 구현 2개는 필요한 쪽만 override한다. `OnInteracted`의 유일한 호출처는 `HasAuthority` 뒤 + `CanInteract()` + 사거리 재검증을 지난 `WxAbility_Interact.cpp:86`이고, 비권위 호출처는 없다. `WxStateTreeTask_EnableInteraction.cpp:72-80`도 `Cast` 실패를 앞에서 걸러 넘긴다.
  - **`IWxSavable` 계약**: 액터 전용 규약이 지켜지고(구현체는 `AWxDevice` 등 액터만), `GetSaveId()` 무효 시 저장·복원 제외라는 헤더의 약속이 `Plugins/WxSave` 서브시스템 구현과 일치한다(직전 리뷰에서 라인 단위로 확인, 이번엔 재확인만).
  - **콜리전 상수**: `ECC_WxAttack = ECC_GameTraceChannel1`이 `Config/DefaultEngine.ini:39`(`Name="WxAttack"`, `DefaultResponse=ECR_Block`, `bTraceType=False`)와 일치하고, 헤더 주석이 말하는 "투사체는 WxProjectile 프리셋"도 같은 파일 40행에 실존한다. 작업 트리의 `DefaultEngine.ini` 미커밋 변경은 시작 맵·CVar 한 줄뿐이라 채널과 무관하다.
  - **모듈 진입점**: `FWxCoreModule`의 Startup/Shutdown이 빈 구현인 것은 정상이다 — 네이티브 태그는 `UE_DEFINE_GAMEPLAY_TAG`의 정적 초기화가 등록하므로 모듈이 할 일이 없다.
- **미검토 / 한계**: 발견 1의 잔재가 실제로 `AbilityTags` 컨테이너 안에 있다는 것은 패키지 네임 테이블 문자열 검색까지만 확인했고 에디터로 열어보지는 않았다(에디터 미실행으로 MCP 조회 불가). 발견 2의 데디케이티드 멀티 오판정은 코드 경로 추적으로만 확인했고 실제 멀티 세션에서 재현해 보지 않았다.

---
*문서 기준 커밋 `807a9da8` · 리뷰일 2026-08-24 · 소스 9파일 — `/module-review`로 갱신*
