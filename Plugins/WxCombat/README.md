# WxCombat — 전투 시스템

> Gameplay Ability System(GAS) 기반 액션 RPG 전투 도메인. 어트리뷰트/어빌리티/이펙트/큐, 대미지 파이프라인, 무기·투사체, 락온 타게팅, 전투용 AnimNotify를 책임진다.

## 책임
- GAS 전반: AttributeSet(스탯), Ability 베이스/구현, GameplayEffect, ExecutionCalculation, GameplayCue, AbilitySet 부여
- 대미지 파이프라인: `FWxDamageInfo` 설계 → Damage GE Spec 변환 → 가드/퍼펙트가드/무적/크리 판정 → HP·DP·SP 차감, HitReact 이벤트, 큐
- 무기/투사체 액터, 락온 컴포넌트, 전투용 AnimNotify(콤보 윈도우·무적·웨폰 공격·투사체 스폰 등)
- 글로벌 TimeDilation 권위 동기화(임시 거처)
- 담당하지 않음: Gameplay Tag **정의**(→ WxCore), UI/뷰모델 표현(→ WxUI), AI 행동 결정(→ WxAI). 여기서는 태그를 소비만 한다.

## 의존성
- **주요 의존**: `WxCore` · GameplayAbilities · GameplayTags · GameplayTasks · EnhancedInput · AIModule · NavigationSystem · NetCore · TargetingSystem · MotionWarping · Niagara · LevelSequence/MovieScene · UMG
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxCombatAttributeSet` | 전투 스탯(HP/SP/DP/MP/UP/ATK/DEF/Crit/SPD/ASPD + Meta IncomingDamage) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxAbilitySystemComponent` | 프로젝트 ASC. 입력 태그 라우팅, AbilitySet 부여, 래그돌 복제 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 쿨다운/충전/코스트/ASPD/후딜 캔슬(StartRecovery) 공통 처리 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | Ability·Effect·어트리뷰트 초기값을 한 에셋으로 묶어 일괄 부여하는 DataAsset | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터 → Damage GE Spec 배열로 변환(MakeSpecs) | `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxExecCalc_Damage` | 대미지 ExecutionCalculation. 무적/가드/퍼펙트가드/크리 판정과 차감의 중심 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |
| `UWxCombatLibrary` | 무기/투사체 외 경로의 대미지 적용·적대 판정 BP 함수 라이브러리 | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `AWxWeaponBase` | 근접 무기 액터. ANS_WeaponAttack이 Begin/EndAttack 호출, Overlap+Sweep 히트 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `AWxProjectileBase` | 투사체 액터. Spawn 시 DamageInfo→Spec 캐싱 후 Overlap 적용 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxProjectileBase.h` |
| `AWxEffectZone` | 접촉 시 GE를 적용하는 존(트랩·도트·환경 위해) 베이스 액터 | `Plugins/WxCombat/Source/WxCombat/Public/WxEffectZone.h` |
| `UWxLockOnComponent` | 캐릭터에 부착되어 락온 대상을 저장/조회 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnComponent.h` |
| `FWxDamageTableRow` / `FWxAbilityTableRow` | 대미지·어빌리티 밸런스 수치 DataTable Row | `Plugins/WxCombat/Source/WxCombat/Public/WxDamageTableRow.h` · `.../Ability/WxAbilityTableRow.h` |

## Gameplay Tags
이 모듈은 C++ Native Tag를 **선언하지 않는다**. 모든 태그는 `WxCore`의 `WxGameplayTags.h`에서 정의되며 여기서는 `WxGameplayTags::...`로 **참조(소비)** 만 한다. 사용 규칙(어떤 태그를 AssetTag/State/ANS로 쓸지)은 WxCore의 태그 문서를 따른다. 주로 쓰는 묶음: `Input.*`(입력 라우팅), `Ability.*`(어빌리티 정체성·캔슬 표식), `State.*`(액터 조건), `Event.HitReact.*`(피격 반응), `ANS.*`(노티파이 윈도우), `SetByCaller.*`(GE 수치 주입).

## 내부 구조
- `AbilitySystem/Attribute` — `UWxCombatAttributeSet` 및 초기값 Row(`WxCombatAttributeInitTableRow.h`)
- `AbilitySystem/Ability` — 어빌리티 베이스 + 구현(Attack/Skill/Ultimate/Dodge/Guard/Jump/Sprint/LockOn/HitReact/Groggy/Death/Pattern·Pattern_Phase)과 `WxAbilityTableRow`
- `AbilitySystem/Effect` — GameplayEffect(Damage/Burn/Cooldown/Cost·Recover/Buff/Sprint/Reflect 등), ExecutionCalculation(`WxExecCalc_Damage`·`WxExecCalc_Burn`), ModMagnitudeCalc(`WxMMC_LinearDrain`)
- `AbilitySystem/Cue` — GameplayCueNotify(Damage/Burn/HitStop/Exceed/BuffATK/PerfectGuard/Metamorphose)
- `AbilitySystem/Task` — AbilityTask(LockOnTarget/PlaySkillCutscene/SlowTime/TurnAround/WaitInputTag Pressed·Released)
- `AbilitySystem/TargetData` — `WxAbilityTargetData_Direction`(방향 입력 타겟 데이터)
- `AbilitySystem` (루트) — `UWxAbilitySystemComponent`, `UWxAbilitySet`
- `AnimNotify` — 전투 노티파이/노티파이스테이트(ComboWindow·Invincible·PerfectGuard·SnapToTarget·WeaponAttack·AreaAttack·SpawnProjectile·StartRecovery)
- `Targeting` — `UWxLockOnComponent`, TargetingSystem 필터 태스크(LineTrace·Team)
- `Weapon` — `AWxWeaponBase`, `AWxProjectileBase`
- `Time` — `UWxTimeDilationComponent`(글로벌 시간 배율 권위 동기화)
- 루트 — `WxCombatLibrary`, `WxDamageInfo`/`WxDamageTableRow`, `WxEffectZone`, `WxCombatModule`

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase`를 상속(BP/C++). 쿨다운(`CooldownTime`/`MaxRecharges`)·코스트(`MPCost`/`UPCost`)·`ActivationInputTag`·`AbilityDataRow`를 디폴트로 설정. 후딜 캔슬은 몽타주에 `ANS_StartRecovery`를 배치해 `StartRecovery()`가 자기 `BlockAbilitiesWithTag`를 풀게 한다.
- **새 대미지 공격**: `FWxDamageInfo`(또는 `FWxDamageTableRow`)로 설계 → 무기는 `ANS_WeaponAttack`, 투사체는 `WxAnimNotify_SpawnProjectile`, 광역/환경은 `UWxCombatLibrary::ApplyDamage/ApplyRawDamage`를 진입점으로 쓴다. 최종 판정은 `UWxExecCalc_Damage`에서 일원화된다.
- **데이터 주도**: 캐릭터 BP가 지정한 `UWxAbilitySet`이 InitAbilityActorInfo 시점에 어트리뷰트 초기값·어빌리티·이펙트를 일괄 부여한다. 밸런스 수치는 `WxAbilityTableRow`/`WxDamageTableRow` DataTable Row로 분리한다.
- **리플리케이션/권한(최대 4인)**: 어트리뷰트·래그돌·글로벌 TimeDilation은 서버 권위로 복제(`ReplicatedUsing`). Loose/OwnedTag는 기본 로컬이므로 서버 판정 조건을 anim 틱에 의존하지 말 것.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 모든 전투 수치의 정의·약어(HP/SP/DP/MP/UP 등). 시스템 어휘의 기준점.
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` — 대미지 판정 흐름 6단계가 주석으로 정리된 전투의 심장부.
3. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 쿨다운/코스트/후딜 캔슬 등 모든 어빌리티 공통 규약.

## 관련
- 상위: 캐릭터·게임 컨텐츠를 조립하는 [[WxGame]]에서 ASC/AbilitySet/무기를 장착. 태그 정의는 [[WxCore]], 어빌리티 발동을 트리거하는 적 AI는 [[WxAI]], 전투 수치를 표시하는 UI는 [[WxUI]].

---
*문서 기준 커밋 `d60410d8` · 생성일 2026-06-09 · 소스 139파일 — `/readme-writer`로 갱신*
