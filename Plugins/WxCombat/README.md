# WxCombat — 전투 시스템

> Gameplay Ability System(GAS) 기반의 액션 RPG 전투 도메인. 어빌리티·이펙트·어트리뷰트·데미지 파이프라인, 락온 타겟팅, 무기/투사체, 히트스톱·슬로우 등 전투 연출을 담당한다.

## 책임
**담당**
- GAS 골격: `UWxAbilitySystemComponent`(입력 태그 라우팅·래그돌 복제), `UWxAbilityBase`(공용 쿨다운/코스트 규약), `UWxCombatAttributeSet`(HP/SP/DP/MP/UP/ATK/DEF 등 전투 스탯), `UWxAbilitySet`(어빌리티·이펙트·어트리뷰트 일괄 부여 데이터에셋).
- 전투 어빌리티 구현군: 공격·회피·가드·스킬·궁극기·히트리액트·그로기·처형·AI 패턴 등 (`Public/AbilitySystem/Ability/`).
- 데미지 파이프라인: `FWxDamageInfo` → GameplayEffect Spec 변환 → `WxExecCalc_Damage` 실행 계산 → `IncomingDamage` 메타 어트리뷰트로 HP 차감. 상태이상(Burn/Exceed/BuffATK 등) GE·MMC·ExecCalc.
- 데미지 전달 매체: `AWxWeaponBase`(스윕 히트 판정), `AWxProjectileBase`(투사체), `AWxEffectZone`(광역), `UWxCombatLibrary`(즉시 데미지 적용 유틸).
- 락온 타겟팅: `UWxLockOnManagerComponent`·`UWxLockOnPointComponent` + TargetingSystem 필터 태스크(`Targeting/`).
- 애님 노티파이: 콤보 윈도우·무적·퍼펙트가드·무기 어택·투사체 스폰·게임플레이 이벤트 송출 등 (`AnimNotify/`).
- 시간 연출: `UWxTimeDilationComponent`(서버 권위 글로벌 슬로우), GameplayCue를 통한 히트스톱.

**경계 (비담당)**
- Gameplay Tag(Event.HitReact.*, Input.*, State.* 등)의 네이티브 선언과 공용 정의는 [[WxCore]]에 있다. 이 모듈은 소비만 한다.
- UI 표시 데이터(어빌리티 아이콘 등)는 `UWxAbilityComponent` 파생 클래스로 [[WxUI]]에 위임한다.
- AI 의사결정/행동 트리는 [[WxAI]]. 이 모듈은 AI가 트리거하는 패턴 어빌리티(`WxAbility_Pattern*`)만 제공한다.

## 의존성
- **주요 의존**: `WxCore` (유일한 Wx 의존). 엔진 서브시스템: GameplayAbilities(GAS), TargetingSystem, MotionWarping, ModularGameplay, EnhancedInput, Niagara·LevelSequence/MovieScene(연출).
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (`.uplugin`·`Build.cs` 모두 `WxCore`만 참조)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 캐릭터 ASC. 입력 태그→어빌리티 라우팅, AbilitySet 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 공용 쿨다운/코스트·후딜(StartRecovery)·테이블 주도 수치 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 어빌리티·이펙트·어트리뷰트 초기 데이터를 한 에셋으로 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | 전투 스탯 어트리뷰트 세트(복제·데미지 처리) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 데미지 한 건의 설계 데이터→GE Spec 변환의 중심 | `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxCombatLibrary` | 무기/투사체 외 경로의 데미지 적용·적대 판정 BP 라이브러리 | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `AWxWeaponBase` | 무기 액터. ANS_WeaponAttack이 구동하는 스윕 히트 판정 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent 단위) 복제 관리 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase`를 상속(C++ 또는 BP). 쿨다운은 `CooldownTime`/`MaxRecharges`, 코스트는 `MPCost`/`UPCost` 프로퍼티로 데이터 주도 설정. 공용 `UWxEffect_Cooldown`/`UWxEffect_Cost` GE를 재사용하므로 개별 GE를 만들 필요 없음.
- 데이터 주도: 어빌리티 수치는 `WxAbilityTableRow`, 어트리뷰트 초기값은 `WxCombatAttributeInitTableRow`, 데미지는 `WxDamageTableRow` DataTable Row로 주입. 캐릭터 BP에 `UWxAbilitySet`을 지정하면 InitAbilityActorInfo 시 일괄 부여.
- 새 데미지 효과: 타겟에 함께 걸 GE는 `FWxDamageInfo::AdditionalEffects`에 추가. 스탯 반영 계산은 `WxExecCalc_Damage`/`WxMMC_LinearDrain` 등을 참고해 ExecCalc/MMC로 확장.
- 리플리케이션: ASC 입력 태그·락온 대상·글로벌 TimeDilation·래그돌은 모두 서버 권위 복제. 소유 클라 예측 후 정합하는 패턴(`UWxLockOnManagerComponent` 참고).

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 쿨다운/코스트/후딜 규약이 모든 어빌리티의 전제. 헤더 주석에 GAS 순정 API를 왜 오버라이드하는지 설명.
2. `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` + `Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 데미지가 어떻게 Spec이 되고 어트리뷰트를 차감하는지 데이터 흐름의 양끝.
3. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` — 캐릭터가 전투 능력을 획득하는 진입점.

## 관련
- 상위: 플레이어/적 캐릭터([[WxGame]])가 ASC·AbilitySet을 붙여 사용. 태그·공용 정의는 [[WxCore]], 어빌리티 UI 데이터는 [[WxUI]], AI 패턴 구동은 [[WxAI]]와 함께 본다.

---
*문서 기준 커밋 `1a693b0` · 생성일 2026-07-02 · 소스 149파일 — `/readme-writer`로 갱신*
