# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 구축한 액션 RPG 전투의 코드 기반. 어빌리티·이펙트·어트리뷰트로 대미지/자원/상태를 처리하고, 무기·투사체 히트 판정, 락온 타겟팅, 히트스톱/슬로우 시간 연출까지 전투 런타임 전반을 담당한다.

## 책임
**담당**
- 캐릭터 스탯 어트리뷰트(HP/SP/DP/MP/UP, ATK/DEF/치명타/속도)와 대미지 계산 파이프라인(`WxExecCalc_Damage`, 메타 어트리뷰트 `IncomingDamage`).
- 어빌리티 계층: 공격·회피·가드·스킬·궁극기·처형·그로기·피격반응·락온·질주 및 AI 패턴. 쿨다운/코스트를 DataTable Row 기반으로 통일.
- 입력→어빌리티 라우팅(`UWxAbilitySystemComponent`)과 AbilitySet 일괄 부여.
- GameplayEffect/Cue/MMC/ExecCalc, 전투용 AnimNotify(무기 공격 구간, 콤보 윈도우, 무적, 투사체 스폰 등).
- 무기/투사체 히트 콜리전, 락온 타겟팅(TargetingSystem 필터 태스크), MotionWarping 스냅, 히트스톱·슬로우모션 시간 조작.

**경계 (비담당)**
- 캐릭터 폰/플레이어 컨트롤러·입력 매핑 자체의 정의 — 소비자 모듈이 이 모듈의 ASC/컴포넌트를 붙여 쓴다.
- 공용 정의(팀/태그 등 foundation)는 [[WxCore]]에 위임.
- AI 의사결정은 [[WxAI]] (이 모듈은 어빌리티 실행부만 제공).
- HUD·자원 게이지 위젯 등 표시 계층은 [[WxUI]].

## 의존성
- **주요 의존**: WxCore. 엔진 서브시스템 — GameplayAbilities, GameplayTags, GameplayTasks, ModularGameplay, EnhancedInput, TargetingSystem, MotionWarping, AIModule, NavigationSystem, UMG, Niagara(Private), LevelSequence/MovieScene(Private).
- 규칙: WxCore 외 다른 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 프로젝트 ASC. 입력 액션을 어빌리티로 라우팅, AbilitySet 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 쿨다운/코스트/충전을 DataTable Row로 통일 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 어빌리티·이펙트·어트리뷰트 초기값을 한 에셋으로 묶어 ASC에 일괄 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | 전투 스탯 전체와 대미지 적용(`PostGameplayEffectExecute`)의 소재지 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터 → GE Spec 변환 지점 | `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxCombatLibrary` | 무기/투사체 밖 경로의 단일 대미지 적용 진입점(`ApplyDamage`) | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `AWxWeaponBase` | 무기 히트 콜리전. AnimNotify가 공격 구간을 여닫음 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent 단위) 저장·복제 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- 새 어빌리티는 `UWxAbilityBase`를 상속. 발동 입력은 CDO의 `ActivationInputAction`(복수 입력은 `IsActivationInput`/`GetInputActions` override), 활성화 정책은 `EWxAbilityActivationPolicy`(OnTriggered/OnGranted).
- 데이터 주도 설정: 쿨다운/코스트/충전/아이콘은 `AbilityDataRow`(`FWxAbilityTableRow`), 대미지 수치는 `FWxDamageTableRow`, 어트리뷰트 초기값은 `FWxCombatAttributeInitTableRow`. 어빌리티·이펙트 부여는 `UWxAbilitySet` 에셋으로.
- 쿨다운은 공용 `UWxEffect_Cooldown` GE + MMC로 처리(어빌리티별 GE 오버라이드 불필요). 코스트도 공용 `UWxEffect_Cost` + MMC. 마커 GE를 다른 GE로 바꾸면 그 어빌리티만 엔진 순정 경로.
- 대미지는 `FWxDamageInfo`→`MakeSpecs`→`UWxEffect_Damage`(`WxExecCalc_Damage`)로 흐르며, 무기/투사체 외 경로는 `UWxCombatLibrary::ApplyDamage`가 유일 진입점.
- 리플리케이션: 어트리뷰트/락온 대상은 서버 권위 복제, 대미지 적용·투사체 스폰은 authority 게이트. 소유 클라는 응답성을 위해 국소 예측 후 복제값과 정합.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력이 어떻게 어빌리티로 라우팅되는지, 전투 진입의 최상단.
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티 공통 규약(쿨다운/코스트/히트스톱/후딜 캔슬)의 근간.
3. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 스탯 정의와 대미지가 HP로 반영되는 지점.
4. `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` — 히트 한 건이 GE Spec으로 바뀌는 데이터 흐름.

## 관련
- 상위: 캐릭터/폰이 `UWxAbilitySystemComponent`·`UWxLockOnManagerComponent`를 부착해 사용. AI 실행부는 [[WxAI]], 표시는 [[WxUI]], 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `a5b5f20` · 생성일 2026-07-29 · 소스 147파일 — `/readme-writer`로 갱신*
