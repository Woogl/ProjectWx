# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반의 액션 RPG 전투 도메인. 어빌리티·어트리뷰트·이펙트로 캐릭터의 공격/방어/피격/사망을 구동하고, 대미지 파이프라인·락온 타게팅·무기/투사체 히트 판정·히트스톱을 제공한다.

## 책임

**담당**
- 어빌리티 실행 프레임워크: `UWxAbilitySystemComponent`(입력 라우팅) + `UWxAbilityBase`(공용 베이스, DataRow 기반 쿨다운/코스트/충전) + 구체 어빌리티(Attack/Dodge/Guard/Skill/Ultimate/Finisher/Groggy/HitReact/Death/LockOn/Sprint/Pattern)
- 캐릭터 스탯: `UWxCombatAttributeSet` (HP/SP/DP/MP/UP, ATK/DEF/Crit/SPD/ASPD, 메타 IncomingDamage)
- 대미지 파이프라인: `FWxDamageInfo` → GE Spec 변환 → `WxExecCalc_Damage` 실행 계산, `UWxCombatLibrary::ApplyDamage`/`ApplyRawDamage` 진입점
- GameplayEffect/MMC/ExecCalc/Cue 라이브러리(버프·디버프·화상·코스트·쿨다운·회복·반사 등)
- 애님 노티파이 기반 전투 타이밍(콤보 윈도우, 무적, 퍼펙트가드, 무기 공격 구간, 투사체/광역 스폰, 후딜 진입)
- 락온 타게팅(`UWxLockOnManagerComponent` + `TargetingSystem` 필터 태스크들), 모션워핑 타겟 스냅, 히트스톱/슬로모(`UWxTimeDilationComponent`)
- 무기/투사체/이펙트존 액터(`AWxWeaponBase`, `AWxProjectileBase`, `AWxEffectZone`)

**경계 (비담당)**
- 캐릭터 클래스·플레이어 입력 바인딩 주체·AI 두뇌는 이 모듈 밖(어빌리티가 요구하는 InputAction 목록만 노출). 소비처가 `GetAbilityInputActions()`로 EnhancedInput에 바인딩한다.
- 공용 정의(팀/태그 등 foundation)는 [[WxCore]]에 위임.

## 의존성
- **주요 의존**: [[WxCore]], GameplayAbilities(GAS), GameplayTags/GameplayTasks, TargetingSystem, MotionWarping, EnhancedInput, ModularGameplay, AIModule/NavigationSystem, Niagara·LevelSequence/MovieScene(private), UMG
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (`.uplugin`·`Build.cs`·인클루드 전수 확인)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | ASC 서브클래스. 입력→어빌리티 라우팅, LastPressedInputAction 복제, AbilitySet 부여 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilitySet` | 어빌리티/이펙트/어트리뷰트 초기값을 한 에셋으로 묶어 ASC에 일괄 부여하는 DataAsset | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxAbilityBase` | 모든 어빌리티의 부모. 쿨다운/코스트/충전을 `WxAbilityTableRow`에서 온디맨드로 해석, 후딜·히트스톱 공용 로직 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | 전투 스탯 전체와 복제/클램프/데미지 후처리(`PostGameplayEffectExecute`) | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxCombatLibrary` | 무기/투사체 밖 경로의 대미지 적용 진입점(BP Function Library) | `Source/WxCombat/Public/WxCombatLibrary.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터. GE Spec(SetByCaller/AssetTag)으로 변환하는 허브 | `Source/WxCombat/Public/WxDamageInfo.h` |
| `AWxWeaponBase` | 근접 무기 액터. ANS_WeaponAttack이 여는 공격 구간 동안 Overlap+Sweep 히트 판정 | `Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent 단위)을 서버 권위로 복제·브로드캐스트 | `Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase`를 상속(C++ 또는 BP). 쿨다운/코스트/충전/아이콘 수치는 `AbilityDataRow`(`FWxAbilityTableRow`)에 데이터로 넣고, 공용 `UWxEffect_Cooldown`/`UWxEffect_Cost` 마커 GE를 그대로 두면 프로젝트 방식(DataRow 기반)이 적용된다. 다른 GE로 바꾸면 엔진 순정 경로로 전환.
- **활성화 정책**: `EWxAbilityActivationPolicy`(OnTriggered/OnGranted). 입력 발동 어빌리티는 `ActivationInputAction` 지정, 복수 입력은 `IsActivationInput`/`GetInputActions` override.
- **캐릭터 셋업**: `UWxAbilitySet` 에셋에 GrantedAbilities/GrantedEffects/AttributeInitRow를 채워 캐릭터 BP의 ASC에 지정 → InitAbilityActorInfo 시점에 일괄 부여.
- **새 이펙트**: `AbilitySystem/Effect/`의 `UWxEffect_*` GE, 계산은 `WxExecCalc_*`(ExecutionCalculation)·`WxMMC_*`(ModMagnitudeCalc)로 추가.
- **전투 타이밍**: `AnimNotify/`의 ANS/AN을 몽타주에 배치해 콤보 윈도우·무적·퍼펙트가드·무기 공격 구간·투사체 스폰·후딜 진입 제어.
- **타게팅 필터**: `Targeting/WxTargetingFilterTask_*`가 `TargetingSystem`의 필터 태스크. Team/LineTrace/ScreenBounds/InputDirection/GameplayTag 조합으로 락온 후보 선별.
- **리플리케이션**: 어트리뷰트는 서버 권위 복제, 대미지·투사체 스폰은 서버 확정. 입력/락온은 소유 클라 예측 후 서버 권위 정합.
- **Native Gameplay Tag**: C++ 선언 없음(문자열 태그 기반). `Event.HitReact.*`, `SetByCaller.*`, `Coeff.ATK`, `Recovery.MP/UP` 등은 코드에서 문자열/메타 카테고리로 참조.

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티 프레임워크의 계약(쿨다운/코스트/충전/후딜/히트스톱)이 헤더 주석에 응축돼 있다. 모듈의 심장.
2. `Source/WxCombat/Public/WxDamageInfo.h` + `Source/WxCombat/Public/WxCombatLibrary.h` — 대미지가 데이터에서 GE Spec으로 흐르는 경로. `WxExecCalc_Damage.cpp`로 이어짐.
3. `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` — 캐릭터가 어떻게 어빌리티/스탯을 부여받는지(셋업 진입점).

## 관련
- 상위: 캐릭터/AI/플레이어 컨트롤러가 `UWxAbilitySystemComponent`와 `UWxAbilitySet`으로 이 모듈을 소비. 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `c275320` · 생성일 2026-07-24 · 소스 149파일 — `/readme-writer`로 갱신*
