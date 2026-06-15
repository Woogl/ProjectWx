# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반의 액션 RPG 전투를 책임진다. 어빌리티·이펙트·어트리뷰트·데미지 파이프라인, 무기/투사체 히트 판정, 락온, 시간 감속(히트스톱/슬로우)을 한곳에 묶는다.

## 책임
**담당**
- ASC/AttributeSet/AbilitySet 등 GAS 런타임 골격과 어빌리티 입력 라우팅(InputTag → Activate)
- 전투 어빌리티 일습: Attack/Skill/Ultimate/Dodge/Guard/Sprint/HitReact/Groggy/Death, AI용 Pattern/Phase, LockOn
- 데미지 파이프라인: `FWxDamageInfo` 설계 데이터 → Damage GE Spec → `WxExecCalc_Damage`(가드/퍼펙트가드/그로기/치명타/반사 처리)
- 상태이상·자원 GE(Burn/BuffATK/Cost/Cooldown/Exceed 등), GameplayCue, AnimNotify(무기 공격 구간/투사체 생성/콤보 윈도우/무적 등)
- 무기·투사체 액터의 히트 판정, 락온 타겟 관리, 글로벌 TimeDilation 서버 동기화

**경계 (비담당)**
- Gameplay Tag 정의·캐릭터/팀 정의 → [[WxCore]] (이 모듈은 `WxGameplayTags`를 소비만 함)
- HUD·체력바·어빌리티 아이콘 등 표시 → [[WxUI]] (어빌리티 표시 데이터는 UWxAbilityComponent 파생으로 외부 부착)
- AI 의사결정(BT/BB) → [[WxAI]] (이 모듈은 패턴 어빌리티 실행만)

## 의존성
- **주요 의존**: `WxCore` / 엔진: `GameplayAbilities`, `GameplayTags`, `GameplayTasks`, `EnhancedInput`, `TargetingSystem`, `MotionWarping`, `AIModule`, `NetCore` (private: `LevelSequence`, `Niagara`, `MovieScene`)
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 프로젝트 ASC. InputTag 입력 라우팅·래그돌 복제 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 공용 쿨다운/코스트 GE·테이블 Row·컴포넌트 부착 규약 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | ASC에 부여할 Ability/Effect/Attribute 초기 데이터 묶음 (DataAsset) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | 캐릭터 스탯 전부(HP/SP/DP/MP/UP/ATK/DEF/Crit/SPD/ASPD + IncomingDamage meta) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 데미지 한 건의 설계 데이터 → GE Spec 변환 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxExecCalc_Damage` | 데미지 최종 계산(가드/퍼펙트가드/그로기/치명타/반사) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |
| `UWxCombatLibrary` | 무기/투사체 외 경로의 데미지 적용·적대 판정 BP 라이브러리 | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `AWxWeaponBase` / `AWxProjectileBase` | 무기 스윙·투사체 히트 판정 액터 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/` |

## Gameplay Tags
- 이 모듈은 C++ Native Tag를 직접 **선언하지 않는다**. 모든 태그는 [[WxCore]]의 `WxGameplayTags`를 include해 소비한다.
- 소비하는 주요 네임스페이스: `State.*`(Guard/PerfectGuard/Groggy/Invincible 등 전투 상태), `Event.*`(HitReact/PerfectGuard/DodgeSuccess 등 어빌리티 트리거), `Input.*`(어빌리티 입력 라우팅 키), `SetByCaller.*`(Coeff.ATK·Recovery.MP/UP·RawDamage·ReflectDP 등 데미지 수치 전달), `Damage.*`(Unblockable/ParryHitReact).

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase` 상속. 쿨다운/코스트는 `CooldownTime`/`MaxRecharges`/`MPCost`/`UPCost` 프로퍼티로 설정(공용 `UWxEffect_Cooldown`/`UWxEffect_Cost` GE 사용). 입력 발동은 `OnInputTriggered`, 패시브는 `OnGranted`.
- 데이터 주도: 어빌리티 수치는 `AbilityDataRow`(FWxAbilityTableRow), 데미지는 `FWxDamageTableRow`, 어트리뷰트 초기값은 `FWxCombatAttributeInitTableRow`(AbilitySet의 `AttributeInitRow`)로 테이블 구동. 캐릭터에 부여할 전투 일체는 `UWxAbilitySet` 에셋 하나로 묶는다.
- 어빌리티 표시 메타데이터는 `UWxAbilityComponent` 파생을 BP에서 Instanced로 부착해 도메인 간 결합 없이 확장(예: UI 아이콘은 WxUI측 파생).
- 리플리케이션/권한(최대 4인): 어트리뷰트·락온 대상·래그돌·글로벌 TimeDilation은 서버 권위 복제. 소유 클라는 락온 등에서 로컬 예측 후 서버 권위로 정합한다.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 모든 어빌리티의 공통 규약(쿨다운/코스트/테이블/후딜). 어빌리티 작업의 출발점.
2. `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp` — 데미지가 가드/그로기/치명타/반사를 거쳐 HP에 도달하는 전체 흐름.
3. `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` + `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` — AnimNotify→무기→GE Spec로 이어지는 히트 적용 경로.

## 관련
- 상위: 캐릭터/게임모드가 `UWxAbilitySet`으로 이 모듈을 장착. 태그·팀·공용 정의는 [[WxCore]], 표시는 [[WxUI]], AI 구동은 [[WxAI]]와 함께 본다.

---
*문서 기준 커밋 `6402bb0` · 생성일 2026-06-15 · 소스 141파일 — `/readme-writer`로 갱신*
