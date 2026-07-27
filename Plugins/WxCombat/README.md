# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반의 액션 RPG 전투를 책임진다. 어빌리티·어트리뷰트·대미지 파이프라인, 무기/투사체 히트 판정, 락온과 히트스톱까지 근접 전투의 핵심을 담는다.

## 책임
**담당**
- GAS 통합: 커스텀 ASC(`UWxAbilitySystemComponent`)의 입력→어빌리티 라우팅, `UWxAbilitySet` 일괄 부여, `UWxCombatAttributeSet` 스탯 관리
- 어빌리티 라이브러리: 공격/회피/가드/스킬/궁극기/피격반응/그로기/사망/락온 등 `UWxAbilityBase` 파생 어빌리티군
- 대미지 파이프라인: `FWxDamageInfo`→GE Spec 변환, `WxExecCalc_Damage`/`WxExecCalc_Burn` 실행 계산, 상태이상·회복 GameplayEffect 군
- 전투 연출 훅: AnimNotify(무기 공격/투사체 스폰/광역/카메라)와 GameplayCue, 히트스톱/슬로모(`UWxTimeDilationComponent`)
- 타겟팅: TargetingSystem 필터 태스크군과 `UWxLockOnManagerComponent` 락온 대상 관리, MotionWarping 스냅

**경계 (비담당)**
- 공용 태그/정의·팀 소속 등 foundation은 [[WxCore]]에 위임 (Gameplay Tag는 WxCombat에서 선언하지 않고 소비만)
- HUD·체력바 등 전투 UI 표현은 [[WxUI]], AI 의사결정은 [[WxAI]]

## 의존성
- **주요 의존**: [[WxCore]], GameplayAbilities, ModularGameplay, EnhancedInput, TargetingSystem, MotionWarping, GameplayTasks, (Private) Niagara·LevelSequence·MovieScene
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 입력 라우팅·AbilitySet 부여의 ASC 진입점 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilitySet` | 어빌리티/이펙트/어트리뷰트 초기값을 한 에셋으로 일괄 부여 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스, 쿨다운·코스트를 DataRow로 해석 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | HP/SP/DP/MP/UP·ATK/DEF 등 스탯, IncomingDamage→HP 차감 | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터, GE Spec으로 변환 | `Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxCombatLibrary` | 무기/투사체 외 경로의 단일 대미지 적용(`ApplyDamage`) | `Source/WxCombat/Public/WxCombatLibrary.h` |
| `AWxWeaponBase` | 무기 스폰·부착·스윙 히트 판정(Overlap) | `Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxLockOnManagerComponent` | 서버 권위 락온 대상(SceneComponent) 관리 | `Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase`(Abstract, Blueprintable)를 상속해 만들고 `UWxAbilitySet.GrantedAbilities`에 등록. 입력 발동은 어빌리티 CDO의 `ActivationInputAction`이 라우팅 키.
- 데이터 주도 밸런싱: 쿨다운·충전·코스트·아이콘은 `FWxAbilityTableRow`, 대미지 계수·회복·HitReact는 `FWxDamageTableRow`. 어트리뷰트 초기값은 `FWxCombatAttributeInitTableRow`.
- 공용 GE 마커: `UWxEffect_Cooldown`/`UWxEffect_Cost`를 그대로 두면 AbilityDataRow 기반 경로, 다른 GE로 바꾸면 엔진 순정 경로(상호배타). 대미지는 `UWxEffect_Damage`+`WxExecCalc_Damage`가 `IncomingDamage`로 전달 후 `PostGameplayEffectExecute`에서 HP 차감.
- 리플리케이션: 어트리뷰트는 복제, 락온 대상과 Global TimeDilation은 서버 권위(소유 클라 예측 후 정합).

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력이 어떻게 어빌리티로 라우팅되고 AbilitySet이 언제 부여되는지, 전투 진입 흐름의 출발점
2. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티가 쿨다운/코스트/입력을 DataRow와 공용 GE로 처리하는 계약
3. `Source/WxCombat/Public/WxDamageInfo.h` + `Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp` — AnimNotify→무기→GE Spec→최종 대미지로 이어지는 대미지 파이프라인
4. `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 스탯 정의와 IncomingDamage/HP·SP·DP 처리 규약

## 관련
- 상위: 플레이어/적 캐릭터가 ASC와 AbilitySet을 이 모듈로 구성하며, [[WxUI]]가 어트리뷰트를 구독하고 [[WxAI]]가 어빌리티를 트리거한다. foundation은 [[WxCore]].

---
*문서 기준 커밋 `21e2e76` · 생성일 2026-07-27 · 소스 147파일 — `/readme-writer`로 갱신*
