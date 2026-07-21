# WxCombat — 코드 리뷰

> GAS 기반 전투 런타임 전반이 일관된 규약(공용 Cooldown/Cost GE + MMC, DataRow 주도, 반응형 어빌리티의 실패복구 패턴)으로 짜여 있고 네트워크 권위 처리도 대체로 신중하다 — 코드 건강도는 높은 편이다. 이번 리뷰는 ASC·AbilitySet·대미지 ExecCalc·AttributeSet·대미지 진입점(Library/DamageInfo)·무기/투사체·LockOn·TimeDilation과 핵심 반응형 어빌리티(HitReact/Guard/Groggy/Finisher/Dodge/Attack/Skill/Death/Ultimate/Pattern)를 깊게 봤고, 나머지 GE/MMC/Cue/AnimNotify/Targeting 필터는 훑었다.

## 요약
| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 2 |

가장 먼저 볼 것: 투사체(`AWxProjectileBase`)와 접촉 이펙트 존(`AWxEffectZone`)이 오버랩 콜백에서 대미지/이펙트 GE를 `HasAuthority` 게이트 없이 적용한다. 같은 모듈의 `WxAnimNotify_AreaAttack`는 명시적으로 권위 게이트를 두는데, 이 둘만 빠져 있어 오버사이트로 보인다.

## 발견

### 🟡 투사체·이펙트 존이 권위 게이트 없이 대미지/이펙트 GE를 적용
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/Weapon/WxProjectileBase.cpp:95`(`HandleHitCollisionOverlap`, 특히 :126~137), `Plugins/WxCombat/Source/WxCombat/Private/WxEffectZone.cpp:18`(`ApplyEffect`)·`:72`(`NotifyActorBeginOverlap`)
- **범주**: 설계/구조 (리플리케이션 권위 모델)
- **문제**: 투사체는 서버 스폰·`bReplicates=true`라 클라에서 시뮬프록시로 존재하고, 오버랩은 클라에서도 발생한다. 그런데 두 경로 모두 `Owner->HasAuthority()` 검사 없이 `ApplyGameplayEffectSpecToSelf/ToTarget`으로 Instant 대미지 GE를 바로 적용한다. Instant GE는 비권위 클라에서도 로컬 실행되므로:
  - `UWxCombatAttributeSet::PostGameplayEffectExecute`가 클라 로컬에서 돌며 HP<=0/DP만땅 시 `State.Dead`·`State.Groggy` **루스 태그를 로컬로 추가**한다(`WxCombatAttributeSet.cpp:131,168`). 이 루스 태그는 복제 정합으로 되돌려지지 않아, 실제로는 살아있는 적이 특정 클라에서만 사망/그로기 상태로 남을 수 있다.
  - 투사체는 이어서 `Destroy()`(`WxProjectileBase.cpp:137`)를 클라에서도 호출한다 — 복제 액터의 수명은 서버 권위여야 하며, 클라 선파괴는 조기 소멸/타이밍 불일치를 만든다.
  - (대미지 GameplayCue는 `ExecuteGameplayCue`가 비권위·무예측키에서 self-gate 되어 중복되지 않으므로, 핵심 피해는 위 두 가지다.)
  대비: `WxAnimNotify_AreaAttack.cpp:20`는 동일 대미지 적용 전에 `Owner->HasAuthority()`로 게이트한다. 이 일관성 결여가 오버사이트라는 근거.
- **제안**: 두 경로의 GE 적용(및 투사체 `Destroy`)을 `HasAuthority()` 뒤로 넣어 `WxAnimNotify_AreaAttack`와 통일한다. 무기(`WxWeaponBase::ProcessHit`)처럼 예측을 의도한다면 예측키 스코프에서 적용하도록 명시하고, 아니라면 서버 권위로 단일화한다.
- **확신도**: 중간 (Instant GE의 비권위 클라 로컬 실행·루스 태그 미정합 동작에 기반. 실제 노출 정도는 `WxProjectile` 콜리전 프로파일과 State.Dead를 읽는 클라 소비처에 따라 달라짐)

### 🟢 대미지 ExecCalc의 치명타가 비결정적 `FMath::FRand()` — 예측 경로에서 클라/서버 분기
- **위치**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp:263`
- **범주**: 버그/정확성
- **문제**: 무기 근접타(`WxWeaponBase::ProcessHit`)는 클라(예측)와 서버 양쪽에서 대미지 GE를 적용하고, ExecCalc도 양쪽에서 실행된다. 전역 RNG `FRand()`는 시드가 공유되지 않아 치명타 판정이 클라/서버 간 달라져 예측 `IncomingDamage` 값이 잠깐 튈 수 있다(HP 복제로 자기수정되고, 크리 큐는 self-gate라 중복은 없음).
- **제안**: 실질 피해가 크지 않으면 그대로 둬도 무방하나, 정합이 중요하면 치명타 판정을 서버 권위로 분리하거나 히트별 결정적 시드를 쓴다.
- **확신도**: 중간 (자기수정되는 시각적 소음 수준)

### 🟢 Targeting 필터 파일 2건의 UTF-8 BOM
- **위치**: `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxTargetingFilterTask_GameplayTag.h:1`, `Plugins/WxCombat/Source/WxCombat/Private/Targeting/WxTargetingFilterTask_GameplayTag.cpp:1`
- **범주**: 규칙 위반 (사소)
- **문제**: 첫 줄 copyright 앞에 UTF-8 BOM(`﻿`)이 붙어 있어 "첫 줄은 `// Copyright ...`로 시작한다" 규칙에 미세하게 어긋난다(내용 자체는 존재). 나머지 파일은 BOM 없음.
- **제안**: 두 파일을 BOM 없는 UTF-8로 재저장.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp`, `.../Attribute/WxCombatAttributeSet.cpp`, `.../Ability/WxAbilityBase.cpp`, `.../Ability/WxAbility_Finisher.cpp`, `.../Ability/WxAbility_HitReact.cpp`, `.../Ability/WxAbility_Guard.cpp`, `.../Ability/WxAbility_Groggy.cpp`, `.../Ability/WxAbility_Dodge.cpp`, `.../Ability/WxAbility_Attack.cpp`, `.../Ability/WxAbility_Skill.cpp`, `.../Ability/WxAbility_LockOn.cpp`, `.../WxAbilitySystemComponent.cpp`, `.../WxAbilitySet.cpp`, `.../Task/WxAbilityTask_LockOnTarget.cpp`, `Private/Weapon/WxWeaponBase.cpp`, `Private/Weapon/WxProjectileBase.cpp`, `Private/Targeting/WxLockOnManagerComponent.cpp`, `Private/Targeting/WxRootMotionModifier_SnapToTarget.cpp`, `Private/Time/WxTimeDilationComponent.cpp`, `Private/WxCombatLibrary.cpp`, `Private/WxDamageInfo.cpp`, `Private/WxEffectZone.cpp`
- **훑은 파일**: 나머지 `WxEffect_*`/`WxMMC_*`(Cooldown/Cost/Damage 확인), `WxCueNotify_*`, `WxAnimNotify(State)_*`(WeaponAttack/ComboWindow/StartRecovery/AreaAttack/FinisherDamage/SpawnProjectile/SnapToTarget 확인), `WxTargetingFilterTask_*`, `WxAbility_Ultimate/Death/Pattern/Sprint`, `WxAbilityTask_SlowTime/WaitInputActionTriggered/PlaySkillCutscene`, `WxCombat.Build.cs`
- **미검토 / 한계**: `WxCueNotify_Burn`(메모리상 이미 미해결 애셋 이슈로 기록됨)과 각 Cue의 세부 연출 로직, `WxAbilityTask_PlaySkillCutscene`/Sprint 내부는 얕게만 확인. BP 디폴트값(몽타주·DataRow·GE 클래스 배선)은 C++ 범위 밖이라 미검증. 리플리케이션 관련 발견은 런타임 네트워크 실측이 아니라 코드·GAS 동작 추론 기반이다.

---
*문서 기준 커밋 `702fc70f` · 리뷰일 2026-07-22 · 소스 149파일 — `/module-review`로 갱신*
