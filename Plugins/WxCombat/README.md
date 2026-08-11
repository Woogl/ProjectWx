# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 올린 액션 RPG 전투의 중심 모듈. 어빌리티·이펙트·어트리뷰트로 캐릭터의 공격·방어·피격·자원 흐름을 구성하고, 락온·히트스톱·타임딜레이션 같은 전투 연출과 무기/투사체 히트 판정을 담당한다.

## 책임
**담당**
- 어빌리티 라우팅과 부여: 입력/이벤트/AI 트리거 → 어빌리티 발동, `WxAbilitySet`으로 어빌리티·이펙트·어트리뷰트 초기값을 ASC에 일괄 부여
- 전투 어트리뷰트(`UWxCombatAttributeSet`): HP/SP/DP/MP/UP, ATK/DEF/Crit, SPD/ASPD와 복제·클램프·사망/그로기 판정
- 대미지 파이프라인: `FWxDamageInfo`(데이터) → Damage GE Spec → `WxExecCalc_Damage`(무적·가드·퍼펙트가드·크리 분기)
- 무기/투사체 히트 판정(`AWxWeaponBase`, `AWxProjectileBase`)과 히트스톱(역경직)
- 타게팅/락온(`UWxLockOnManagerComponent`, TargetingSystem 필터 태스크), MotionWarping 기반 타겟 스냅
- 전투 연출용 AnimNotify·GameplayCue, 전역 타임딜레이션(`UWxTimeDilationComponent`)

**경계 (비담당)**
- 플레이어 입력 매핑 컨텍스트 소유·캐릭터 클래스 정의는 소비 측(게임/캐릭터 모듈)에 위임 — 이 모듈은 어빌리티가 요구하는 `UInputAction` 목록만 제공
- 컴포넌트 주입 설정(Experience) 자체는 GameFeature/게임 모듈 소관

## 의존성
- **주요 의존**: `WxCore` · GameplayAbilities · GameplayTags · GameplayTasks · ModularGameplay · EnhancedInput · TargetingSystem · MotionWarping · AIModule/NavigationSystem · NetCore · UMG (private: Niagara · LevelSequence · MovieScene)
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (참조하는 유일한 Wx 플러그인은 `WxCore`)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 전투용 ASC. 입력 액션을 어빌리티로 라우팅하고, 대미지 적용 연출 발행과 히트스톱을 담당 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. Row 기반 쿨다운/코스트, 후딜(캔슬)·투사체 스폰 규약 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 캐릭터가 지정하는 부여 묶음(어빌리티·이펙트·어트리뷰트 초기 Row) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | 전투 스탯 어트리뷰트 세트와 복제/파생 처리 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터 → GE Spec 변환 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageInfo.h` |
| `UWxExecCalc_Damage` | 대미지 최종 계산과 가드/무적/크리 분기 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |
| `AWxWeaponBase` | AnimNotify 구동 무기 히트 판정(스윙당 1회) | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent 단위) 복제·예측 관리 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase`를 상속(`Public/AbilitySystem/Ability/`의 `WxAbility_*`가 예시). 쿨다운/코스트는 GE를 새로 만들지 말고 `AbilityDataRow`(`WxAbilityTableRow`)에 수치를 채워 공용 GE 마커를 그대로 두면 Row 경로가 동작한다. 입력 발동이면 `ActivationInputAction` 지정, AI/반응형/패시브면 비운다.
- **새 이펙트/실행계산**: `Public/AbilitySystem/Effect/`의 `WxEffect_*`(GE), `WxExecCalc_*`, `WxMMC_*` 패턴을 따른다. 대미지에 상태이상을 얹으려면 `FWxDamageInfo::AdditionalEffects`에 GE를 추가.
- **데이터 주도**: 캐릭터는 `UWxAbilitySet` 하나로 부여 묶음을 지정하고, 어트리뷰트 초기값은 `WxCombatAttributeInitTableRow`, 대미지는 `WxDamageTableRow`로 관리.
- **리플리케이션/권한**: 대미지·타임딜레이션·락온은 서버 권위. `UWxCombatLibrary::ApplyDamage`가 모든 대미지의 단일 진입점이며 어빌리티 활성화 키로 클라 예측을 얹는다(실제 예측되는 것은 지속형 AdditionalEffects뿐).
- **연출 훅**: `Public/AnimNotify/`(콤보 윈도우·무적·무기 판정·투사체 스폰 등)와 `Public/AbilitySystem/Cue/`(GameplayCue)로 몽타주가 전투 로직·이펙트를 구동.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력→어빌리티 라우팅과 히트스톱, 전투의 제어 진입점
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 모든 어빌리티가 공유하는 쿨다운/코스트/후딜 규약
3. `Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageInfo.h` + `AbilitySystem/Effect/WxExecCalc_Damage.h` — 대미지가 데이터에서 최종 적용까지 흐르는 경로
4. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 전투에서 다루는 스탯 전부의 정의

## 관련
- 상위: 캐릭터/게임 모듈(`WxGame`)과 GameFeature 콘텐츠 플러그인이 이 모듈의 ASC·AbilitySet·컴포넌트를 주입해 사용
- 함께 보는 모듈: 공용 정의·기반은 [[WxCore]], 스탯·상태 표시는 [[WxUI]], 적 행동 트리거는 [[WxAI]]

---
*문서 기준 커밋 `1ec70f2` · 생성일 2026-08-10 · 소스 143파일 — `/readme-writer`로 갱신*
