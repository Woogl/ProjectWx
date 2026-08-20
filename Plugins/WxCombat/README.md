# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반 액션 RPG 전투를 담당한다. 어빌리티·어트리뷰트·대미지 판정·락온·무기/투사체·히트스톱까지 캐릭터가 싸우는 데 필요한 런타임 전반을 제공한다.

## 책임
**담당**
- GAS 파운데이션: 커스텀 `UAbilitySystemComponent`, `UAttributeSet`, `AbilitySet`(어빌리티·이펙트·어트리뷰트 일괄 부여), 커스텀 `AbilitySystemGlobals`
- 전투 어빌리티군: 공격·회피·가드·스킬·궁극기·피니셔·히트리액트·그로기·락온·스프린트, AI 패턴 어빌리티
- 대미지 파이프라인: `FWxDamageInfo` 설계 데이터 → GE Spec 생성 → `UWxExecCalc_Damage` 판정(무적·가드·퍼펙트가드·크리) → 메타 어트리뷰트와 `FWxCombatEffectContext`로 결과 전달 → `UWxCombatAttributeSet::PostGameplayEffectExecute`가 소비(Cue·반응 이벤트·후속 GE)
- GameplayEffect·GameplayCue·AnimNotify(무기 판정·투사체 스폰·콤보 윈도우·카메라·무적/퍼펙트가드 구간) 세트
- 타게팅: 락온 관리/필터 태스크, 몽타주 스냅용 RootMotionModifier
- 무기·투사체 액터, 히트스톱, 서버 권위 글로벌 TimeDilation

**경계 (비담당)**
- 공용 Gameplay Tag 네이티브 선언과 공용 정의는 [[WxCore]]에 있다(`WxGameplayTags`)
- 캐릭터/폰 클래스, 입력 매핑 소스, Experience/GameFeature 주입 설정은 이 모듈 밖(게임 모듈·GameFeature)
- AI 의사결정(BT/유틸리티)은 [[WxAI]]; 이 모듈은 AI가 트리거하는 패턴 어빌리티만 제공
- 데미지 수치의 HUD 표기 등 UI 위젯 자체는 [[WxUI]]

## 의존성
- **주요 의존**: `WxCore`, 그리고 엔진 서브시스템 `GameplayAbilities` / `GameplayTags` / `GameplayTasks`, `TargetingSystem`, `MotionWarping`, `ModularGameplay`, `EnhancedInput`, `AIModule` / `NavigationSystem`, `UMG`, `Niagara`(연출)
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (`.uplugin`·`Build.cs`·소스 include 모두 `WxCore`만 참조)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxCombatLibrary` | 모든 대미지 경로가 공유하는 단일 진입점 `ApplyDamage` | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `UWxAbilitySystemComponent` | 입력→어빌리티 라우팅, 히트스톱, GE 적용 후 Cue·반응 이벤트 발행 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 전투 어빌리티의 베이스 — Row 기반 쿨다운/코스트, 몽타주, 후딜(캔슬) 규약 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 어빌리티·이펙트·어트리뷰트 초기화를 ASC에 일괄 부여하는 DataAsset | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | HP/SP/DP/MP/UP·ATK/DEF·Crit·SPD/ASPD와 IncomingDamage·IncomingReflect 메타. 대미지 판정 결과를 소비하는 지점이기도 하다 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxEffect_Damage` / `UWxExecCalc_Damage` | 대미지 GE와 판정 계산(무적·가드·퍼펙트가드·크리·반사) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터, Row에서 생성돼 GE Spec으로 변환 | `Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageInfo.h` |
| `FWxCombatEffectContext` | 어트리뷰트로도 스펙 태그로도 복원할 수 없는 판정(크리 여부 등)만 소비 지점까지 나르는 컨텍스트 | `Plugins/WxCombat/Source/WxCombat/Public/Damage/WxCombatEffectContext.h` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent 단위)을 서버 권위로 복제·브로드캐스트 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase` 상속. 쿨다운·코스트 수치는 `AbilityDataRow`(WxAbilityTableRow)에서 읽으며, 기본 쿨다운/코스트 GE 클래스는 공용 GE를 가리키는 "마커"다 — 마커면 Row 기반, 다른 GE로 바꾸면 엔진 순정 경로(Row와 상호배타). 입력 발동 어빌리티는 `ActivationInputAction`을 지정하면 AbilitySet이 모아 바인딩한다.
- 새 이펙트/큐: `Public/AbilitySystem/Effect`·`Cue` 아래 명명 규약(`WxEffect_*`, `WxCueNotify_*`)을 따른다.
- 데이터 주도: `UWxAbilitySet`(GrantedAbilities/GrantedEffects/AttributeInitRow)로 캐릭터에 일괄 부여. 대미지는 DataTable Row(`WxDamageTableRow`)→`FWxDamageInfo`, 어트리뷰트 초기값은 `WxCombatAttributeInitTableRow`.
- 대미지 판정 결과 전달은 `FWxCombatEffectContext`가 유일 통로다. `UWxAbilitySystemGlobals`를 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`에 등록해야 모든 GE가 이 컨텍스트를 갖는다(누락 시 ExecCalc가 ensure).
- 리플리케이션/권한: 대미지 GE는 Instant+Execution이라 클라 예측 시 execution을 건너뛰고 어트리뷰트와 그 결과 소비는 서버 권위다. 클라 예측은 `UWxExecCalc_Damage::CheckDamage` 선판정으로 서버와 결론을 맞춘다. 락온·TimeDilation은 서버 권위 복제.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` — 대미지가 흐르는 단일 입구. 예측/권위 모델을 주석에서 먼저 잡는다.
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h` — 실제 판정 규칙(상태별 분기)과 `CheckDamage` 선판정.
3. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 판정 결과를 실제 상태 변화와 연출로 옮기는 소비 지점. 출력 모디파이어 순서가 계약인 이유가 여기 있다.
4. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 개별 어빌리티를 읽기 전 공통 쿨다운/코스트/후딜 규약.

## 관련
- 상위: 캐릭터·Experience·GameFeature(게임 모듈, `Plugins/GameFeatures/`)가 AbilitySet·TimeDilation 컴포넌트를 주입해 사용
- 함께: [[WxCore]](공용 정의·`WxGameplayTags`), [[WxAI]](패턴 어빌리티 트리거), [[WxUI]](어트리뷰트·판정 연출 표기)

---
*문서 기준 커밋 `e355c65` · 생성일 2026-08-19 · 소스 153파일 — `/readme-writer`로 갱신*
