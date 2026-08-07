# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반 액션 RPG 전투 도메인. 어빌리티·어트리뷰트·이펙트 파이프라인 위에 공격/가드/회피/락온/히트스톱/투사체 등 전투 전반을 얹는다.

## 책임
**담당**
- 어빌리티 실행·입력 라우팅: `UWxAbilitySystemComponent`가 EnhancedInput 입력을 어빌리티로 라우팅하고, `UWxAbilityBase` 파생 어빌리티(Attack/Guard/Dodge/Skill/Ultimate/Finisher/Groggy/HitReact/LockOn/Sprint/Pattern 등)가 실제 동작을 담당.
- 전투 스탯: `UWxCombatAttributeSet` (HP/SP/DP/MP/UP·ATK/DEF/Crit·SPD/ASPD·IncomingDamage 메타).
- 대미지 파이프라인: `FWxDamageInfo` → GameplayEffect Spec → `WxExecCalc_Damage`. GE/MMC/ExecCalc·GameplayCue 일습.
- 무기/투사체 히트 판정: `AWxWeaponBase`(오버랩 스윙), `AWxProjectileBase`.
- 타게팅/락온: `UWxLockOnManagerComponent`, TargetingSystem FilterTask 군.
- 시간 조작: 히트스톱(몽타주 프리즈) 및 전역 슬로우(`UWxTimeDilationComponent`).
- AnimNotify 전투 훅: WeaponAttack/ComboWindow/Invincible/PerfectGuard/SnapToTarget/SpawnProjectile 등.

**경계 (비담당)**
- 캐릭터/입력 셋업·Experience 주입 등 상위 조립은 게임 모듈(`WxGame`) 몫. 이 모듈은 컴포넌트/어빌리티/에셋만 제공.
- HUD·자원 게이지 등 표현은 [[WxUI]]. 여기선 어트리뷰트 값과 델리게이트만 노출.
- AI 의사결정은 [[WxAI]]. 이 모듈은 AI가 트리거하는 `UWxAbility_Pattern` 실행 진입점까지만.

## 의존성
- **주요 의존**: [[WxCore]] (유일한 Wx 의존). 엔진: GameplayAbilities(GAS), TargetingSystem, MotionWarping, ModularGameplay, EnhancedInput, Niagara(투사체 FX), LevelSequence/MovieScene(스킬 컷신).
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | ASC 파생. 입력 엣지/레벨을 어빌리티로 라우팅하고 `Event.HitStop` 처리·몽타주 재생속도(ASPD)를 중재 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티의 Abstract 베이스. 쿨다운/코스트/충전을 `AbilityDataRow` 기반 공용 GE로 처리, 후딜(캔슬 구간)·투사체 스폰 훅 | `.../AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | 전투 스탯 전량 + `IncomingDamage` 메타 어트리뷰트(ExecCalc→HP 차감 패턴) | `.../AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxAbilitySet` | 어빌리티·이펙트·어트리뷰트를 묶어 ASC에 일괄 부여/회수하는 DataAsset (부여 결과는 `FWxAbilitySetGrantedHandles`) | `.../AbilitySystem/WxAbilitySet.h` |
| `UWxCombatLibrary` | 무기/투사체 밖 단일 대미지 적용의 공용 진입점 `ApplyDamage` (BP Function Library) | `.../Public/WxCombatLibrary.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터. `MakeSpecs`로 Damage GE Spec + 부가효과 Spec 생성 | `.../Public/WxDamageInfo.h` |
| `AWxWeaponBase` | 근접 무기 액터. ANS_WeaponAttack이 여는 오버랩 스윙, 스윙당 액터 1회 피격 | `.../Weapon/WxWeaponBase.h` |
| `UWxLockOnManagerComponent` | 서버 권위로 복제되는 락온 대상(SceneComponent 단위) 보관·브로드캐스트 | `.../Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase`를 상속. 쿨다운/코스트/아이콘 수치는 코드가 아니라 `AbilityDataRow`(`FWxAbilityTableRow`)에서 온디맨드로 읽는다. `CooldownGameplayEffectClass`/`CostGameplayEffectClass`를 공용 GE(`UWxEffect_Cooldown`/`UWxEffect_Cost`) 마커 그대로 두면 데이터 주도 경로, 다른 GE로 바꾸면 엔진 순정 경로(상호배타).
- 입력 발동 어빌리티는 `ActivationInputAction` 지정. 복수 입력(약/강 공격 등)은 `IsActivationInput`/`GetInputActions` override로 확장. AI·반응·패시브는 비워두거나 `ActivationPolicy=OnGranted`.
- 대미지 튜닝: `FWxDamageTableRow`(`WxDamageTableRow.h`)에 공격별 계수·회복·HitReact 태그·가드 관통 여부를 두고, `FWxDamageInfo`로 실어 SetByCaller/DynamicAssetTags로 변환. HitReact 종류는 `Event.HitReact.*` 태그로 지정.
- 새 GE/계산: `AbilitySystem/Effect/`의 `WxEffect_*`(GE), `WxExecCalc_*`(ExecutionCalculation), `WxMMC_*`(ModMagnitudeCalculation) 패턴을 따른다. 연출은 `AbilitySystem/Cue/`의 `WxCueNotify_*`.
- 리플리케이션/권한: 투사체 스폰·파괴·락온 대상·전역 시간배율은 서버 권위, 표현(ImpactFX/큐)은 각 머신 로컬. 대미지 적용은 몽타주 재생 어빌리티의 활성화 키로 예측된다.
- GameplayTag는 C++ Native 선언 없이 데이터(태그 매니저/에셋)에서 참조한다 — 태그 문자열은 `Event.*`/`Event.HitReact.*` 등 규약을 따른다.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력→어빌리티 라우팅과 히트스톱 중재. 전투 흐름의 관문.
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 모든 어빌리티의 데이터 주도 쿨다운/코스트·후딜 규약. 어빌리티를 이해하는 기준점.
3. `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` — 대미지가 GE Spec으로 변환되는 지점. `WxExecCalc_Damage.cpp`로 이어 읽으면 데미지 계산 전모.
4. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` — 캐릭터가 무엇을 갖게 되는지(부여/회수)의 조립 뷰.

## 관련
- 상위: 캐릭터/Experience 조립·입력 셋업은 `WxGame`. 함께 보는 모듈 — [[WxCore]](공용 정의), [[WxUI]](자원 게이지), [[WxAI]](패턴 트리거).

---
*문서 기준 커밋 `bb8ee6b` · 생성일 2026-08-07 · 소스 143파일 — `/readme-writer`로 갱신*
