# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반 액션 RPG 전투 도메인. 어빌리티·어트리뷰트·이펙트·타게팅·무기·AnimNotify를 묶어 캐릭터의 공격/피격/스킬/락온/그로기 흐름을 구현한다.

## 책임
**담당**
- ASC/AbilitySet 부여 파이프라인, 입력→어빌리티 라우팅 (`UWxAbilitySystemComponent`, `UWxAbilitySet`)
- 전투 어트리뷰트(HP/SP/DP/MP/UP/ATK/DEF/Crit/SPD/ASPD)와 데미지 계산·메타 어트리뷰트 처리 (`UWxCombatAttributeSet`, `WxExecCalc_*`, `WxMMC_*`)
- 어빌리티 세트(공격/회피/가드/스킬/궁극/피격/그로기/사망/락온/스프린트 등)와 공용 GameplayEffect
- 무기/투사체 히트 판정, 락온·타게팅 필터, AnimNotify 기반 전투 이벤트 발행, 히트스톱/타임딜레이션

**경계 (비담당)**
- 공용 정의·기반 타입 — [[WxCore]]
- AI 의사결정·패턴 구동 로직 — [[WxAI]] (본 모듈은 어빌리티/타게팅 실행부만 제공)

## 의존성
- **주요 의존**: `WxCore` · `GameplayAbilities` · `GameplayTags` · `GameplayTasks` · `ModularGameplay` · `TargetingSystem` · `MotionWarping` · `EnhancedInput` · `AIModule`/`NavigationSystem`
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 프로젝트 ASC. 입력 액션을 어빌리티로 라우팅, AbilitySet 부여·최근 입력 추적 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilitySet` | Ability/Effect/Attribute 초기값을 한 에셋으로 묶어 ASC에 일괄 부여하는 DataAsset | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 쿨다운/코스트/충전을 DataTable(`FWxAbilityTableRow`)로 데이터 주도, 히트스톱 처리 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | 전투 어트리뷰트 세트. IncomingDamage 메타로 데미지 수신 후 HP 차감 | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터. GE Spec(SetByCaller/AssetTags)으로 변환 | `Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxCombatLibrary` | 무기/투사체 외 경로의 단일 데미지 적용 진입점(`ApplyDamage`) | `Source/WxCombat/Public/WxCombatLibrary.h` |
| `AWxWeaponBase` / `AWxProjectileBase` | 무기 스윙 오버랩 히트 판정 / 서버 스폰 투사체 | `Source/WxCombat/Public/Weapon/` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent 단위)을 서버 권위로 복제 관리 | `Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase`를 상속하고 쿨다운/코스트/충전/아이콘은 공용 GE(`UWxEffect_Cooldown`/`UWxEffect_Cost`)를 마커로 둔 채 `AbilityDataRow`(DataTable)에 채운다. 다른 GE로 바꾸면 엔진 순정 경로로 전환(상호배타).
- 입력 발동 어빌리티는 `ActivationInputAction`을 설정하고, 복수 입력은 `IsActivationInput`/`GetInputActions`를 override. AI·반응형·패시브는 비워둔다(`OnGranted` 정책은 부여 즉시 자동 발동).
- 데미지: `FWxDamageInfo` → `MakeSpecs` → `WxExecCalc_Damage`가 ATK/DEF/Crit/HitStop을 계산해 IncomingDamage 메타에 실음. 광역·환경 등 무기 외 경로는 `UWxCombatLibrary::ApplyDamage`.
- 타게팅: `TargetingSystem`의 필터 태스크(`WxTargetingFilterTask_*`: Team/LineTrace/ScreenBounds/GameplayTag/InputDirection)로 락온 후보를 조합.
- 리플리케이션: 어트리뷰트는 ReplicatedUsing, 락온 대상·투사체는 서버 권위 복제(소유 클라 예측 반영).

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력이 어떻게 어빌리티로 흘러가는지, 부여 진입점
2. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티 공통 규약(쿨다운/코스트/충전/히트스톱)의 데이터 주도 설계
3. `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 스탯 정의와 데미지 수신 파이프라인
4. `Source/WxCombat/Public/WxDamageInfo.h` + `Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp` — 대미지 데이터가 최종 수치가 되는 경로

## 관련
- 상위: [[WxCore]] (공용 정의) · 소비 측 [[WxAI]]

---
*문서 기준 커밋 `1bd11a9` · 생성일 2026-07-26 · 소스 147파일 — `/readme-writer`로 갱신*
