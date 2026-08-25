# WxCore — 코드 리뷰

> 9파일·504줄의 얇은 foundation 모듈이고 코드 자체는 여전히 깨끗하다. 네이티브 태그 102개가 `.h` 선언 ↔ `.cpp` 정의로 빠짐없이 짝을 이루고, 심볼명(`_`→`.`)과 태그 문자열이 102건 전부 일치하며 중복 문자열도 0건이다. 지난 리뷰의 잔재 지적(`Trait.*` 주석·`IsInteractionEnabled` 오기·죽은 `Gimmick.*` 리다이렉트)은 모두 정리된 것을 확인했고, 남은 지적은 태그 카탈로그의 양방향 대조에서 새로 드러난 두 건과 계약 인터페이스 설계 한 건이다. 이번 리뷰는 `*.Build.cs`·`.uplugin`·전 헤더·전 cpp를 통독했고, 태그 102개를 C++ 심볼 참조와 `Content/`·`Plugins/*/Content/` 패키지 문자열 양방향으로 기계 대조했으며, 두 계약 인터페이스의 구현체 5종과 소비처(WxWorld 스캐너·ST 태스크, WxGame 상호작용 어빌리티, WxSave 서브시스템)를 따라가 계약 준수를 확인했다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 2 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 `State.Groggy`가 카탈로그에 없는데 살아 있는 위젯이 이 태그로 그로기 표시를 건다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:10-22` (State 블록에 선언 없음); 소비처 `Content/UI/Widget/WBP_Nameplate_Enemy.uasset` (패키지에 `(TagName="State.Groggy")` 상주), 그 위젯을 쓰는 `Content/Character/Enemy/BP_Enemy.uasset`·`Content/Character/Sandbag/BP_Sandbag.uasset`
- **범주**: 버그/정확성
- **문제**: `State.Groggy`는 프로젝트 어디에도 선언이 없다 — WxCore 헤더/cpp의 102개에 없고, C++ 어느 파일도 그 문자열을 쓰지 않으며, `Config/DefaultGameplayTags.ini`는 리다이렉트 삭제 후 섹션 헤더 한 줄만 남아 `GameplayTagList` 항목이 0개다(태그 소스는 WxCore 네이티브 선언이 유일). 그런데 적 네임플레이트 위젯은 `Groggy`/`GroggyDim` 비주얼을 이 태그로 켜도록 저작돼 있고, 이 위젯은 `BP_Enemy`·`BP_Sandbag`가 실제로 물고 있다. 등록되지 않은 태그는 태그 매니저가 로드 시 `Invalid GameplayTag ... found in property` 로 알리고 무효로 취급하므로, 그로기 네임플레이트 표시는 어떤 경우에도 켜지지 않는다. 코드가 그로기를 표현하는 태그는 `Ability.Groggy`이고(`Source/WxGame/Character/WxEnemyCharacter.cpp:125`·`:161`가 `WxGameplayTags::Ability_Groggy`로 앞잡/뒤잡을 가른다), `Ability.Groggy`를 참조하는 에셋은 하나도 없다 — 즉 위젯이 옛 이름을 든 채 남았거나, 카탈로그에서 `State.Groggy`가 지워질 때 소비처가 함께 이관되지 않았다.
- **제안**: 둘 중 하나로 정한다. (a) 그로기 표시를 어빌리티 활성 태그로 읽는 것이 맞다면 위젯의 태그 값을 `Ability.Groggy`로 바꾼다(카탈로그 변경 없음). (b) 표시용 상태 태그를 따로 두는 것이 맞다면 `State_Groggy`를 `WxGameplayTags.h`/`.cpp` 짝에 추가하고 부여/제거 주체(`WxAbility_Groggy`)를 명시한다. 어느 쪽이든 "선언은 이 헤더와 짝 cpp에만"이라는 규약상 지금처럼 에셋에만 존재하는 상태로 두면 안 된다.
- **확신도**: 높음 (미선언·미참조는 코드·ini·전 패키지 문자열 대조로 확인. 위젯이 이 태그를 정확히 어떤 조건에 쓰는지는 BP 내부라 리뷰 범위 밖이며, 표시가 죽는다는 결론은 태그 무효화 동작에 근거한다)

### 2. 🟡 `IWxInteractable::CanInteract()`에 주체 인자가 없어 서버 권위 검증이 주체별로 정확하지 않다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h:31`
- **범주**: 설계/구조
- **문제**: `CanInteract()`는 인자가 없어 "누가" 상호작용하려는지를 계약이 전달하지 못한다. 그런데 이 함수는 클라 표시 게이트(`Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp:180`)이자 **서버 권위 자격 검증**(`Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp:75`)의 단일 소스다. 주체 상대 자격이 필요한 구현체는 주체를 스스로 추측할 수밖에 없어, `AWxEnemyCharacter::CanInteract()`의 뒤잡 후방 원뿔 판정이 `UGameplayStatics::GetPlayerPawn(this, 0)`으로 0번 플레이어를 조회한다(`Source/WxGame/Character/WxEnemyCharacter.cpp:167` → `:74-92`). 싱글·리슨호스트에서는 정확하지만 데디케이티드 멀티에서는 2번째 이후 플레이어의 뒤잡을 서버가 0번 플레이어 위치로 판정한다 — 자기 뒤에 아무도 없는 적을 뒤잡하거나, 정당한 뒤잡이 거부될 수 있다.
- **제안**: 계약을 `CanInteract(const AActor* Interactor) const`로 되돌린다. 발동 경로는 이미 같은 함수 스코프에 아바타를 들고 있고(`WxAbility_Interact.cpp:61`의 `Avatar`, `:86`에서 `OnInteracted`에 넘기는 그 값), 표시 경로는 로컬 폰을 넘기면 되어 두 머신의 답이 같아진다. 데디케이티드 멀티를 목표로 하지 않기로 확정했다면 반대로 헤더에 "주체 상대 자격은 계약이 지원하지 않는다"를 명시해 다음 구현체가 같은 추측을 되풀이하지 않게 한다.
- **확신도**: 낮음(의도된 설계일 수 있음) — 인터페이스 단순화의 대가로 `.claude/worklog/2026-08-22-적-후방-백스탭-복원.md`의 「후속 과제」에 이미 남겨 둔 항목이다. 여기 적는 이유는 그 후속 과제가 사실상 WxCore 계약 변경이라서다.

### 3. 🟢 `SetByCaller.Recovery.UP`/`.MP`는 소비처가 0인 데드 태그이고, 주석이 존재하지 않는 GE를 지목한다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:198-202`, 정의 `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:111-112`
- **범주**: 중복/복잡도
- **문제**: 이 두 태그는 C++ 참조 0건, 패키지 문자열 0건으로 프로젝트 전체에서 소비처가 없다(같은 조건의 나머지 미참조 태그는 전부 에셋이 소비하는 Device·Cue·Ability 식별 태그다). 게다가 두 주석이 지목하는 `WxEffect_RecoverResource`는 클래스로도 에셋으로도 존재하지 않는다 — `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/`에 GE 20종이 있으나 회복 계열은 `WxEffect_HealPercent`(HP)·`WxEffect_RegenSP`(SP)뿐이고, 리포지터리 전체에서 `RecoverResource` 문자열의 유일한 히트가 이 헤더 자신이다. UP/MP 어트리뷰트 자체는 살아 있어(`WxCombatAttributeSet.h:70`·`:78`) 태그만 먼저 깔아 둔 것으로 보이지만, 이 헤더는 도메인 간 프로토콜의 사전이라 "쓰이는 키"와 "예약 키"가 섞이면 다음 사람이 없는 GE를 찾아 헤맨다.
- **제안**: 두 선언·정의를 지우고 UP/MP 회복 GE를 실제로 만들 때 다시 넣는다. 예약으로 남길 거라면 주석을 "아직 소비처 없음(회복 GE 미구현)"으로 바꿔 없는 클래스를 지목하지 않게 한다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxSavable.h`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxSavable.cpp`, `Plugins/WxCore/README.md`, `Config/DefaultGameplayTags.ini`, `Config/DefaultEngine.ini`, `Config/DefaultGame.ini`
- **확인했으나 문제 없던 항목**:
  - **태그 정합**: 선언 102개 ↔ 정의 102개 완전 일치(양쪽 차집합 0). 심볼명의 `_`를 `.`로 바꾼 값이 태그 문자열과 102건 모두 일치하고, 중복 심볼·중복 문자열도 0건이다. `WxGameplayTags::` 접두 참조를 기계 수집해 대조한 결과 **C++에서 쓰이나 선언되지 않은 태그는 0건**이다.
  - **직전 리뷰 지적의 해소**: `Trait.*` 잔재는 헤더 주석(`WxGameplayTags.h:151`이 이제 `EWxAbilityActivationGroup`을 지목)·README·GA 에셋 모두에서 사라졌고, `WxInteractable.h:39`·`WxItemPickup.cpp:29` 주석은 `CanInteract`로 정정됐으며, `Config/DefaultGameplayTags.ini`는 죽은 `Gimmick.*` 리다이렉트가 삭제되어 섹션 헤더 한 줄만 남았다.
  - **데드 태그(의도된 것)**: C++ 미참조 38개 중 36개는 설계상 에셋 전용 소비 태그로 실제 소비 패키지를 확인했다 — `Device.*` 15개(ST_Button·ST_Door·ST_Elevator 등 StateTree), `Event.Device.Triggered`(BP_ButtonDevice 외 4), `GameplayCue.AttackTelegraph.*` 4개(GC/AM 에셋), `Ability.*` 식별 태그 16개(GA·BT·WBP_PlayerSkills). `Ability.Pattern.6`~`9`는 코드·에셋 모두 0건이지만 슬롯 번호 예약이라 데드로 보지 않았다(런타임 태그 문자열 조립 코드는 프로젝트에 없음을 확인).
  - **미선언 태그 역방향 대조**: 전 패키지에서 프로젝트 루트 네임스페이스 모양의 문자열을 뽑아 카탈로그와 차집합을 냈고, 바이너리 노이즈를 제외하면 실제 미선언 소비는 발견 1의 `State.Groggy` 한 건뿐이다.
  - **헤더 주석의 사실성**: 태그 주석이 지목하는 부여·소비 주체를 표본 대조했고 `SetByCaller.Recovery.*`(발견 3)를 뺀 전부가 실제 코드와 일치했다 — `State.Dialogue`(`WxDialogueSessionComponent.cpp:148`/`:206` 발행 ↔ `WxAbility_Interact.cpp:37` ActivationBlockedTags), `State.LockedOn`(`WxAbilityTask_LockOnTarget.cpp:184` 로컬 loose 부여), `State.InCombat`(`WxAIPerceptionComponent.cpp:116` 서버 MinimalReplication), `State.BeingFinished`(`WxAbility_Finisher.cpp:79` 권위 발행), `Movement.InAir`(`WxCharacterMovementComponent.cpp:80`), `Ability`(`WxAbility_Death.cpp:23` BlockAbilitiesWithTag) 등.
  - **GameplayCue 쿠킹**: 신규 `GameplayCue.GhostTrail` 포함 큐 에셋 9개가 전부 `/Game/AbilitySystem/Cue` 아래에 있고, `Config/DefaultGame.ini:21`의 `GameplayCueNotifyPaths`와 `:25`의 `DirectoriesToAlwaysCook`가 둘 다 그 경로를 덮는다.
  - **플러그인 경계**: `WxCore.Build.cs:11-17`은 Core/CoreUObject/Engine/GameplayTags만 참조하고, `WxCore.uplugin`에는 `Plugins` 의존 목록 자체가 없으며, 소스에 다른 Wx 모듈 `#include`가 하나도 없다. `Wx.uproject:39-40`에서 정상 활성. DAG 최하단 규칙 준수.
  - **코딩 규칙**: 10파일(cs 포함) 전부 첫 줄 Copyright 준수(`WxGameplayTags.h/.cpp`만 UTF-8 BOM이 앞서지만 UBT가 `/utf-8`을 넘겨 무해). 람다·델리게이트 콜백·`BlueprintCallable`·`FORCEINLINE`/인라인 함수 정의가 모듈 전체에 0건이고, `WxCollisionChannels.h:15`의 `inline constexpr`은 변수라 규칙 6 대상이 아니다. `Wx` prefix도 전 타입 준수.
  - **`IWxInteractable` 계약 준수**: 순수 가상 2개(`OnInteracted`/`GetInteractionPrompt`)를 구현체 4종(`AWxDialogueActor`·`AWxDevice`·`AWxItemPickup`·`AWxEnemyCharacter`)이 모두 채우고, 기본 구현 2개는 필요한 쪽만 override한다. `OnInteracted`의 유일한 호출처는 `CanInteract()` + 사거리 재검증을 지난 `WxAbility_Interact.cpp:86`이고, `AWxDevice::OnInteracted`는 `HasAuthority()`로 한 번 더 가른다(`WxDevice.cpp:56-60`).
  - **`IWxSavable` 계약**: 액터 전용 규약이 지켜지고(구현체는 `AWxDevice`·`AWxSpawner`), 헤더가 약속한 세 가지가 구현과 일치한다 — 무효 `GetSaveId()` 제외(`WxSaveWorldSubsystem.cpp:311-316`, `:385-390`), 전부 기본값이면 옛 레코드까지 걷어내고 트랜스폼도 함께 빠짐(`:352-357`), 복원 직후 `OnSaveRestored()` 호출(`:450`).
  - **콜리전 상수**: `ECC_WxAttack = ECC_GameTraceChannel1`이 `Config/DefaultEngine.ini:39`(`Name="WxAttack"`, `DefaultResponse=ECR_Block`, `bTraceType=False`)와 일치하고 — 등록된 `DefaultChannelResponses`가 이 한 줄뿐이라 순서 어긋남이 성립하지 않는다 — 헤더가 말하는 "메시 Overlap·캡슐 Ignore" 오버라이드가 `Source/WxGame/Character/WxCharacterBase.cpp:25`·`:29`에 실존하고, "투사체는 WxProjectile 프리셋"도 `DefaultEngine.ini:40`과 `WxProjectileBase.cpp`의 `SetCollisionProfileName`에서 확인된다.
  - **모듈 진입점**: `FWxCoreModule`의 Startup/Shutdown이 빈 구현인 것은 정상이다 — 네이티브 태그는 `UE_DEFINE_GAMEPLAY_TAG`의 정적 초기화가 등록하므로 모듈이 할 일이 없다.
- **미검토 / 한계**: 발견 1의 위젯 측 소비 형태(어떤 바인딩이 `State.Groggy`를 읽는지)는 BP 내부라 확인하지 않았고, 패키지 문자열과 `Groggy`/`GroggyDim` 심볼 존재까지만 대조했다. 발견 2의 데디케이티드 멀티 오판정은 코드 경로 추적으로만 확인했고 실제 멀티 세션에서 재현해 보지 않았다. 에셋 대조는 UTF-8/UTF-16 리터럴 문자열 검색 기준이라, 태그를 런타임에 조립하는 BP 노드가 있다면 놓칠 수 있다(C++ 쪽에는 그런 조립이 없음을 확인했다).

---
*문서 기준 커밋 `9d64349b` · 리뷰일 2026-08-25 · 소스 9파일 — `/module-review`로 갱신*
