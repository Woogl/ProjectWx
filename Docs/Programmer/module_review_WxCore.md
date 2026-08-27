# WxCore — 코드 리뷰

> 선언과 계약만 담은 얇은 파운데이션이라 여전히 매우 건강하다. CLAUDE.md 코딩·모듈 규칙 위반은 한 건도 없고, 이전 리뷰가 지적한 두 건(`GetActorLabel` 부작용, UOL 의 Public 런타임 의존)은 모두 수정되어 있다. 이번 리뷰는 `Plugins/WxCore/Source/WxCore` 의 11개 소스 전부를 통독했고, 계약의 실효성을 확인하려고 소비 측 구현체 5개(`AWxDevice`·`AWxDialogueActor`·`AWxItemPickup`·`AWxEnemyCharacter`·`AWxSpawner`)와 호출 지점(스캐너·ST 태스크·WxSave 서브시스템)까지 함께 읽었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 `SetInteractionEnabled` 의 기본 no-op 이 토글 미지원 대상을 조용히 삼킨다

- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:37`~`40`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp:5`~`7`
- **범주**: 설계/구조
- **문제**: 계약이 `SetInteractionEnabled` 를 기본 no-op 으로 두고, 이를 `CanInteract` 와 잇는 일은 전적으로 구현체에 맡긴다. 그런데 호출자에겐 대상이 토글을 실제로 지원하는지 알 방법이 없다.
  실제로 `AWxItemPickup`(`Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemPickup.h:41`~`44`)과 `AWxEnemyCharacter`(`Source/WxGame/Character/WxEnemyCharacter.h:37`~`41`)는 이 함수를 override 하지 않는다. 반면 `FWxStateTreeTask_EnableInteraction::ApplyTargetInteraction` 은 대상이 `IWxInteractable` 이기만 하면 무조건 부르고 `Succeeded` 를 답한다(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxStateTreeTask_EnableInteraction.cpp:85`,`:87`). 경고는 "대상이 상호작용 대상이 아님"일 때만 나간다(같은 파일 `:80`).
  결과적으로 레벨 디자이너가 '상호작용 켜기/끄기' 노드로 픽업이나 적을 지목하면 아무 일도 일어나지 않고 아무 신호도 없다. 계약을 넓히는 방향(README 가 예고한 '옵션 목록' 확장 등)으로 갈수록 이런 조용한 무시가 늘어난다.
- **제안**: 둘 중 하나. (a) `SetInteractionEnabled` 를 순수 가상으로 올려 구현체가 "토글 지원 여부"를 명시적으로 선택하게 한다 — 미지원 대상은 빈 구현 + 이유 주석으로 남으니 의도가 코드에 드러난다. (b) 최소 조치로 헤더에 "기본 구현은 토글을 지원하지 않는다(= 언제나 `CanInteract` 판정 그대로). 토글을 쓰려면 반드시 override 해 `CanInteract` 와 이어야 한다"를 명시한다.
- **확신도**: 중간(항상 상호작용 가능한 액터를 위한 의도된 기본값일 수 있음)

### 2. 🟢 `GetInteractionPrompt()` 의 반환 범위가 계약에 없어 구현체마다 키 힌트 유무가 갈린다

- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:35`
- **범주**: 설계/구조
- **문제**: 반환 텍스트가 "표시 문구만"인지 "키 힌트까지 포함"인지 계약에 없다. 그래서 구현체 넷이 서로 다르게 답한다 — `AWxItemPickup` 은 로컬라이즈 문자열에 키를 박아 `"[F] {0}"` 로(`Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:107`~`108`), `AWxDevice` 는 ST 가 세팅한 순수 문구로(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:96`), `AWxDialogueActor` 는 컴포넌트 문구로(`Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp:29`), `AWxEnemyCharacter` 는 비로컬라이즈 리터럴로(`Source/WxGame/Character/WxEnemyCharacter.cpp:107`). 프롬프트를 한 줄에 나열하는 스캐너 UI(`WxInteractionScannerComponent::GetPrompts`) 에서 픽업만 `[F]` 를 달고 나오며, 키 리바인딩이 들어오면 이 문자열이 거짓말이 된다.
- **제안**: 헤더에 한 줄로 못박는다 — "키 힌트를 포함하지 않는 표시 문구만 답한다. 입력 힌트는 UI 가 붙인다." 그 뒤 픽업의 `[F]` 를 걷어내는 것은 WxInventory/WxUI 쪽 후속 작업.
- **확신도**: 중간

### 3. 🟢 `Device.*` 15개 태그의 예외 사유가 여전히 헤더에 없다

- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:98`~`121`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:41`~`59`
- **범주**: 설계/구조
- **문제**: 이전 리뷰(커밋 `8a9bb7e6`)가 같은 항목을 제기하며 "현행 유지를 택한다면 헤더에 근거 한 줄을 못박아 반복 제기되지 않게 하라"고 남겼는데, 주석은 그대로다(`WxGameplayTags.h:100`~`101` 은 여전히 "코드에서 읽거나 쓰는 값은 아니지만 태그는 여기서 정의한다"까지만 말한다). 이번 확인에서도 `Device.*` 는 WxCore 밖 C++ 참조가 0건인 순수 저작 어휘였다.
- **제안**: 태그 허브 구조는 그대로 두고, Device 섹션 주석에 "태그는 전부 `WxGameplayTags` 쌍으로만 추가한다는 상위 규칙이 도메인 소유권보다 우선하므로 여기 둔다"를 덧붙여 논의를 종결한다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위

- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **미검토 / 한계**: 네이티브 태그 102개가 BP·에셋에서 실제로 소비되는지는 콘텐츠 바이너리를 스캔하지 않아 확인하지 않았다. 따라서 "죽은 태그" 판정은 하지 않았고, 발견 3도 어휘가 죽었다는 주장이 아니라 소유권 근거 주석의 부재를 가리킨다. 아래는 이번에 확인했으나 문제가 없어 발견으로 올리지 않은 것들이다(다음 리뷰의 오탐 방지용).
  - 태그 선언 102개와 정의 102개가 정확히 일치하고, WxCore 밖에 네이티브 태그 선언이 한 건도 없으며 `Config/DefaultGameplayTags.ini` 도 없다 — "태그는 이 파일 쌍에만"이라는 규칙이 저장소 전체에서 지켜지고 있다.
  - `WxGameplayTags.h` 는 60개 `.cpp` 에서만 include 되고 공개 헤더에서 include 되는 곳이 0건이라, 태그 추가의 재빌드 파급이 헤더 체인을 타고 번지지 않는다.
  - 태그 doc-comment 의 주요 주장은 실제 코드와 일치한다 — `State.LockedOn` 의 로컬 loose 태그(`WxAbilityTask_LockOnTarget.cpp:184`), `Damage.CanGuard` 가 없으면 일반·퍼펙트 가드가 둘 다 뚫린다(`WxEffect_Damage.cpp:93`~`:118`), `SetByCaller.RawDamage` 의 ATK/DEF/크리 우회(`WxEffect_Damage.cpp:185`~`:212`), `Event.Hit` 부모 매칭 구독(`WxAbility_Guard.cpp:106` 이 `bOnlyMatchExact=false`), `Effect.SuperArmor` 가 HitReact 만 막는다(`WxAbility_HitReact.cpp:31`).
  - `ECC_WxAttack = ECC_GameTraceChannel1` 은 `Config/DefaultEngine.ini:39` 의 `Name="WxAttack"` 과 일치하고, 주석의 "메시 Overlap / 캡슐 Ignore"(`Source/WxGame/Character/WxCharacterBase.cpp:25`,`:29`)와 "투사체는 WxProjectile 프리셋"(`WxProjectileBase.cpp:24`)도 코드와 일치한다.
  - `IWxSavable` 헤더가 서술하는 계약 3가지가 WxSave 구현과 정확히 맞는다 — 무효 GUID 는 저장·복원에서 제외(`WxSaveWorldSubsystem.cpp:312`,`:386`), 액터·컴포넌트가 전부 기본값이면 레코드를 지우고 트랜스폼도 함께 빠짐(`:352`~`:359`, `ShouldSave` 는 아키타입 비교), `OnSaveRestored()` 는 `OnWorldInitializedActors` 경로라 BeginPlay 이전에도 불릴 수 있음(`:450`, `:456`).
  - `AWxDevice`/`AWxDialogueActor` 의 `bInteractionEnabled` 가 복제되지 않는 것은 버그가 아니다 — 장치 ST 가 각 피어에서 돌아 같은 값에 수렴한다고 `WxDevice.h:82` 가 명시한다.
  - `FWxLocatorUtils` 호출 7곳 전부 `#if WITH_EDITOR` 안에 있고 소비 모듈 5개 모두 자기 `.Build.cs` 에 `UniversalObjectLocator` 를 선언하고 있어, WxCore 의 에디터 전용 Private 의존으로 충분하다.
  - `WxCollisionChannels.h:15` 의 `inline constexpr` 은 함수가 아니라 변수이므로 "인라인 함수 정의 금지" 규칙 대상이 아니다. WxCore 전체에 `FORCEINLINE`·람다·`BlueprintCallable`·델리게이트 콜백이 하나도 없어 관련 규칙은 검사할 대상 자체가 없다.

---
*문서 기준 커밋 `e54feda9` · 리뷰일 2026-08-27 · 소스 11파일 — `/module-review`로 갱신*
