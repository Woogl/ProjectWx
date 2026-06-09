# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반의 액션 RPG 전투 도메인. 어빌리티·어트리뷰트·이펙트·큐로 데미지/스킬/상태이상을 처리하고, 무기·투사체·락온·히트스톱 등 액션 전투의 런타임을 담당한다.

## 책임
**담당**
- GAS 통합: 커스텀 ASC(`UWxAbilitySystemComponent`), 전투 AttributeSet, AbilitySet 일괄 부여
- 어빌리티: 공격/회피/가드/스킬/궁극기/패턴(보스) 등 `UWxAbilityBase` 파생 (쿨다운·코스트·후딜 캔슬 공용 처리)
- GameplayEffect/ExecCalc/MMC/Cue: 데미지·번·버프·코스트·리소스 회복·치트성 이펙트
- 무기/투사체 히트 판정과 데미지 적용(`AWxWeaponBase`, `AWxProjectileBase`, `UWxCombatLibrary::ApplyDamage`)
- 락온/타겟팅(TargetingSystem 필터 태스크), 멀티 동기화되는 글로벌 타임딜레이션, 접촉형 이펙트 존
- 전투용 AnimNotify/AnimNotifyState (콤보 윈도우, 무적, 퍼펙트가드, 무기 공격 구간, 투사체/광역 스폰)

**경계 (비담당)**
- Gameplay Tag 네이티브 선언 — [[WxCore]]의 `WxGameplayTags`를 소비만 한다
- 캐릭터 클래스/입력 바인딩/카메라 등 폰 본체 — 게임 모듈([[WxGame]]) 및 [[WxCore]]
- UI 표시/뷰모델 — [[WxUI]] (이 모듈은 아이콘/쿨다운 GE 클래스 등 읽을 데이터만 노출)

## 의존성
- **주요 의존**: [[WxCore]] / 엔진 서브시스템 `GameplayAbilities`, `GameplayTags`, `GameplayTasks`, `TargetingSystem`, `EnhancedInput`, `MotionWarping`, `LevelSequence`/`MovieScene`(스킬 컷씬), `Niagara`(FX)
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 커스텀 ASC. 입력 태그→어빌리티 활성화, AbilitySet 부여, 래그돌 복제 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티의 추상 베이스. 쿨다운/코스트/후딜 캔슬·테이블 Row 공용 처리 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | Ability/Effect/Attribute 초기 데이터를 묶은 DataAsset, 일괄 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | HP/SP/DP/MP/UP·ATK/DEF/Crit/SPD/ASPD·IncomingDamage 어트리뷰트 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 데미지 한 건의 설계 데이터. GE Spec 배열로 변환 | `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxCombatLibrary` | `ApplyDamage`/`ApplyRawDamage`/`IsHostile` 공용 전투 유틸 (BP Function Library) | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `UWxExecCalc_Damage` | 데미지 ExecutionCalculation (ATK/DEF/Crit 반영) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |
| `AWxWeaponBase` | 무기 액터. ANS_WeaponAttack 구간 동안 캡슐 Overlap 히트 판정 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `AWxProjectileBase` | 투사체 액터. Spec 캐싱 후 Overlap 시 적용 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxProjectileBase.h` |
| `UWxLockOnComponent` | 락온 대상 저장/조회 컴포넌트 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnComponent.h` |
| `UWxTimeDilationComponent` | GameState 부착, 서버 권위 글로벌 타임딜레이션(멀티 동기화) | `Plugins/WxCombat/Source/WxCombat/Public/Time/WxTimeDilationComponent.h` |
| `AWxEffectZone` | 접촉 시 GE를 적용하는 베이스 액터(트랩/장판) | `Plugins/WxCombat/Source/WxCombat/Public/WxEffectZone.h` |

## 폴더 구성
- `AbilitySystem/Ability` — `UWxAbilityBase` 파생 어빌리티 + `WxAbilityTableRow`(수치 데이터)
- `AbilitySystem/Attribute` — AttributeSet + 어트리뷰트 초기화 테이블 Row
- `AbilitySystem/Effect` — GameplayEffect, ExecCalc(Damage/Burn), MMC(LinearDrain)
- `AbilitySystem/Cue` — GameplayCue Notify (데미지/히트스톱/번/버프 등 FX·연출)
- `AbilitySystem/Task` — 커스텀 AbilityTask (락온/컷씬/슬로우타임/입력 대기 등)
- `AbilitySystem/TargetData` — 어빌리티 TargetData(방향)
- `AnimNotify` — 전투 AnimNotify / AnimNotifyState
- `Targeting` — 락온 컴포넌트 + TargetingSystem 필터 태스크(LineTrace/Team)
- `Weapon` — 무기/투사체 베이스 액터
- `Time` — 글로벌 타임딜레이션 컴포넌트

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase` 상속. `ActivationPolicy`(입력/부여) + `ActivationInputTag` 지정, 쿨다운/코스트는 `CooldownTime`/`MaxRecharges`/`MPCost`/`UPCost` 프로퍼티로 데이터 주도. 후딜 캔슬 구간은 `StartRecovery()`로 진입.
- 데이터 주도: 캐릭터별 시작 능력은 `UWxAbilitySet` DataAsset(어빌리티/이펙트/어트리뷰트 초기 Row). 어빌리티 수치는 `WxAbilityTableRow`, 데미지는 `WxDamageTableRow` DataTable Row로 오버라이드.
- 데미지 경로: 무기/투사체는 `FWxDamageInfo`→`MakeSpecs`→Damage GE, 그 외 광역/환경 단발은 `UWxCombatLibrary::ApplyDamage`/`ApplyRawDamage`. 최종값은 `IncomingDamage` 메타 어트리뷰트로 모아 `PostGameplayEffectExecute`에서 HP 차감.
- 리플리케이션(최대 4인): 어트리뷰트는 `ReplicatedUsing` OnRep, 입력 태그/래그돌은 ASC에서 서버 RPC·복제, 타임딜레이션은 GameState 컴포넌트가 서버 권위로 전 머신 동기화.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티 공통 규약(쿨다운/코스트/후딜)의 중심
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 전투 수치 전체 정의와 데미지 흐름
3. `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` + `Public/WxCombatLibrary.h` — 데미지가 어떻게 구성·적용되는지

## 관련
- 상위: 게임 모듈 [[WxGame]]이 캐릭터에 ASC/AbilitySet을 부착해 사용. 공용 정의·Gameplay Tag는 [[WxCore]], 표시는 [[WxUI]], 적 행동은 [[WxAI]]와 함께 본다.

---
*문서 기준 커밋 `80cc348` · 생성일 2026-06-09 · 소스 137파일 — `/readme-writer`로 갱신*
