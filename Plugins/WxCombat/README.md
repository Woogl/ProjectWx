# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반 액션 RPG 전투의 코어. 어빌리티·어트리뷰트·대미지 판정·무기 히트·락온·히트스톱을 한 플러그인에서 책임진다.

## 책임
**담당**
- 어빌리티 골격과 실제 행동들: 공격·스킬·궁극기·가드·회피·질주·피니셔·히트리액트·그로기·사망·락온·AI 패턴 (`WxAbility_*`), 그리고 이들을 캐릭터에 일괄 부여하는 `UWxAbilitySet`.
- 전투 어트리뷰트(HP/SP/DP/MP/UP/ATK/DEF/Crit/SPD/ASPD)와 그 복제·초기화 (`UWxCombatAttributeSet`).
- 대미지 파이프라인: 설계 데이터(`FWxDamageInfo`) → GE Spec → 판정 계산(`UWxExecCalc_Damage`) → 판정결과 전달(`FWxCombatEffectContext`) → 적용 후 Cue·이벤트 발행(`UWxAbilitySystemComponent`).
- 다양한 GameplayEffect·MMC·ExecCalc·GameplayCue·AbilityTask·AnimNotify(State) 구현체.
- 무기 히트 판정(`AWxWeaponBase`)·투사체(`AWxProjectileBase`)·락온(`UWxLockOnManagerComponent`)·타게팅 필터·시간 감속(`UWxTimeDilationComponent`).

**경계 (비담당)**
- Gameplay Tag 정의: `WxGameplayTags`는 [[WxCore]]에 있고 이 모듈은 소비만 한다 (자체 Native Tag 선언 없음).
- 어빌리티/이펙트/무기의 실제 데이터(BP 파생 클래스, DataTable 에셋, 몽타주)는 콘텐츠 측에서 저작한다 — 여기엔 C++ 베이스·규약만 있다.

## 의존성
- **주요 의존**: [[WxCore]] (유일한 Wx 의존). 엔진: `GameplayAbilities`, `TargetingSystem`, `MotionWarping`, `ModularGameplay`, `EnhancedInput`, `AIModule`, `GameplayTasks`, `Niagara`(private).
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (`.uplugin`·`Build.cs` 모두 WxCore만 참조)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | ASC 서브클래스. 입력→어빌리티 라우팅의 유일한 진입점이자, 대미지 GE 적용 후 Cue·반응 이벤트 발행·히트스톱을 담당하는 허브 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 `WxAbility_*`의 추상 베이스. 쿨다운·코스트를 `AbilityDataRow`에서 읽는 규약을 정의 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 어빌리티·이펙트·어트리뷰트 초기화 행을 묶은 DataAsset. 서버에서 ASC에 일괄 Grant | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | 전투 스탯 전체와 복제·`IncomingDamage` 메타 어트리뷰트 | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxExecCalc_Damage` | 대미지 계산·가드/무적/퍼펙트가드 분기. 판정결과는 EffectContext에 남기고 Cue는 쏘지 않음 | `Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |
| `FWxCombatEffectContext` | ExecCalc와 ASC의 발행부를 잇는 유일한 통로. 판정결과·크리·최종대미지·반응태그를 운반 | `Source/WxCombat/Public/Damage/WxCombatEffectContext.h` |
| `UWxCombatLibrary` | `ApplyDamage` — 무기·피니셔가 공유하는 단일 대미지 적용 진입점 | `Source/WxCombat/Public/WxCombatLibrary.h` |
| `AWxWeaponBase` | AnimNotifyState가 여닫는 무기 히트 콜리전(스윙당 1회 판정) | `Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxLockOnManagerComponent` | 복제되는 락온 대상 관리(SceneComponent 단위) | `Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase`를 상속하고, `AbilityDataRow`(`FWxAbilityTableRow`)에 쿨다운·충전·코스트를 채운 뒤 `UWxAbilitySet.GrantedAbilities`에 등록한다. 입력 발동이면 `ActivationInputAction` 지정, 아니면 비워 트리거/이벤트/패시브(`ActivationPolicy`)로 둔다.
- **쿨다운/코스트 규약**: `CooldownGameplayEffectClass`/`CostGameplayEffectClass`의 기본값(공용 `UWxEffect_Cooldown` 등)이 "마커"다. 마커 그대로면 Row 기반(MMC가 Row 조회), 다른 GE로 바꾸면 엔진 순정 경로. `UWxAbilityBase`가 순정 쿨다운 API를 CDO 쿼리 기반으로 오버라이드해 다중 충전을 지원한다.
- **새 대미지 원천**: `FWxDamageInfo`(테이블 행 또는 코드)를 만들어 `UWxCombatLibrary::ApplyDamage`에 넘긴다. 상태이상 등은 `AdditionalEffects`로 얹는다.
- **리플리케이션/권한**: 대미지 GE는 Instant+Execution이라 서버 권위이며, ExecCalc의 부수효과·어트리뷰트 확정은 서버에서만 일어난다. 클라 예측 경로는 ExecCalc를 건너뛰므로 `EWxDamageResult::None`이 되고 발행부가 연출을 스킵한다. 히트스톱만은 `UWxExecCalc_Damage::CheckDamage`를 직접 돌려 클라/서버가 같은 결론에 도달한다.
- **글로벌 등록 필수**: `UWxAbilitySystemGlobals`를 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`에 등록해야 모든 EffectContext가 `FWxCombatEffectContext`로 할당된다. 빠지면 판정결과가 실리지 못하고 ExecCalc가 ensure로 알린다.

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력 라우팅·히트스톱·Cue 발행이 모이는 허브. 전투 제어 흐름의 중심.
2. `Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` + `Damage/WxCombatEffectContext.h` — 대미지 판정과 그 결과가 발행부로 넘어가는 파일 횡단 통로. 함께 읽어야 흐름이 잡힌다.
3. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` + `WxAbilityTableRow.h` — 어빌리티 베이스와 Row 기반 쿨다운/코스트 규약.

## 관련
- 상위: 이 플러그인의 ASC·AbilitySet·AttributeSet을 소유·구성하는 캐릭터/플레이어 계층(게임 모듈)과, 어빌리티를 켜는 콘텐츠(GameFeature) 측이 소비처다.

---
*문서 기준 커밋 `de46ee7` · 생성일 2026-08-11 · 소스 153파일 — `/readme-writer`로 갱신*
