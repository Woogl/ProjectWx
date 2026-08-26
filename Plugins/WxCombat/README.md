# WxCombat — GAS 기반 액션 RPG 전투 시스템

> Gameplay Ability System 위에 얹은 근접/원거리 전투 도메인. 어빌리티 발동·입력 라우팅, 어트리뷰트, 대미지 판정, 무기/투사체, 락온, 히트/타이밍 연출을 한 모듈에 담는다.

## 책임
**담당**
- 어빌리티 발동의 유일한 진입점(`UWxAbilitySystemComponent`)과 발동 그룹 배타 점유·입력 버퍼·히트스톱
- 어빌리티 베이스와 파생(공격·회피·가드·스킬·궁극·피격반응·그로기·사망·패턴 등), 데이터 주도 쿨다운/코스트
- 전투 어트리뷰트(HP·SP·DP·MP·UP·ATK·DEF·CritRate 등)와 피격 후처리(가드 해제·큐·이벤트)
- 대미지 단일 진입점(`UWxCombatLibrary::ApplyDamage`)과 계산(`UWxExecCalc_Damage`), 판정 결과 전달용 EffectContext
- 무기 히트박스 스윙 판정·투사체, 락온 대상 복제, 타겟팅 필터, 타임딜레이션/모션워핑

**경계 (비담당)**
- Gameplay Tag 네이티브 정의 — [[WxCore]](`WxGameplayTags.h`)를 참조만 하고 이 모듈은 태그를 선언하지 않는다

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 입력→어빌리티 라우팅·발동 그룹 배타 점유·히트스톱의 중심 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티의 Abstract 베이스, 활성화 그룹 축·데이터 주도 쿨다운 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | 전투 어트리뷰트 보관, `PostGameplayEffectExecute`에서 피격 후처리·연출 분기 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxCombatLibrary` | `ApplyDamage`·`IsHostile`·상태 GE 착탈 등 대미지 경로 공용 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `UWxExecCalc_Damage` | 무적·가드·퍼펙트가드 분기와 최종 대미지 산출(`UWxEffect_Damage`와 한 쌍) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h` |
| `UWxAbilitySet` | 어빌리티·이펙트·어트리뷰트 초기값을 서버에서 ASC에 일괄 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `AWxWeaponBase` | 무기 히트박스 스윙(Overlap+Sweep) 판정, ANS가 여닫음 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxLockOnManagerComponent` | 락온 대상을 SceneComponent 단위로 서버 권위 복제 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- 새 어빌리티는 `UWxAbilityBase` 파생으로 만들고 `AbilityDataRow`(`FWxAbilityTableRow`)로 쿨다운/코스트/아이콘을 잇는다. 발동 배타성은 `EWxAbilityActivationGroup`(Exclusive_Blocking→ComboWindow→Recovery 축, Reaction은 뚫음)으로 선언하고, 콤보 창/후딜 전이는 몽타주 노티파이가 `OpenComboWindow`/`StartRecovery`를 호출해 런타임 전환한다.
- 캐릭터 BP는 `UWxAbilitySet` 하나만 지정하면 `InitAbilitySystem` 시점에 어빌리티·GE·어트리뷰트 초기값이 서버에서 일괄 부여된다.
- 대미지는 어트리뷰트를 직접 쓰지 말고 항상 `UWxCombatLibrary::ApplyDamage`로 보낸다. 밸런스 수치는 `FWxDamageTableRow`(계수·자원·`HitReactTag`·가드/패리 플래그·`AdditionalEffects`)로 데이터 주도한다.
- 커스텀 EffectContext(`FWxCombatEffectContext`, 크리 플래그 운반)는 `UWxAbilitySystemGlobals`가 할당한다 — `DefaultGame.ini`의 `AbilitySystemGlobalsClassName` 등록이 빠지면 판정 결과가 실리지 못한다.
- 리플리케이션: ExecCalc는 서버 권위(대미지 GE는 Instant+Execution이라 예측 시 건너뜀), 지속형 `AdditionalEffects`만 예측된다. 락온 대상은 서버 권위 복제, 시선 입력은 로컬 전용.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력이 어빌리티로 흘러 들어가는 라우팅·버퍼·배타 점유 규칙의 출발점
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 활성화 그룹 축과 데이터 주도 쿨다운, 모든 어빌리티가 공유하는 계약
3. `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` → `Effect/WxEffect_Damage.h` → `Attribute/WxCombatAttributeSet.h` — 히트에서 대미지 적용·연출까지 파일 횡단 흐름

## 관련
- 상위: [[WxCore]] (공용 정의·Gameplay Tag)

---
*문서 기준 커밋 `718b827` · 생성일 2026-08-26 · 소스 148파일 — `/readme-writer`로 갱신*
