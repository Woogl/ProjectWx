# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 얹은 액션 RPG 전투 코어. 어빌리티 발동·배타 점유, 대미지 판정과 상태 전이(가드·패리·그로기·사망), 무기/투사체 히트, 락온·모션워핑을 담당한다.

## 책임
**담당**
- 어빌리티 라우팅과 발동 그룹 배타 점유(콤보 창·후딜·반응), 쿨다운·코스트·히트스톱 (`WxAbilitySystemComponent`, `WxAbilityBase`)
- 전투 어트리뷰트(HP/SP/DP/MP/UP/ATK 등)와 피격 후처리 — 대미지·가드·퍼펙트 가드 결과를 실제 상태 변화·연출로 옮김 (`WxCombatAttributeSet`)
- 대미지 단일 진입점과 계산 — 적중 성립 판정, ExecCalc, 데이터 주도 밸런스 (`WxCombatLibrary::ApplyDamage`, `WxExecCalc_Damage`, `FWxDamageTableRow`)
- 어빌리티/이펙트/큐/태스크 구현체와 몽타주 노티파이(무기 판정·콤보 창·투사체 스폰 등)
- 무기·투사체 히트 판정, 락온, 모션워핑 스냅, 시간 지연(히트스톱·슬로우)

**경계 (비담당)**
- 팀/적대 관계의 원천(GenericTeamAgent 등)은 [[WxCore]]에 두고 `IsHostile`이 이를 소비
- Gameplay Tag 네이티브 선언은 이 모듈에 없음 — 태그 정의는 [[WxCore]] 참조
- 캐릭터/폰 소유·AI 구동은 소비 측(게임 모듈·[[WxAI]])에 위임

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 입력→어빌리티 라우팅의 유일한 진입점, 발동 그룹 점유·히트스톱 | `Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티의 추상 베이스 — 발동 정책/그룹, 쿨다운·코스트, 몽타주 | `Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | 전투 어트리뷰트 + 피격/퍼펙트가드 후처리 | `Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxAbilitySet` | 캐릭터에 어빌리티·이펙트·어트리뷰트 초기값을 일괄 부여하는 데이터 애셋 | `Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatLibrary` | 대미지·상태 GE의 공용 진입점(`ApplyDamage`/`ApplyEffect`) | `Public/WxCombatLibrary.h` |
| `UWxExecCalc_Damage` | 대미지 계산과 적중 성립 선판정(`CheckDamage`) | `Public/AbilitySystem/Effect/WxEffect_Damage.h` |
| `FWxDamageTableRow` | 공격별 밸런스(계수·반응 태그·추가 이펙트) 데이터 Row | `Public/Damage/WxDamageTableRow.h` |
| `AWxWeaponBase` | 무기 히트박스 판정(Overlap+Sweep), 스윙당 1회 피격 | `Public/Weapon/WxWeaponBase.h` |
| `UWxLockOnManagerComponent` | SceneComponent 단위 락온 대상, 서버 권위 복제 | `Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase` 상속(Blueprintable). `ActivationPolicy`(OnTriggered/OnGiven)·`ActivationGroup`(Independent / Exclusive_Blocking·ComboWindow·Recovery / Reaction)로 발동 배타 규칙을 선언. 쿨다운·코스트는 `AbilityDataRow`(`FWxAbilityTableRow`)에서 읽어 공용 GE 사용.
- 발동 그룹은 몽타주 노티파이가 런타임 전환(`OpenComboWindow`/`StartRecovery`)하는 하나의 축이다 — Blocking→ComboWindow→Recovery 순으로 잠금이 풀린다.
- 대미지·상태이상은 데이터 주도: 공격 몽타주 RowName을 `FWxDamageTableRow`에 매핑(`CoeffATK`, `HitReactTag`, `bCanGuard`/`bCanParry`, `AdditionalEffects`). 무기 판정은 `WxAnimNotifyState_WeaponAttack`이 `BeginAttack/EndAttack`으로 연다.
- 대미지 판정 결과의 크리 여부는 `FWxCombatEffectContext`로 실려 흐른다 — `UWxAbilitySystemGlobals`가 컨텍스트를 할당하므로 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName` 등록이 필수.
- 리플리케이션: 어트리뷰트는 서버 권위. 대미지 GE는 Instant+Execution이라 예측 시 execution을 건너뛰고, 예측되는 것은 Row의 지속형 `AdditionalEffects`뿐. 락온은 서버 권위 복제.

## 여기서부터 읽어라
1. `Public/AbilitySystem/Ability/WxAbilityBase.h` — 발동 정책/그룹 enum과 몽타주·쿨다운 계약. 전투 흐름의 뼈대.
2. `Public/WxCombatLibrary.h` — `ApplyDamage`가 모든 대미지 경로의 단일 진입점. 여기서 적중 성립·히트스톱·예측 키를 잡는다.
3. `Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — `ProcessDamageTaken`/`ProcessPerfectGuard`가 계산 결과를 실제 상태·연출로 옮기는 종착점.
4. `Public/AbilitySystem/Effect/WxEffect_Damage.h` — 무적/가드/퍼펙트가드/가드불가별 분기 판정.

## 관련
- 참조: [[WxCore]] (공용 정의·팀·태그) — Wx 플러그인 중 유일한 의존
- 상위: 캐릭터·폰을 소유하는 게임 모듈, AI([[WxAI]]), 어트리뷰트/자원을 소비하는 UI([[WxUI]])

---
*문서 기준 커밋 `c4db6c0` · 생성일 2026-08-25 · 소스 148파일 — `/readme-writer`로 갱신*
