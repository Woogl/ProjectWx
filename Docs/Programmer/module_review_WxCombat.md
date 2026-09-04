# WxCombat — 코드 리뷰

> 건강한 모듈이다. 직전 리뷰에서 지적한 9건 중 4건(입력 눌림/뗌 비대칭, 파일명·클래스명 불일치, GhostTrail 널 역참조, 미사용 모듈 의존)이 해결됐고, 무기 겹침 구간은 의도를 밝힌 주석이 붙어 지적을 거뒀다. 프로젝트 규칙 위반은 이번에도 한 건도 없다(모듈 경계 `WxCore` 단일 의존, `Wx` 접두사, `Handle` 콜백 접두사, `BlueprintCallable` 1건이 Blueprint Function Library, 인라인 정의 0건, 람다 0건, 저작권 첫 줄 전 파일 충족). 이번 리뷰는 변경분(락온 어빌리티·태스크, ASC 입력 라우팅, GhostTrail, Build.cs)을 먼저 확인한 뒤 대미지 파이프라인(ExecCalc·AttributeSet·CombatLibrary·DamageTableRow), 어빌리티 기반 클래스와 발동 그룹·캔슬 창, 선입력 버퍼, 무기·투사체 히트, 소환/처형/히트스톱, 어빌리티 15종·GE 21종·AnimNotify 10종·GameplayCue 6종·Targeting 태스크 7종을 cpp까지 읽었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 3 |
| 🟢 사소 | 2 |

## 결과

### 1. 🟡 크리티컬 난수를 예측 클라와 서버가 각각 굴린다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp:168`
- **범주**: 버그/정확성
- **문제**: `bIsCritical = FMath::FRand() < CritChance`는 시드를 공유하지 않는다. 공격 어빌리티가 `LocalPredicted`이고 `UWxCombatLibrary::ApplyDamage`가 예측 키를 실어 GE를 걸므로(`WxCombatLibrary.cpp:128`), 소유 클라의 `HasNetworkAuthorityToApplyGameplayEffect`가 통과해 실행 계산이 클라와 서버에서 각각 한 번씩 돈다. 두 결과가 갈리면 ① `FinalDamage`가 달라져 HP가 복제 도착 시 튀고 ② 같은 함수 205행의 `TargetSP <= FinalDamage` 판정까지 갈려 `Damage.GuardBreak` 여부가 어긋나며 ③ 195행 `SetCritical`이 실은 크리 표식이 클라 플로터와 서버 값 사이에서 불일치한다. 로컬 플레이만 하는 동안에는 드러나지 않는다.
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
- **문제**: `ComboMontages`·`ComboIndex` 선언이 세 헤더에 각각 있고(`WxAbility_Attack.h:33,37` / `WxAbility_Skill.h:37,41` / `WxAbility_Pattern.h:31,34`), 인덱스 전진·`PlayMontage`·`EndAbility`의 `bWasCancelled` 리셋까지 세 cpp가 사실상 같은 문장이다. Attack과 Skill은 쿨다운 GE 지정과 애셋 태그를 빼면 구현이 동일하다. 콤보 단계 규약이 바뀌면 세 곳을 함께 고쳐야 하고, 실제로 Pattern만 `HandleMontageCompleted`를 오버라이드하지 않아 자연 종료에서 인덱스를 되돌리지 않는 차이가 이미 생겼다(다음 발동의 `IsValidIndex(ComboIndex + 1)` 실패가 우연히 0으로 되돌려 덮고 있다).
- **제안**: "콤보 몽타주 배열을 순서대로 재생한다"는 공통분만 중간 베이스(또는 `UWxAbilityBase`의 보호 헬퍼)로 올리고, 재발동 방식(입력 재발동 vs 블렌드아웃 자동 전진)만 파생에 남긴다.
- **확신도**: 낮음(의도된 설계일 수 있음 — 배선 재사용 목적의 상속은 과거에 거부된 방향이므로, 공통분이 정말 의미 단위인지 먼저 합의가 필요하다)

### 4. 🟢 Max 어트리뷰트 변경이 현재값을 베이스에 덮어쓴다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp:188`
- **범주**: 버그/정확성
- **문제**: `AdjustCurrentAttributeForMaxChange`는 `GetNumericAttribute`(모디파이어가 반영된 **현재값**)를 읽어 비례 스케일한 뒤 `SetNumericAttributeBase`(**베이스**)에 쓴다(194행). 지금은 HP/SP/GP/MP/UP에 지속형 aggregator 모디파이어를 거는 GE가 없어 베이스=현재값이라 드러나지 않지만, 그런 GE가 하나라도 생기면 MaxHP 변동 한 번에 모디파이어 몫이 베이스로 굳어 GE가 걷혀도 남는다.
- **제안**: `GetHP()` 등 베이스에 대응하는 읽기로 바꾸거나, 함수 주석에 "이 어트리뷰트들에는 지속형 모디파이어를 걸지 않는다"는 전제를 남긴다.
- **확신도**: 중간

### 5. 🟢 컷신 태스크가 캐릭터 메시를 널 검사 없이 역참조한다
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp:98`
- **범주**: 성능/안전
- **문제**: `AvatarCharacter ? AvatarCharacter->GetMesh()->GetComponentTransform() : ...` 는 캐릭터 여부만 보고 메시 유효성은 보지 않는다. 기본 서브오브젝트라 대개 유효하지만, 메시를 만들지 않은 파생 캐릭터가 궁극기를 쓰면 그대로 크래시다. 같은 유형의 지적을 받은 `AWxGhostTrail::BeginPlay`는 이번 커밋에서 널 검사가 추가돼(`WxCueNotify_GhostTrail.cpp:31-36`) 이 자리만 남았다.
- **제안**: `AvatarCharacter->GetMesh()`를 지역 변수로 받아 널이면 액터 트랜스폼으로 떨어뜨린다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_LockOn.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_LockOnCamera.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Cue/WxCueNotify_GhostTrail.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Attribute/WxCombatAttributeSet.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/WxCombatLibrary.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxInputBufferComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxWeaponBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxLockOnComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxHitStopComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Dodge.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Guard.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_HitReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_GuardReact.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Groggy.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbility_Finisher.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionManagerComponent.cpp`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Task/WxAbilityTask_PlaySkillCutscene.cpp`
- **훑은 파일**: 나머지 어빌리티(`WxAbility_Attack/Skill/Pattern/Passive/Ultimate/Sprint/Death/PlayMontageOnce`), GE 정의 21종(`Private/AbilitySystem/Effect/*`), AnimNotify 10종(`Private/AnimNotify/*`), GameplayCue 5종(`Private/AbilitySystem/Cue/*`), Targeting 필터·정렬 태스크 6종(`Private/Targeting/WxTargeting*`), 어빌리티 태스크(`WxAbilityTask_SlowTime/RotateToTarget/WaitMoving`), `WxAbilitySet.cpp`, `WxAbilitySystemGlobals.cpp`, `WxCombatEffectContext.cpp`, `WxDamageTableRow.cpp`, `WxEffectComponent_Table.cpp`, `WxFinisherDamageComponent.cpp`, `WxProjectileManagerComponent.cpp`, `WxLockOnPointComponent.cpp`, `WxAbilityTargetData_Direction.cpp`, `WxCombatModule.cpp`, `WxCombat.Build.cs`, `WxCombat.uplugin`
- **미검토 / 한계**: 데이터 주도 저작물(DT_Ability·DT_Damage·DT_Effect 행 값, 어빌리티/GE의 BP 파생, 몽타주 노티파이 배치)은 범위 밖이라, 발견 3처럼 저작 배치가 조건인 항목은 실제 에셋에서 성립하는지 확인하지 못했다. GE 스택/면역 컴포넌트의 런타임 동작과 예측 롤백 경로는 코드 독해로만 판단했고 실행 검증은 하지 않았다. 락온 대상 지정은 `UWxLockOnComponent::ServerSetLockOnTarget`이 클라 값을 검증 없이 받는 구조이나, README가 "대상 선택은 클라 신뢰"로 명시한 의도라 발견으로 세우지 않았다 — 멀티플레이 정책을 정할 때 다시 볼 지점이다.

---
*문서 기준 커밋 `303d8d7f` · 리뷰일 2026-09-05 · 소스 169파일 — `/module-review`로 갱신*
