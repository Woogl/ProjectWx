# WxCombat — 코드 리뷰

> 건강한 모듈이다. 직전 리뷰 이후 커밋(주석 정리·GE 컴포넌트 표시명 제거·AI 가드 수정)은 동작 구조를 바꾸지 않아 지적 5건이 그대로 남았고, 새로 생긴 문제는 없다. 프로젝트 규칙 위반도 이번에도 0건이다(모듈 의존 `WxCore` 단일, `Wx` 접두사, `Handle` 콜백 접두사, `BlueprintCallable` 1건이 Blueprint Function Library, 인라인 정의 0건, 람다 0건, 저작권 첫 줄 전 파일 충족). 이번 리뷰는 `Private/`의 cpp 83개를 모두 열어, 대미지 파이프라인(ExecCalc·AttributeSet·CombatLibrary·DamageTableRow)·어빌리티 16종·발동 그룹/캔슬 창·선입력 버퍼·무기/투사체 히트·락온·히트스톱·GE 21종·AnimNotify 10종·GameplayCue 6종·Targeting 9종·어빌리티 태스크 5종을 헤더와 함께 읽었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 크리티컬 난수를 예측 클라와 서버가 각각 굴린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:170`
- **범주**: 버그/정확성
- **문제**: `bIsCritical = FMath::FRand() < CritChance`는 시드를 공유하지 않는다. 공격 어빌리티가 `LocalPredicted`이고 `UWxCombatLibrary::ApplyDamage`가 예측 키를 실어 GE를 걸므로(`WxCombatLibrary.cpp:128`), 소유 클라의 `HasNetworkAuthorityToApplyGameplayEffect`가 통과해 실행 계산이 클라와 서버에서 각각 한 번씩 돈다. 두 결과가 갈리면 ① `FinalDamage`가 달라져 HP가 복제 도착 시 튀고 ② 같은 파일 207행의 `TargetSP <= FinalDamage` 판정까지 갈려 `Damage.GuardBreak` 부여 여부가 어긋나며(이 태그가 곧 `Event.Hit.GuardBreak` 라우팅을 정한다) ③ 197행 `SetCritical`이 실은 클라 플로터와 서버 값 사이에서 크리 표식을 불일치시킨다. 로컬 플레이만 하는 동안에는 드러나지 않는다.
- **제안**: 크리 판정을 권위에서만 굴려 컨텍스트에 싣고 예측 측은 그 값을 읽게 하거나(비크리 낙관 예측), 예측 키·스펙 식별자를 시드로 삼는 결정적 난수로 바꾼다.
- **확신도**: 중간(멀티플레이 정책이 보류 상태라 의도적으로 미룬 것일 수 있음)

### 2. 🟡 대미지 플로터가 피격 1회마다 액터+위젯을 새로 만든다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_DamageFloater.cpp:34`
- **범주**: 성능/안전
- **문제**: `Executed` 큐마다 `AWxDamageFloaterActor`를 스폰하고(34행) `InitWidget()`으로 UMG 위젯을 새로 구성한다(59-60행). 액터의 `InitialLifeSpan`이 5초라(52행) 다타·다수 적 상황에서 화면당 수십 개가 동시에 살아 있게 되며, 각각이 Screen-space `UWidgetComponent`를 들고 매 프레임 그려진다. 액터 스폰과 위젯 생성은 전투 핫패스에서 가장 비싼 축이다.
- **제안**: 플로터 액터/위젯을 풀링해 재사용하거나, 수명을 애니메이션 길이에 맞춰 줄이고 동시 표시 개수 상한을 둔다.
- **확신도**: 중간(현 규모에서는 문제가 안 될 수 있음)

### 3. 🟡 콤보 어빌리티 3종이 상태와 진행 로직을 그대로 복제한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp:29`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp:34`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp:29`
- **범주**: 중복/복잡도
- **문제**: `ComboMontages`·`ComboIndex` 선언이 세 헤더에 각각 있고(`WxAbility_Attack.h:33,37` / `WxAbility_Skill.h:36,40` / `WxAbility_Pattern.h:31,34`), `ActivateAbility`의 인덱스 전진·`PlayMontage`·`EndAbility`의 `bWasCancelled` 리셋이 세 cpp에서 같은 문장이다. Attack과 Skill은 애셋 태그와 쿨다운 GE 지정을 빼면 구현이 완전히 같다. Pattern은 진행 방식만 다른데(재발동 대신 블렌드아웃 연쇄, `WxAbility_Pattern.cpp:48`) 복제한 `ActivateAbility`의 인덱스 전진은 그 방식에서 뜻이 없고 — Pattern은 `bRetriggerInstancedAbility`가 없어 활성 중 재발동이 배타 판정에 막힌다 — 사실상 항상 0으로 떨어진다. 그 자리에 남은 로직이 실제 차이를 낳는 지점도 이미 있다: 연쇄 도중 `PlayMontage`가 실패하면 `bWasCancelled=false`로 끝나(`WxAbility_Pattern.cpp:55-58`) `ComboIndex`가 중간 단에 남아, 다음 발동이 첫 단이 아니라 그 다음 단부터 시작한다.
- **제안**: "콤보 몽타주 배열을 순서대로 재생한다"는 공통분만 중간 베이스(또는 `UWxAbilityBase`의 보호 헬퍼)로 올리고, 재발동 방식(입력 재발동 vs 블렌드아웃 자동 전진)만 파생에 남긴다. 최소 조치로는 Pattern에서 뜻 없는 인덱스 전진을 걷어내고 실패 종료를 `bWasCancelled=true`로 바꾼다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 배선 재사용 목적의 상속은 과거에 거부된 방향이므로, 공통분이 정말 의미 단위인지 먼저 합의가 필요하다)

### 4. 🟢 Max 어트리뷰트 변경이 현재값을 베이스에 덮어쓴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:188`
- **범주**: 버그/정확성
- **문제**: `AdjustCurrentAttributeForMaxChange`는 `GetNumericAttribute`(모디파이어가 반영된 **현재값**)를 읽어 비례 스케일한 뒤 `SetNumericAttributeBase`(**베이스**)에 쓴다(194행). 지금 HP/SP/GP/MP/UP를 건드리는 GE는 Instant이거나 주기형(`WxEffect_RegenSP`·`DrainSP`·`DrainGP`·`InfiniteMP`)뿐이라 전부 베이스를 직접 바꾸고, 지속형 aggregator 모디파이어가 하나도 없어 베이스=현재값이다. 그런 GE가 하나라도 생기면 MaxHP 변동 한 번에 모디파이어 몫이 베이스로 굳어 GE가 걷혀도 남는다.
- **제안**: `GetHP()` 등 베이스에 대응하는 읽기로 바꾸거나, 함수 주석에 "이 어트리뷰트들에는 지속형 모디파이어를 걸지 않는다"는 전제를 남긴다.
- **확신도**: 중간

### 5. 🟢 컷신 태스크가 캐릭터 메시를 널 검사 없이 역참조한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:98`
- **범주**: 성능/안전
- **문제**: `AvatarCharacter ? AvatarCharacter->GetMesh()->GetComponentTransform() : ...` 는 캐릭터 여부만 보고 메시 유효성은 보지 않는다. 기본 서브오브젝트라 대개 유효하지만, 메시를 만들지 않은 파생 캐릭터가 궁극기를 쓰면 그대로 크래시다. 같은 유형의 지적을 받았던 `AWxGhostTrail::BeginPlay`는 널 검사가 붙어(`WxCueNotify_GhostTrail.cpp:31-36`) 이 자리만 남았다.
- **제안**: `AvatarCharacter->GetMesh()`를 지역 변수로 받아 널이면 액터 트랜스폼으로 떨어뜨린다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxDamageTableRow.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Damage/WxCombatEffectContext.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Attack.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Skill.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Pattern.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Death.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Sprint.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Ultimate.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_DamageFloater.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionManagerComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Finisher/WxFinisherDamageComponent.cpp`
- **훑은 파일**: 나머지 어빌리티(`WxAbility_Passive/PlayMontageOnce`), GE 정의 21종(`Private/AbilitySystem/Effect/*` — 각 `DurationPolicy`·`Period`·모디파이어 대상까지 확인), AnimNotify 10종(`Private/AnimNotify/*`), GameplayCue 5종(`Private/AbilitySystem/Cue/*`), Targeting 필터·정렬 태스크 6종(`Private/Targeting/WxTargeting*`, `WxLockOnPointComponent.cpp`), 어빌리티 태스크(`WxAbilityTask_SlowTime/RotateToTarget/WaitMoving`), `WxAbilitySystemGlobals.cpp`, `WxAbilityTargetData_Direction.cpp`, `WxProjectileManagerComponent.cpp`, `WxCombatModule.cpp`, `WxCombat.Build.cs`, `WxCombat.uplugin`
- **미검토 / 한계**: 데이터 주도 저작물(DT_Ability·DT_Damage·DT_Effect 행 값, 어빌리티/GE의 BP 파생, 몽타주 노티파이 배치)은 범위 밖이라, 발견 3처럼 저작 배치가 조건인 항목은 실제 에셋에서 성립하는지 확인하지 못했다. GE 스택/면역 컴포넌트의 런타임 동작과 예측 롤백 경로는 코드 독해로만 판단했고 실행 검증은 하지 않았다. 락온 대상 지정은 `UWxLockOnComponent::ServerSetLockOnTarget`이 클라 값을 검증 없이 받는 구조이나, README가 "대상 선택은 클라 신뢰"로 명시한 의도라 발견으로 세우지 않았다 — 멀티플레이 정책을 정할 때 다시 볼 지점이다. 같은 이유로 `UWxCombatAttributeSet::PostGameplayEffectExecute`의 GP 분기가 SP 분기와 달리 권위 검사를 두지 않는 점도 발견으로 세우지 않았다(그로기 어빌리티가 `NetSecurityPolicy=ServerOnly`라 클라 트리거는 무해하게 기각된다).

---
*문서 기준 커밋 `a900118d` · 리뷰일 2026-09-05 · 소스 169파일 — `/module-review`로 갱신*
