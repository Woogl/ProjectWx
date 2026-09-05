# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 액션 RPG 전투를 구현한다. 어빌리티/어트리뷰트/이펙트, 입력·선입력 라우팅, 무기·투사체 히트 판정, 락온 타겟팅, 소환물, 처형(Finisher)까지 전투의 런타임 전반을 담당한다.

## 책임
**담당**
- ASC·어트리뷰트·GameplayEffect 정의와 어빌리티 부여(AbilitySet)
- 입력 → 어빌리티 라우팅, 배타 점유(ActivationGroup)와 콤보/후딜 캔슬 창(ActionPhase), 선입력 버퍼
- 대미지 산출·적용 파이프라인(DamageTableRow → GE), 히트스톱, 크리티컬/가드/퍼펙트가드 판정
- 무기 히트박스 스윕, 투사체 스폰, 소환물 스폰·명령
- 락온 타겟팅(TargetingSystem 필터/소터, MotionWarping 스냅), AnimNotify 기반 전투 이벤트

**경계 (비담당)**
- 캐릭터 폰·컨트롤러·팀/AI 퍼셉션 자체: 소비만 하며 소유는 [[WxAI]]·게임 모듈
- UI 표시(대미지 플로터 데이터 계약은 IWxUIData로 노출): [[WxUI]]
- 공용 정의·태그·인터페이스: [[WxCore]]

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 프로젝트 ASC. 입력 라우팅·몽타주 재생·배타 캔슬의 라이브 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilitySet` | 캐릭터 BP가 지정하는 부여 묶음(어트리뷰트 초기화·어빌리티·이펙트) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. ActivationPolicy/Group·ActionPhase 규약을 정의 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | HP/SP/GP/MP/UP·ATK/DEF·Crit 등 전투 어트리뷰트 전체 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxCombatLibrary` | 대미지 성립 판정·적용·상태 GE 부여의 공용 함수 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `UWxInputBufferComponent` | 발동 실패 입력을 기억해 캔슬 창에서 재시도(선입력) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxInputBufferComponent.h` |
| `UWxLockOnComponent` | 캐릭터가 겨누는 대상(SceneComponent 단위)을 복제해 보관 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnComponent.h` |
| `AWxWeaponBase` | 무기 히트박스 스윕/오버랩으로 스윙당 1회 히트 판정 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase` 파생. `EWxAbilityActivationPolicy`(OnTriggered/OnGiven)·`EWxAbilityActivationGroup`(Independent/Exclusive/Override)를 선언하고, Exclusive는 몽타주 노티파이가 여닫는 `EWxAbilityActionPhase`(Blocking→ComboWindow→Recovery) 창을 밟는다. 종류별 파생이 `Public/AbilitySystem/Ability/`에 다수 존재(Attack/Skill/Guard/Dodge/HitReact/Finisher 등).
- 새 GameplayEffect: `Public/AbilitySystem/Effect/`의 `UWxEffect_*` 패턴. 데이터 주도 GE는 `UWxEffectComponent_Table`로 `FWxEffectTableRow`를 지목하고 MMC가 계산 시점에 읽는다.
- 데이터 주도 설정: 대미지는 `FWxDamageTableRow`(CoeffATK·HitReactTag·가드/크리 플래그), 어트리뷰트 초기값은 `FWxCombatAttributeInitTableRow`, 어빌리티 메타는 `FWxAbilityTableRow`가 DataTable Row로 저작.
- 타겟팅: TargetingSystem 플러그인의 `WxTargetingFilterTask_*`/`WxTargetingSorterTask_*` 파생으로 후보 필터·정렬을 추가. 몽타주 중 대상 스냅은 `WxRootMotionModifier_SnapToTarget`(MotionWarping).
- 리플리케이션: 대미지·소환·투사체는 서버 권위. 락온 대상 선택은 클라 신뢰 후 서버 반영. 대미지 성립 판정(`UWxCombatLibrary::CheckDamage`)은 어트리뷰트를 보지 않아 클라 예측·투사체 임팩트가 같은 결론을 공유한다.
- `UWxAbilitySystemGlobals`를 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`으로 등록해야 `FWxCombatEffectContext`가 만들어진다 — 누락 시 대미지 결과가 실리지 못한다.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 배타 점유·캔슬 창 규약이 전투 어빌리티 전체의 뼈대다. 먼저 읽어야 나머지가 읽힌다.
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력이 어떻게 어빌리티로 흘러가고 선입력 버퍼와 갈라지는지의 진입점.
3. `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` — 히트 → 성립 판정 → 대미지 GE 적용의 전투 데이터 흐름 요약.
4. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 전투 자원 모델(HP/SP/GP/MP/UP)과 사망·그로기 트리거 지점.

## 관련
- 상위: 캐릭터/AI가 `UWxAbilitySet`로 이 시스템을 켠다 — [[WxAI]], 게임 모듈(WxGame)
- 함께: 공용 정의·태그는 [[WxCore]], 대미지/이펙트 표시 계약은 [[WxUI]]

---
*문서 기준 커밋 `f826b21` · 생성일 2026-09-05 · 소스 171파일 — `/readme-writer`로 갱신*
