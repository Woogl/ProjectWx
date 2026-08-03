# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 구축한 액션 RPG 전투의 핵심 모듈. 어빌리티·어트리뷰트·대미지 파이프라인부터 무기/투사체, 락온 타게팅, 히트박스 AnimNotify, 시간 감속까지 "때리고 맞고 반응하는" 흐름 전체를 담당한다.

## 책임
**담당**
- GAS 통합: `UWxAbilitySystemComponent`(입력 라우팅), `UWxCombatAttributeSet`(스탯), `UWxAbilitySet`(데이터 주도 부여)
- 전투 어빌리티: 공격·회피·가드·스킬·궁극기·피니셔·히트리액트·그로기·사망·락온 등 (`AbilitySystem/Ability/WxAbility_*`)
- 대미지 파이프라인: `FWxDamageInfo` 설계 데이터 → GE Spec 변환 → `WxExecCalc_Damage` 산출 → `PostGameplayEffectExecute` HP 차감
- GameplayEffect/계산 모듈군: `WxEffect_*`(대미지·코스트·쿨다운·번·리소스), `WxExecCalc_*`, `WxMMC_*`, GameplayCue(`AbilitySystem/Cue/`)
- 무기·투사체·광역 액터: `AWxWeaponBase`, `AWxProjectileBase`, `AWxEffectZone`
- 히트박스/전투 타이밍 AnimNotify: `WxAnimNotify*`(무기 공격창·콤보 윈도·무적·퍼펙트가드·투사체 스폰 등)
- 락온 타게팅: `UWxLockOnManagerComponent`, TargetingSystem 필터 태스크(`Targeting/WxTargetingFilterTask_*`), 스냅 루트모션
- 시간 조작(히트스톱/슬로모): `UWxTimeDilationComponent`, `WxAbilityTask_SlowTime`

**경계 (비담당)**
- 공용 정의·팀/태그 등 파운데이션은 [[WxCore]]에 위임
- 어떤 어빌리티를 언제 쓸지(패턴 선택 등) 결정 로직은 [[WxAI]]. 본 모듈은 어빌리티 실행만 제공
- 전투 HUD·체력바 등 표현은 [[WxUI]]

## 의존성
- **주요 의존**: `WxCore` · GameplayAbilities · GameplayTags · GameplayTasks · ModularGameplay · TargetingSystem · MotionWarping · EnhancedInput · AIModule · Niagara · LevelSequence/MovieScene
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (Build.cs 의존은 `WxCore` 하나뿐)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 캐릭터 ASC. 입력 액션 라우팅과 AbilitySet 부여의 허브 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilitySet` | 어빌리티·이펙트·어트리뷰트를 한 번에 부여/회수하는 데이터 애셋 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | HP/SP/DP/MP/UP·ATK/DEF·크리 등 전투 스탯 + Meta `IncomingDamage` | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 쿨다운/코스트를 DataTable Row로 해석 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터. GE Spec 변환의 입력 | `Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxExecCalc_Damage` | 대미지 최종 산출(ExecutionCalculation) | `Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |
| `AWxWeaponBase` / `AWxProjectileBase` | 무기 스윙·투사체 히트 콜리전과 대미지 전달 | `Source/WxCombat/Public/Weapon/` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent 단위)을 서버 권위로 복제·추적 | `Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase` 상속. `EWxAbilityActivationPolicy`로 트리거형/부여즉시형 선택. 쿨다운·코스트 수치는 코드가 아닌 `FWxAbilityTableRow`(`AbilitySystem/Ability/WxAbilityTableRow.h`)에서만 읽는다. 쿨다운은 공용 `UWxEffect_Cooldown` GE + 소스 어빌리티 CDO로 구분.
- **데이터 주도 부여**: `UWxAbilitySet`에 Ability/Effect/AttributeInit을 묶어 `GiveAbilitySet()`으로 부여, `FWxAbilitySetGrantedHandles`로 일괄 회수. 초기값은 `FWxCombatAttributeInitTableRow`.
- **대미지 튜닝**: `FWxDamageInfo` + `WxDamageTableRow`(`Public/WxDamageTableRow.h`), 최종 계산은 `WxExecCalc_Damage`. 파생 코스트/쿨다운은 `WxMMC_*`.
- **히트박스/타이밍**: 신규 전투 타이밍은 `WxAnimNotify_*`/`WxAnimNotifyState_*`로 몽타주에 배치. 무기 공격창은 ANS_WeaponAttack이 `AWxWeaponBase::BeginAttack/EndAttack` 호출.
- **타게팅 필터**: TargetingSystem 필터 태스크로 확장(`WxTargetingFilterTask_Team/LineTrace/ScreenBounds/GameplayTag/InputDirection`).
- **리플리케이션**: 어트리뷰트·락온 대상·최근 입력은 서버 권위 복제. 락온은 소유 클라 예측 후 복제값으로 정합. Gameplay Tag는 C++ Native 선언 없이 에셋에서 관리.

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력→어빌리티 라우팅과 AbilitySet 부여, 전투의 중심 허브
2. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티 작성 규약(활성화 정책·데이터 Row·쿨다운) 파악
3. `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 스탯 약어(HP/SP/DP/MP/UP…)와 피격/사망 파이프라인(`IncomingDamage`→HP 차감)
4. `Source/WxCombat/Public/WxDamageInfo.h` + `Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` — 대미지가 설계 데이터에서 최종 수치로 흐르는 경로

## 관련
- 상위: 전투 어빌리티를 구동하는 [[WxAI]], 전투 상태를 표시하는 [[WxUI]] · Experience/캐릭터 BP가 `UWxAbilitySet`을 지정해 이 시스템을 부팅 · 공용 정의는 [[WxCore]]

---
*문서 기준 커밋 `28ee2c6` · 생성일 2026-08-03 · 소스 147파일 — `/readme-writer`로 갱신*
