# WxCore — 코드 리뷰

> 선언만 담는 foundation이라 실행 결함은 없고, 의존 규칙·저작권·인라인 금지 규칙도 전부 지켜져 있다. 남는 문제는 전부 "계약 문구와 실제 이행이 어긋난 지점"이다. 커버리지: 소스 13개 전부와 `WxCore.Build.cs`·`WxCore.uplugin`·`README.md`를 읽고, 세 인터페이스의 구현체·호출부와 태그 118개의 C++·에셋 참조를 저장소 전역에서 대조했다. 이전 리뷰(`5eb1a754`)의 발견 1·2는 대상 파일(`Minion/WxMinion.h`·`.cpp`)이 모듈에서 사라져 소멸했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 3 |

## 결과

### 1. 🟡 `IWxSpawnable`을 `MustImplement`로 강제하는 소환 경로가 정작 `OnSpawnedBy`를 부르지 않는다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxSpawnable.h:15`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp:67-83`
- **범주**: 설계/구조
- **문제**: 헤더가 밝히는 계약은 "스폰 주체를 아는 픽커가 `MustImplement`로 이 계약을 강제한다"이고, 그 강제가 곧 `OnSpawnedBy` 호출을 보장한다는 뜻으로 읽힌다. 두 픽커 중 한쪽만 그렇다.
  - `AWxSpawner`는 계약대로다 — `ImplementsInterface` 검사(`Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:124`) 후 `FinishSpawning` 앞에서 호출한다(:148).
  - `UWxMinionSubsystem::SpawnMinion`은 똑같이 Deferred Spawn을 쓰면서(`:67`) 팀만 심고 바로 `FinishSpawning`한다(`:83`). `OnSpawnedBy`는 어디에도 없다. 그런데 이 경로의 픽커 두 곳은 `MustImplement = "/Script/WxCore.WxSpawnable"`로 계약을 강제한다(`Plugins/WxCombat/Source/WxCombat/Public/AnimNotify/WxAnimNotify_SpawnMinion.h:29`, `WxAnimNotify_DespawnMinion.h:24`).

  즉 소환물 쪽에서 인터페이스는 실질적으로 타입 필터로만 쓰이고 있다. 지금은 유일한 구현체 `AWxEnemyCharacter::OnSpawnedBy`가 `OwningSpawner`만 채우므로(`Source/WxGame/Character/WxEnemyCharacter.cpp:87-90`) 소환물에는 채울 값이 없어 증상이 없다. 하지만 이후 누가 이 콜백에 "태어날 때 주인에게서 받아야 하는 값"을 추가하면, 스포너로 태어난 개체에만 적용되고 소환물에는 조용히 빠진다.
- **제안**: 둘 중 하나로 맞춘다. (a) `SpawnMinion`이 `FinishSpawning` 앞에서 `Cast<IWxSpawnable>`로 주인을 넘긴다 — 헤더가 약속한 "빙의/BeginPlay 이전" 타이밍도 그대로 지켜진다. (b) 소환 경로가 이 계약을 쓰지 않기로 하면 두 노티파이의 `MustImplement`를 걷고 실제 필요한 베이스(`AWxEnemyCharacter` 등)로 좁힌다. `DespawnMinion` 쪽은 애초에 스폰 계약과 무관하다.
- **확신도**: 높음(호출 부재는 확인, 어느 쪽으로 맞출지는 설계 판단)

### 2. 🟡 `IWxInteractable::OnInteracted`·`GetInteractionPrompt`에 호출 맥락 계약이 없다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:36`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:38`
- **범주**: 설계/구조
- **문제**: 바로 위 `CanInteract`만 호출 맥락을 밝히고(`:30-33`), 나머지 둘은 선언만 있다. 실제 규칙은 소비 코드에만 있다.
  - `OnInteracted`의 유일한 호출부는 서버 전용 어빌리티다. `CanInteract`(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:73`)와 사거리 검증(`:79`)을 통과한 뒤 아바타를 넘긴다(`:84`).
  - `GetInteractionPrompt`는 로컬 스캐너만 부른다(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:80`).

  그래서 구현체마다 계약을 따로 추측해 같은 문장을 자기 주석에 다시 쓰고 있다 — `AWxItemPickup`은 "서버 권위에서만"을 재기술하고(`Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp:76`), `AWxDevice`는 권위와 `CanInteract`를 한 번 더 검사하며(`Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp:26`, `:37`), `AWxEnemyCharacter`는 "프롬프트는 로컬 표시"를 전제로 `GetPlayerPawn(this, 0)`에 기댄다(`Source/WxGame/Character/WxEnemyCharacter.cpp:130-133`). 새 구현체가 `OnInteracted`에 클라 로직을 넣으면 스탠드얼론·리슨 서버 호스트에서는 정상으로 보이고 원격 클라에서만 조용히 빠진다.
- **제안**: 각 함수에 한 줄씩 붙인다. `OnInteracted` — "서버에서 `CanInteract`·사거리 검증을 통과한 뒤에만 호출된다. `Interactor`는 아바타 폰이다." `GetInteractionPrompt` — "로컬 표시용으로만 호출된다." 계약이 박히면 구현체들의 중복 재기술과 `AWxItemPickup`의 도달 불가 분기(`:88-89`)를 걷을 수 있다.
- **확신도**: 중간

### 3. 🟢 `IWxSpawnable::OnSpawnedBy`가 서버에서만 불린다는 사실이 헤더에 없다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxSpawnable.h:21-22`
- **범주**: 설계/구조
- **문제**: 주석은 호출 **순서**(`FinishSpawning` 이전)만 말하고 호출 **머신**은 말하지 않는다. 실제로 두 스폰 경로 모두 권위 게이트 뒤에 있다(`Plugins/WxWorld/Source/WxWorld/Private/Spawnable/WxSpawner.cpp:109`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp:23`). 그래서 이 콜백에서 세운 상태는 복제하지 않는 한 클라에 없다. 현재 유일한 구현체가 채우는 `OwningSpawner`는 비복제 `TWeakObjectPtr`이고(`Source/WxGame/Character/WxEnemyCharacter.h:90`) 읽는 곳이 `HasAuthority()` 뒤라(`WxEnemyCharacter.cpp:160`, `:165`) 지금은 문제가 없다. 다만 헤더만 읽는 다음 구현체는 여기서 클라 표시용 값을 잡아도 된다고 읽는다.
- **제안**: 주석에 "스폰하는 머신(= 서버)에서만 불린다 — 클라가 알아야 하는 값이면 복제하라"를 한 줄 더한다.
- **확신도**: 중간

### 4. 🟢 `Ability.*` 구획 주석의 "플레이어 캐릭터 전용"이 실제 사용과 어긋난다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:190`, `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:220`
- **범주**: 설계/구조
- **문제**: `BP_Minion`의 부모는 `AWxEnemyCharacter`인데(`Content/Character/Minion/BP_Minion.uasset` 이름 테이블), 그 어빌리티들이 "플레이어 캐릭터 전용" 구획의 태그를 쓴다 — `GA_Minion_Attack_Heavy`는 `Ability.Attack.Heavy`·`Ability.Attack.Light`를, `GA_Minion_Skill_2`는 `Ability.Skill.2`를 참조한다. README가 이 헤더를 "프로젝트 게임플레이 계약의 색인"으로 지목하므로, 구획 이름을 캐릭터 클래스 제한으로 읽은 사람은 현재 구성을 오용으로 오판한다.
- **제안**: 구획 이름을 캐릭터 클래스가 아니라 킷 기준으로 고친다(예: "플레이어 킷 — 플레이어와 그 킷을 쓰는 소환물", "적 패턴 킷").
- **확신도**: 중간

### 5. 🟢 "네이티브 태그는 여기에만 선언한다" 규약이 이미 깨져 있다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:7`, `Plugins/WxCore/README.md:45`
- **범주**: 설계/구조
- **문제**: 헤더 첫 줄은 "태그 추가 시 이 파일과 `WxGameplayTags.cpp`에만 작성", README는 "다른 곳에서 Native Tag를 선언하지 않는다"고 못 박는다. 그런데 `WxCombat`이 `WxCombatGameplayTags` 네임스페이스로 자기 태그를 선언·정의한다(`Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_IgnoreAbilityTags.h:10-12`, `.../Private/AbilitySystem/Effect/WxEffect_IgnoreAbilityTags.cpp:9`). 형태가 잘 갖춰진 별도 네임스페이스인 걸 보면 실수보다는 의도된 예외로 보이는데, 그렇다면 WxCore 쪽 문구가 사실이 아닌 상태로 남아 다음 사람이 어느 쪽을 따를지 알 수 없다. (`Config/`에는 `DefaultGameplayTags.ini`가 없어 ini 선언은 없다 — 선언처는 이 둘뿐이다.)
- **제안**: 한쪽으로 맞춘다. 도메인 안에서만 쓰는 태그는 그 모듈이 선언해도 된다면 헤더 7행과 README 45행을 "도메인 밖으로 나가는 태그만 WxCore"로 고치고, 아니면 `Effect.IgnoreAbilityTags`를 `WxGameplayTags`로 옮긴다.
- **확신도**: 중간(WxCombat 쪽이 의도된 예외일 수 있음 — 그렇다면 고칠 곳은 WxCore 문구다)

## 검토 범위
- **깊게 본 파일**:
  - `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp` — 선언 118개와 정의 118개의 식별자 집합이 정확히 일치하고 `_`→`.` 변환도 태그 문자열과 맞는다(스크립트 대조). 태그별 C++ 참조를 집계하고, 참조 0인 태그는 `Content/`·플러그인 `Content/`·`Config/`의 이름 테이블까지 확인했다. `Event.Hit.Parry`·`.GuardBreak`의 계층은 GAS `HandleGameplayEvent`의 부모 전파(`UE_5.8/.../AbilitySystemComponent_Abilities.cpp:2564-2585`)와 소비 측 분기(`WxAbility_HitReact.cpp:33`, `:41`, `WxAbility_GuardReact.cpp:30`, `:72`)를 대조해 의도된 설계임을 확인했다.
  - `Plugins/WxCore/Source/WxCore/Public/WxSpawnable.h`, `Private/WxSpawnable.cpp` — 구현체(`AWxEnemyCharacter`)와 두 스폰 경로(`AWxSpawner`, `UWxMinionSubsystem`), 세 `MustImplement` 픽커를 전부 봤다. BP 구현 가능성은 엔진의 `FKismetEditorUtilities::IsClassABlueprintImplementableInterface`(`UE_5.8/.../Kismet2/Kismet2.cpp:2706-2740`)로 확인했다 — `BlueprintEvent` UFUNCTION이 없어 BP는 이 인터페이스를 구현할 수 없고, 따라서 `Cast<IWxSpawnable>`이 null이 되는 경로는 없다(`CannotImplementInterfaceInBlueprint` 누락은 무해하다).
  - `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Private/WxInteractable.cpp` — 구현체 6곳과 호출부 2곳을 대조했다. 호출부 둘 다 `Interactor`가 null이 아님을 보장하므로 문서화되지 않은 널 계약이 현재 깨지는 지점은 없다.
  - `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp` — `SyncFind` 컨텍스트 없는 해석 경로를 엔진 구현(`UE_5.8/.../ActorLocatorFragment.cpp:76-145`)과 대조했다. 에디터에서는 `FSoftObjectPath::ResolveObject` 폴백으로 풀리므로 표시명 용도에 문제가 없고, 호출부 7곳이 전부 `WITH_EDITOR` 안이라 비에디터 빌드 링크도 깨지지 않는다.
  - `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` — `Config/DefaultEngine.ini:39-40`의 `ECC_GameTraceChannel1 = WxAttack` 바인딩, 그리고 주석이 말하는 메시 Overlap·캡슐 Ignore override(`Source/WxGame/Character/WxCharacterBase.cpp:26`, `:30`)와 대조했다. 주석과 실제가 일치한다.
  - `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin` — Wx 플러그인 의존이 0이다(엔진 모듈 4개 + 에디터 빌드 한정 `UniversalObjectLocator`). 전 파일 첫 줄이 저작권 문구이고, 헤더에 인라인 함수 정의가 없다(`inline constexpr` 변수 1개뿐이며 함수가 아니다).
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`·`Private/WxUIData.cpp`(순수 가상 3 + 기본값 1, 결함 없음), `Public/WxCoreModule.h`·`Private/WxCoreModule.cpp`(빈 Startup/Shutdown), `Public/WxLocatorUtils.h`, `Plugins/WxCore/README.md`
- **미검토 / 한계**:
  - 정적 리뷰다 — 빌드·PIE를 돌리지 않았다. BP·에셋 내부는 범위 밖이고, 에셋 근거는 패키지 이름 테이블에 문자열이 있는지까지만 봤다.
  - 발견으로 싣지 않은 판단 3가지.
    - 예약 슬롯 태그 `Ability.Skill.4`, `Ability.Pattern.4`~`.9`는 C++·에셋 어디에도 참조가 없다. 이전 리뷰가 슬롯 확장용 사전 정의로 보고 제외했고, 그 판단을 유지한다.
    - `Source/WxGame/Character/WxNpc.cpp:40-43`의 `CanInteract`는 서버에만 있는 대기 등록부(`Plugins/WxWorld/.../WxStateTreeTask_WaitForInteraction.cpp:34-36`의 모듈 정적 레지스트리)로 답해서, `WxInteractable.h:31-33`이 약속한 "클라 표시 게이트와 서버 검증이 같은 답"에서 벗어난다. 구현체가 스스로 "서버가 곧 클라인 전제"라 적어 둔 사항이라 WxGame 리뷰 몫으로 남겼다.
    - `UI.Layer.GameMenu`는 레이어 등록 외에 푸시하는 곳이 없다. WxUI 소관이라 제외했다.

---
*문서 기준 커밋 `fe57e17a` · 리뷰일 2026-09-20 · 소스 13파일 — `/module-review`로 갱신*
