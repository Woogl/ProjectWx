# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 얹은 액션 RPG 전투의 코어. 어빌리티 발동·라우팅, 어트리뷰트(HP/SP/DP/MP/UP), 대미지·가드 판정, 락온·모션워핑, 히트스톱·시간감속을 담당한다.

## 책임
**담당**
- ASC/AttributeSet 확장과 어빌리티 라우팅(입력 → 발동), 배타 점유(ActivationGroup) 판정
- 대미지 파이프라인: `CheckDamage` 선판정 → `ExecCalc` 계산 → `AttributeSet` 확정·연출 발행
- 가드/퍼펙트가드/무적/그로기/처형 등 전투 상태 GE와 GameplayCue 연출
- 락온(SceneComponent 단위, 서버 권위 복제), 몽타주 타겟 스냅(MotionWarping), 무기/투사체 히트 판정
- 히트스톱(개별 몽타주 프리즈)과 글로벌 시간감속(GameState 복제)

**경계 (비담당)**
- 공용 정의·foundation은 [[WxCore]]에 위임
- 캐릭터 클래스·입력 바인딩 소유자·Experience 주입 설정은 이 모듈 밖(게임 모듈/GameFeature)에서 온다

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 입력 라우팅·히트스톱·배타 점유 판정의 중심 ASC | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxCombatAttributeSet` | 어트리뷰트 보유 + `PostGameplayEffectExecute`에서 대미지 확정·연출 발행 | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. `ActivationPolicy`/`ActivationGroup`, 데이터 Row 연결 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 어빌리티·이펙트·어트리뷰트 초기화를 ASC에 일괄 부여하는 데이터에셋 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatLibrary` | `ApplyDamage`/`ApplyEffect`/`RemoveEffect` — 대미지·상태 공용 진입점(BP) | `Source/WxCombat/Public/WxCombatLibrary.h` |
| `UWxExecCalc_Damage` | 무적·가드·크리 판정과 HP/DP/SP 모디파이어 산출 (`CheckDamage` 선판정 포함) | `Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent)을 서버 권위로 복제·브로드캐스트 | `Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |
| `FWxDamageTableRow` / `FWxAbilityTableRow` | 공격 밸런스(계수·반응·추가효과) / 쿨다운·코스트 데이터 Row | `Source/WxCombat/Public/Damage/WxDamageTableRow.h`, `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityTableRow.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase` 파생(대개 `Public/AbilitySystem/Ability/` 아래 `WxAbility_*`). 쿨다운·코스트는 `AbilityDataRow`(`FWxAbilityTableRow`)에서 읽어 공용 GE를 쓰고, 활성 구간 유지 효과는 `ActivationOwnedEffects`(Infinite GE)로 건다. 배타 점유는 `ActivationGroup`(Independent / Exclusive_Blocking·ComboWindow·Recovery / Reaction)로 선언.
- **새 전투 이펙트/큐**: `Public/AbilitySystem/Effect/`(`WxEffect_*`), `Public/AbilitySystem/Cue/`(`WxCueNotify_*`)에 추가. 대미지 부수효과는 코드가 아니라 `FWxDamageTableRow::AdditionalEffects`로 데이터 주도.
- **데이터 주도 설정**: 어빌리티/이펙트/어트리뷰트 초기화는 `UWxAbilitySet` DataAsset, 공격 수치는 `FWxDamageTableRow`, 어트리뷰트 초기값은 `WxCombatAttributeInitTableRow`.
- **EffectContext 등록 필수**: 대미지 판정 결과(크리 등)는 `FWxCombatEffectContext`에 실린다. `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`이 `UWxAbilitySystemGlobals`로 등록돼야 하며, 빠지면 `ExecCalc`가 ensure로 알린다.
- **리플리케이션/권한(최대 4인 멀티)**: 어트리뷰트·락온·시간감속은 서버 권위. 대미지 GE는 Instant+Execution이라 예측 시 execution을 건너뛰고 어트리뷰트는 서버 권위로 남으며, 예측되는 것은 지속형 AdditionalEffects뿐. 락온은 소유 클라 로컬 선반영 후 서버 정합.

## 여기서부터 읽어라
1. `Source/WxCombat/Public/WxCombatLibrary.h` — 대미지·상태 적용의 단일 진입점. 예측/권위 모델 주석이 파이프라인 전체를 요약한다.
2. `Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h` — `CheckDamage`→`ExecCalc`→AttributeSet로 이어지는 판정·확정 흐름의 핵심.
3. `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력이 어떻게 어빌리티로 라우팅되고 배타 점유가 판정되는지.
4. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티 활성화 정책·그룹 규약(모든 `WxAbility_*`의 공통 계약).

## 관련
- 상위: 캐릭터/입력 소유자와 Experience 주입(게임 모듈·`Plugins/GameFeatures/`)이 이 모듈의 컴포넌트·AbilitySet을 켠다
- 함께 보는 모듈: [[WxCore]] (foundation 의존), [[WxAI]] (AI 어빌리티 발동), [[WxUI]] (어트리뷰트·연출 표시)

---
*문서 기준 커밋 `e1999dc` · 생성일 2026-08-24 · 소스 146파일 — `/readme-writer`로 갱신*
