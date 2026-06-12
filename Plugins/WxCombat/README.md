# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반의 액션 RPG 전투 도메인. 어빌리티/이펙트/어트리뷰트, 데미지 파이프라인, 무기·투사체, 락온/타게팅, 타임 딜레이션(히트스톱·슬로모)을 담당한다.

## 책임
**담당**
- 캐릭터 ASC·어트리뷰트(HP/SP/DP/MP/UP/ATK/DEF/Crit/SPD/ASPD)와 어빌리티/이펙트 부여(`UWxAbilitySet`).
- 어빌리티 베이스 및 구현 — 공격·회피·가드·스킬·궁극기·히트리액트·그로기·사망·AI 패턴.
- 데미지 한 건의 설계→적용 파이프라인: `FWxDamageInfo` → `UWxExecCalc_Damage` → `IncomingDamage` 메타 어트리뷰트 → HP 차감.
- 무기/투사체 히트 판정(`AWxWeaponBase`, `AWxProjectileBase`), AnimNotify로 공격 구간·콤보 윈도우·무적·퍼펙트가드 제어.
- 락온/타게팅(`UWxLockOnManagerComponent`, TargetingSystem 필터 태스크), 히트스톱·슬로모 타임 딜레이션.

**경계 (비담당)**
- Gameplay Tag 정의 — [[WxCore]]의 `WxGameplayTags`를 참조만 한다(이 모듈은 태그를 선언하지 않음).
- 어빌리티 UI 표시 데이터(아이콘 등) — [[WxUI]]의 `UWxAbilityComponent` 파생으로 위임(EditInline 컴포넌트로 부착).
- 입력 바인딩의 상위 라우팅·플레이어 캐릭터 정의 — `WxGame` 게임 모듈.

## 의존성
- **주요 의존**: `WxCore`, GameplayAbilities, GameplayTasks, TargetingSystem, AIModule, MotionWarping(Private), Niagara(Private), EnhancedInput, LevelSequence/MovieScene(컷신, Private).
- 규칙: 「WxCore 외 Wx 플러그인 참조」 검증 — 없음 ✅ (Build.cs 의존은 `WxCore`뿐).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 캐릭터 ASC. 입력태그→어빌리티 라우팅, AbilitySet 부여, 래그돌 복제 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스(Abstract). 공용 쿨다운/코스트 GE, 테이블 Row 주입, 후딜·캔슬 규약 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | Ability/Effect/Attribute 초기값을 묶는 DataAsset. ASC에 일괄 부여 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | 전투 어트리뷰트 전체. `IncomingDamage` 메타로 데미지 수신 | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 데미지 한 건의 설계 데이터. AnimNotify에서 편집→Spec 변환 | `Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxCombatLibrary` | 무기/투사체 외 경로의 데미지 적용·적대 판정 진입점(BlueprintCallable) | `Source/WxCombat/Public/WxCombatLibrary.h` |
| `AWxWeaponBase` | 무기 액터. AnimNotify의 BeginAttack/EndAttack로 콜리전 히트 판정 | `Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxExecCalc_Damage` | 데미지 ExecutionCalculation. ATK/DEF/Crit 반영해 `IncomingDamage` 산출 | `Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase` 상속. 쿨다운/코스트는 `CooldownTime`·`MaxRecharges`·`MPCost`·`UPCost` 프로퍼티로 설정(공용 `UWxEffect_Cooldown`/`UWxEffect_Cost` GE 자동 사용). `OnActivateEffects`로 발동 시 자기 버프 부여.
- 새 이펙트/계산: `Effect/`에 `UGameplayEffect` 파생을, 복합 계산은 `UWxExecCalc_*`(ExecutionCalculation) 또는 `UWxMMC_*`(ModifierMagnitudeCalculation)로 추가.
- 데이터 주도: 어빌리티 수치는 `WxAbilityTableRow`, 어트리뷰트 초기값은 `WxCombatAttributeInitTableRow`, 데미지는 `WxDamageTableRow` DataTable Row로 주입(`UWxAbilitySet`/`UWxAbilityBase`의 `FDataTableRowHandle`).
- 연출은 GameplayCue(`Cue/`의 `UWxCueNotify_*`)로 분리 — 데미지·히트스톱·번·퍼펙트가드 등.
- 리플리케이션/권한(최대 4인): GAS 권한 모델을 따른다. 어트리뷰트·래그돌은 서버 권한+`ReplicatedUsing`으로 복제, 입력태그는 클라이언트→서버 RPC(`ServerSetLast*InputTag`)로 동기화.

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 모든 어빌리티의 공통 규약(쿨다운/코스트/후딜 캔슬)이 여기 정리되어 있다.
2. `Source/WxCombat/Public/WxDamageInfo.h` + `Source/WxCombat/Private/WxCombatLibrary.cpp` — 데미지가 설계 데이터에서 Spec→적용까지 흐르는 파이프라인.
3. `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 어트리뷰트 의미·복제·`IncomingDamage` 수신 패턴.

## 관련
- 상위: [[WxCore]] (foundation — 공용 정의·`WxGameplayTags`)

---
*문서 기준 커밋 `5ae4876` · 생성일 2026-06-11 · 소스 135파일 — `/readme-writer`로 갱신*
