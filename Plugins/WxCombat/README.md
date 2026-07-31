# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반의 액션 RPG 전투 도메인. 어빌리티·이펙트·어트리뷰트, 락온 타게팅, 무기/투사체 히트, 애님 노티파이 구동, 시간 감속을 담당한다.

## 책임
**담당**
- 어빌리티 파이프라인: 공격/스킬/회피/가드/궁극기/그로기/피격/사망 등 어빌리티(`AbilitySystem/Ability/`), 전용 ASC의 입력 라우팅
- 어트리뷰트·이펙트: `UWxCombatAttributeSet`(HP/SP/DP/MP/UP/ATK/DEF 등), Damage/Cost/Cooldown/Burn 등 GameplayEffect·ExecCalc·MMC(`AbilitySystem/Effect/`)
- 데미지 산출·적용: `FWxDamageInfo`→Spec 변환, `WxExecCalc_Damage`, 무기(`AWxWeaponBase`)·투사체(`AWxProjectileBase`)·광역존(`AWxEffectZone`) 히트 경로
- 타게팅/락온: `UWxLockOnManagerComponent`, TargetingSystem 필터 태스크(`Targeting/`), 스냅 루트모션
- 애니메이션 구동 노티파이(`AnimNotify/`): 콤보 윈도우·무기 공격·무적·퍼펙트가드·투사체 스폰 등
- 연출: `UWxTimeDilationComponent`(히트스톱/슬로우), GameplayCue(`AbilitySystem/Cue/`)

**경계 (비담당)**
- 공용 정의·태그·팀/캐릭터 기반은 [[WxCore]] 참조 (WxCore 외 Wx 플러그인 비참조)
- 캐릭터/폰·입력 소유 및 AbilitySet 지정은 게임 모듈/캐릭터 BP 측에서 수행

## 의존성
- **주요 의존**: [[WxCore]] · 엔진 서브시스템 GameplayAbilities, TargetingSystem, MotionWarping, ModularGameplay, EnhancedInput
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (uplugin/Build.cs 확인)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 쿨다운/코스트를 `FWxAbilityTableRow` 데이터 주도로 처리 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySystemComponent` | 전용 ASC. 입력 액션→어빌리티 라우팅, 입력 대기 방송 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilitySet` | 어빌리티/이펙트/어트리뷰트 초기값을 한 에셋으로 일괄 부여 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | 캐릭터 스탯 어트리뷰트 세트(+ Meta `IncomingDamage`) | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 데미지 한 건의 설계 데이터→Effect Spec 변환 | `Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxExecCalc_Damage` | 데미지 최종 산출(ExecutionCalculation) | `Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |
| `AWxWeaponBase` / `AWxProjectileBase` | 무기 스윙·투사체 히트 콜리전 및 데미지 전달 | `Source/WxCombat/Public/Weapon/` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent) 복제 관리 | `Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |
| `UWxCombatLibrary` | 무기 외 경로의 단일 데미지 적용 진입점(`ApplyDamage`) | `Source/WxCombat/Public/WxCombatLibrary.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase` 상속. 입력 발동은 `ActivationInputAction` 지정, AI/반응형/패시브는 비움. 쿨다운/코스트 수치는 오버라이드 없이 `FWxAbilityTableRow`(AbilityDataRow)에서 조회 — 공용 `UWxEffect_Cooldown`/`UWxEffect_Cost` GE가 엔진 순정 경로로 적용. `CooldownGameplayEffectClass`를 다른 GE로 바꾸면 순정 GE 경로로 전환(상호배타).
- 새 이펙트: `AbilitySystem/Effect/`에 GE/ExecCalc/MMC 추가. 데미지는 `FWxDamageInfo`→`MakeSpecs`로 SetByCaller·DynamicAssetTags를 실어 전달.
- 데이터 주도 설정: `UWxAbilitySet`(GrantedAbilities/Effects + AttributeInitRow), `FWxDamageTableRow`, `FWxCombatAttributeInitTableRow` DataTable.
- 리플리케이션/권한: HP/SP/DP/MP/UP 등 어트리뷰트는 서버 권위 복제. 락온 대상·최근 입력도 서버 권위이며 소유 클라는 즉시 예측 후 서버 요청·복제 정합. 데미지 적용은 권위 측 GE.
- Gameplay Tag는 C++ Native 선언 없이 에디터/에셋에서 관리(`Event.HitReact.*` 등 메타 카테고리로 참조).

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티 계약(활성화 정책·입력·쿨다운/코스트 데이터 규약)의 핵심
2. `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력이 어빌리티로 흘러가는 라우팅 경계
3. `Source/WxCombat/Public/WxDamageInfo.h` + `Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` — 데미지 데이터→Spec→산출의 파일 횡단 흐름
4. `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 스탯 정의와 피격/사망 파이프라인(`IncomingDamage`→HP 차감)

## 관련
- 상위: [[WxCore]] (공용 정의) · Experience/캐릭터 BP가 `UWxAbilitySet`을 지정해 이 시스템을 부팅

---
*문서 기준 커밋 `c549ea2` · 생성일 2026-07-31 · 소스 147파일 — `/readme-writer`로 갱신*
