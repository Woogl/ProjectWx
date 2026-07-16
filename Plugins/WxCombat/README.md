# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반 액션 RPG 전투의 런타임을 책임진다. 어빌리티·어트리뷰트·데미지 파이프라인, 락온/타게팅, 무기·투사체, AnimNotify 기반 히트 판정, 히트스톱/슬로우모션까지 전투 한 판에 필요한 모든 서버 권위 로직을 담는다.

## 책임
**담당**
- 어빌리티 실행 모델: `UWxAbilityBase` 파생 어빌리티(공격·스킬·가드·회피·궁극기·피격·사망·AI 패턴 등)와 공용 쿨다운/코스트 GE 처리.
- 어트리뷰트/스탯: `UWxCombatAttributeSet`(HP/SP/DP/MP/UP + ATK/DEF/Crit/SPD/ASPD)과 리플리케이션·클램프.
- 데미지 파이프라인: `FWxDamageInfo` → GameplayEffect Spec 변환 → ExecutionCalculation(`WxExecCalc_Damage`, `WxExecCalc_Burn`)으로 최종 데미지 산출.
- GameplayEffect/MMC/GameplayCue 카탈로그(`AbilitySystem/Effect`, `AbilitySystem/Cue`).
- 타게팅/락온: `UWxLockOnManagerComponent`, TargetingSystem 필터 태스크, 루트모션 스냅.
- 무기·투사체(`AWxWeaponBase`, `AWxProjectileBase`)와 AnimNotify 기반 콜리전/이벤트 트리거.
- 전투 연출 시간 제어: `UWxTimeDilationComponent`(슬로우모션), 히트스톱 큐.

**경계 (비담당)**
- 어트리뷰트/태그 등 공용 정의·베이스 타입은 [[WxCore]]에 위임(유일한 Wx 의존).
- UI 표시(아이콘·게이지)는 어빌리티에 부착하는 `UWxAbilityComponent` 파생으로 다른 도메인(예: WxUI)이 확장 — 이 모듈은 표시 데이터를 갖지 않는다.
- 입력 바인딩 자체는 EnhancedInput/게임 모듈 소관이며, 여기서는 InputTag 라우팅만 담당.

## 의존성
- **주요 의존**: `WxCore` · GameplayAbilities(GAS) · TargetingSystem · MotionWarping · ModularGameplay · GameplayTasks · AIModule · EnhancedInput · NavigationSystem · Niagara · UMG · NetCore · LevelSequence/MovieScene
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (`Build.cs`의 Public/PrivateDependencyModuleNames에 다른 Wx 플러그인 없음)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 쿨다운/코스트/테이블 Row/컴포넌트 확장 규약의 중심 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySystemComponent` | 프로젝트 ASC. 입력 태그 라우팅, AbilitySet 부여, 래그돌 복제 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxCombatAttributeSet` | 전투 스탯 어트리뷰트 세트와 데미지 후처리(HP 차감) | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxAbilitySet` | Ability/Effect/Attribute 초기값을 묶어 ASC에 일괄 부여하는 DataAsset | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `FWxDamageInfo` | 데미지 한 건의 설계 데이터 → GE Spec 변환 진입점 | `Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxCombatLibrary` | 무기/투사체 외 경로의 데미지 적용·적대 판정 BP 라이브러리 | `Source/WxCombat/Public/WxCombatLibrary.h` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent 단위)을 서버 권위로 복제 관리 | `Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |
| `AWxWeaponBase` | 무기 액터. AnimNotify 스윙 구간 동안 Overlap 히트 판정 | `Source/WxCombat/Public/Weapon/WxWeaponBase.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase`를 상속(BP도 가능). 쿨다운은 `CooldownTime`/`MaxRecharges`, 코스트는 `MPCost`/`UPCost` 프로퍼티로 데이터 주도 설정하고, 공용 `UWxEffect_Cooldown`/`UWxEffect_Cost` GE가 소스 CDO 기준으로 처리한다.
- **데이터 주도**: `AbilityDataRow`(`FWxAbilityTableRow`)로 어빌리티 수치를, `UWxAbilitySet`의 `AttributeInitRow`(`FWxCombatAttributeInitTableRow`)로 어트리뷰트 초기값을, `FWxDamageInfo`/`FWxDamageTableRow`로 데미지 수치를 테이블에서 읽는다.
- **새 이펙트/큐/MMC**: `AbilitySystem/Effect`·`AbilitySystem/Cue`에 GE/ExecCalc/GameplayCue를 추가. 데미지는 `IncomingDamage` 메타 어트리뷰트를 통해 `PostGameplayEffectExecute`에서 HP로 반영되는 패턴을 따른다.
- **어빌리티 부가 데이터**: `UWxAbilityBase::Components`에 `UWxAbilityComponent` 파생을 EditInline으로 부착해 도메인 확장(UI 데이터 등).
- **리플리케이션/권한**: 서버 권위 모델. 어트리뷰트·락온 대상·래그돌은 복제되며, 투사체 스폰과 데미지 확정은 서버(authority)에서만 수행하고 소유 클라는 예측 후 정합한다.

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티 실행/쿨다운/코스트 규약의 중심. 파생 어빌리티들을 이해하는 출발점.
2. `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 스탯 약어(HP/SP/DP/MP/UP…)와 데미지가 어트리뷰트로 반영되는 방식.
3. `Source/WxCombat/Public/WxDamageInfo.h` + `Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp` — 데미지 데이터가 Spec을 거쳐 최종 수치로 계산되는 전 과정.
4. `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` — 캐릭터가 어빌리티/이펙트/어트리뷰트를 어떻게 부여받는지.

## 관련
- 상위: 캐릭터/폰이 `UWxAbilitySystemComponent`와 `UWxAbilitySet`을 통해 이 시스템을 구동하며, AI 패턴 어빌리티는 [[WxAI]], 표시 데이터 확장은 [[WxUI]]가 소비한다(코드 의존 아님).
- 공용 정의·베이스는 [[WxCore]].

---
*문서 기준 커밋 `08c2d0c` · 생성일 2026-07-16 · 소스 151파일 — `/readme-writer`로 갱신*
