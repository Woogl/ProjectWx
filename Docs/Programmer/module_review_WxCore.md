# WxCore — 코드 리뷰

> 11파일 중 실행 로직은 에디터 전용 표시명 헬퍼 하나뿐이고 나머지는 전부 선언·상수·인터페이스 계약이라, foundation 모듈로서 여전히 얇고 건강하다. 소스 11파일을 모두 통독했고, 태그 111개의 선언↔정의 짝·심볼↔문자열 표기 일치·프로젝트 전체(C++·`*.uasset`/`*.umap`·`Config`) 참조 여부를 전수 대조했으며, 태그 doc-comment가 지목하는 클래스·발행자·소비자 14종을 실제 코드와 하나씩 맞춰 봤다. 직전 리뷰(`6ea7624`) 이후 태그가 110→111개로 늘었고(`Damage.Attack` 추가), `State.Minion.Active`의 서술이 코드와 어긋난 새 문제가 생겼다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 1 |

## 결과

### 1. 🟡 `State.Minion.Active` 주석이 발행자가 없다고 말하지만 실제로는 발행된다
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:21`
- **범주**: 버그/정확성
- **문제**: 주석이 "소환 어빌리티와 명령 어빌리티가 한 슬롯을 나누려고 선언해 둔 태그. 아직 발행하는 쪽도 읽는 쪽도 없다"라고 단언하지만, 발행자는 실재한다 — `Source/WxGame/Character/WxEnemyCharacter.cpp:38`이 이 태그를 `MasterStateTag` 기본값으로 잡고, `:57`이 소환물 `BeginPlay`에서 주인 ASC에 `AddLooseGameplayTag(..., EGameplayTagReplicationState::TagOnly)`로 올리며, `:220`이 사망·`EndPlay` 경로에서 내린다. 커밋 `15d8806e`("소환물 상태 태그 발행을 소환물 자신에게 이관")가 발행 주체를 `UWxMinionSubsystem`에서 소환물로 옮겼는데, 이 헤더의 서술만 그 이관을 반영하지 못했다(HEAD의 옛 주석은 서브시스템을 가리키고 있었고, 작업 트리의 새 주석은 아예 발행자가 없다고 적었다). 사실인 부분은 읽는 쪽이 없다는 절반뿐이다 — 태그 문자열은 C++·에셋·Config 어디에서도 조회되지 않는 write-only 상태다. `WxGameplayTags.h`는 README가 "다른 모듈을 읽기 전 태그 구조부터 파악하라"고 지목한 프로젝트 어휘 사전이라, 이 한 줄을 믿은 다음 세션이 태그와 함께 `AWxEnemyCharacter`의 발행 코드까지 죽은 것으로 보고 걷어낼 위험이 실재한다.
- **제안**: 주석을 현재 구조대로 고친다 — 발행은 소환물 자신이 주인 ASC에 카운트로 올린다(`AWxEnemyCharacter::MasterStateTag`), 읽는 쪽은 아직 없고 슬롯 분기는 `.claude/worklog/2026-09-05-분신-소환-명령-슬롯-분기.md` 기준으로 보류 상태다. 발행/소비 상태를 함께 적어 두면 나중에 어느 쪽이 비어 있는지가 한 줄로 읽힌다.
- **확신도**: 높음

### 2. 🟢 어디에서도 참조되지 않는 태그 6개 (직전 세 리뷰에서 이월)
- **위치**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h:190`, `:214-218` (정의는 `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp:93`, `:113-117`)
- **범주**: 중복/복잡도
- **문제**: `Ability.Skill.2`, `Ability.Pattern.5` ~ `Ability.Pattern.9` 여섯 개는 C++(`WxGameplayTags::` 심볼)에서도, `Content`·`Plugins/*/Content`의 에셋 문자열에서도, `Config`에서도 참조가 0건이다. 태그 111개 중 나머지 105개는 모두 코드나 에셋에 실제 참조가 있다. 같은 축의 `Ability.Skill.1`·`.3`·`.4`와 `Ability.Pattern.1`~`.4`는 에셋에 있으므로, 연속된 슬롯 표에서 이 여섯 자리만 비어 있는 모양이다. 직전 세 리뷰(`491dd7ec`, `303d8d7f`, `6ea7624`) 시점과 동일한 여섯 개로, 그동안 늘지도 줄지도 않았다.
- **제안**: 슬롯 예약이 맞다면 그대로 두되 "슬롯 예약, 아직 구현 없음" 한 줄을 남긴다. 예약 의도가 아니면 지운다 — 어빌리티 식별 태그는 대부분 에셋 쪽에서 소비되므로, 미사용이 쌓이면 슬롯 표에서 어디까지가 실재인지 코드만 봐서는 구분되지 않는다.
- **확신도**: 낮음(의도된 설계일 수 있음)

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`, `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`, `Plugins/WxCore/Source/WxCore/Private/WxLocatorUtils.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h`, `Plugins/WxCore/Source/WxCore/WxCore.Build.cs`
- **훑은 파일**: `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h`, `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h`, `Plugins/WxCore/Source/WxCore/Private/WxInteractable.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxUIData.h`, `Plugins/WxCore/Source/WxCore/Private/WxUIData.cpp`, `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h`, `Plugins/WxCore/Source/WxCore/Private/WxCoreModule.cpp`, `Plugins/WxCore/WxCore.uplugin`, `Plugins/WxCore/README.md`
- **확인했고 문제 없었던 것**:
  - 태그 111개가 헤더 선언·cpp 정의 1:1로 짝을 이루고(누락·잉여 0), 문자열 중복도 없으며, 심볼명을 `_`→`.`로 치환한 값이 정의 문자열과 111개 전부 정확히 일치한다(오타 0). 단일 출처 규약도 지켜진다 — WxCore 밖에서 `UE_DECLARE_GAMEPLAY_TAG_EXTERN`/`UE_DEFINE_GAMEPLAY_TAG`을 쓰는 곳이 없고, `Config`에 `GameplayTagList`·태그 테이블 항목도 없다.
  - 이번에 추가된 `Damage.Attack`이 실제 구현과 맞는다. 주석이 말하는 "표식 없는 피해는 반응도 플로터도 없다"는 `WxEffectComponent_DamageResponse.cpp:22`의 게이트(`GetDynamicAssetTags().HasTag(Damage_Attack)`) 그대로이고, 표식은 `WxDamageTableRow.cpp:25`가 붙인다.
  - 나머지 태그 doc-comment가 지목한 클래스·경로가 전부 실재한다 — `WxExecCalc_Damage`·`WxEffect_MoveSpeedScale`·`WxEffect_NoCooldown`·`WxEffect_InfiniteMP`·`WxEffect_Cooldown`·`WxHitStopComponent`·`WxAbility_Interact`·`WxEffect_{Exhaust,SuperArmor,Invincible,GuardReduction,PerfectGuard,HitStop}`·`WxMinionSubsystem`. `SetByCaller.Duration`이 서술한 세 소비처(`WxEffect_HitStop.cpp:14`, `WxEffect_InfiniteMP.cpp:12`, `WxEffect_NoCooldown.cpp:13`)와 `SetByCaller.Coeff.ATK`(`WxEffect_Damage.cpp:150`), `Effect.HitStop`↔`WxHitStopComponent.cpp:25,44,62`, `Event.Hit`/`Event.Hit.GuardBreak`/`Event.DamageDealt` 발행부(`WxEffectComponent_DamageResponse.cpp:56-79`)도 서술과 일치한다.
  - `ECC_WxAttack`(`WxCollisionChannels.h:15`)과 `Config/DefaultEngine.ini:39`의 `ECC_GameTraceChannel1`/`Name="WxAttack"`/`DefaultResponse=ECR_Block` 짝이 정확히 일치하고, doc-comment가 서술하는 메시 Overlap·캡슐 Ignore override는 `WxCharacterBase.cpp:27,31`, `"WxProjectile"` 프리셋은 `Config/DefaultEngine.ini:40`에 실재한다.
  - 규칙 위반 없음: `Wx` prefix, 인라인 함수 정의 없음(`ECC_WxAttack`은 `inline constexpr` **상수**라 규칙 6과 무관), `BlueprintCallable` 없음, 람다 없음, 델리게이트 콜백 없음. 인터페이스 기본 구현 두 개(`IWxInteractable::CanInteract`, `IWxUIData::GetMaxRecharges`)는 모두 cpp에 내려가 있고, 그에 맞게 U-class는 `MinimalAPI`, I-class는 `WXCORE_API`로 짝이 맞다. 12개 소스(`*.h`/`*.cpp`/`*.cs`) 전부가 BOM 없이 `// Copyright Woogle. All Rights Reserved.`로 시작한다.
  - 공용 계약 두 개 모두 살아 있다 — `IWxInteractable`은 `AWxEnemyCharacter`·`AWxNpc`·`AWxDialogueActor`·`AWxItemPickup`·`AWxDevice`가 구현하고 `WxInteractionScannerComponent`·`WxAbility_Interact`가 소비한다. `IWxUIData`는 `UWxAbilityBase`·`UWxEffectComponent_Table`이 구현하고 `WxViewModel_Ability`·`WxViewModel_AbilitySystem`·`WxViewModel_Effect`·에디터 썸네일 렌더러가 소비한다.
  - `FWxLocatorUtils` 소비처 7곳(`WxActorLocatorCustomization.cpp:143`, StateTree 노드 `GetDescription()` 5곳, `WxSpawnerLocatorUtils.cpp:29`의 컴파일 검증)이 전부 에디터 경로라 `SyncFind`·`FText` 왕복이 핫패스에 놓이지 않는다.
  - WxCore는 다른 Wx 플러그인을 참조하지 않는다 — `Core`/`CoreUObject`/`Engine`/`GameplayTags`에 에디터 전용 `UniversalObjectLocator`(엔진 런타임 모듈, 플러그인 아님)뿐이고, `WxCore.uplugin`에 `Plugins` 항목이 없다. `Build.cs`의 `bBuildEditor` 블록과 `WxLocatorUtils.h`의 `#if WITH_EDITOR` 구간도 정합하며, Content 폴더가 없어 에셋 역방향 의존도 성립하지 않는다.
  - 빈 `FWxCoreModule`(`WxCoreModule.h:8-13`, `WxCoreModule.cpp:6-9`)은 다른 Wx 플러그인이 모두 똑같이 쓰는 프로젝트 공통 보일러플레이트라 WxCore만 지적할 사안이 아니다.
- **미검토 / 한계**:
  - 리뷰 대상은 작업 트리 현재 상태다. `WxGameplayTags.{h,cpp}`를 포함해 커밋되지 않은 변경이 여럿 있어(`Damage.Attack` 추가, `State.Minion.Active` 주석 수정, WxCombat 대미지 파이프라인 개편), 이 문서의 라인 번호는 커밋 `53bc7de6` 자체가 아니라 그 위의 작업 트리 기준이다.
  - 태그의 에셋 참조 여부는 `.uasset`/`.umap` 바이너리 문자열 검색으로 판정했다 — 이름 테이블에 남지 않는 형태로 참조되는 태그가 있다면 2번 발견이 위양성일 수 있다. BP/WBP·GE·StateTree 에셋의 내부 구조는 범위 밖이다.
  - 엔진 소스가 이 환경에 없어 `FUniversalObjectLocator::SyncFind`가 동기 로드를 유발하지 않는다는 점은 직전 리뷰(엔진 5.8 대조)의 결론을 그대로 승계했고 이번에 재검증하지 못했다.

---
*문서 기준 커밋 `53bc7de6` · 리뷰일 2026-09-08 · 소스 11파일 — `/module-review`로 갱신*
