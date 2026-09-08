# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 얹은 액션 RPG 전투의 코어. 어빌리티·이펙트·어트리뷰트로 공격/방어/피격을 굴리고, 락온·선입력·히트스톱·소환·투사체 같은 액션 감각을 붙인다.

## 책임
**담당**
- 어빌리티 발동 파이프라인: 배타 점유(EWxAbilityActivationGroup)와 발동 중 캔슬 창(EWxAbilityActionPhase), 몽타주 구동, 선입력 버퍼링
- 전투 어트리뷰트(HP/SP/GP/MP/UP/ATK/DEF/치명타/속도)와 데미지 산출·피격 반응 흐름
- 데이터 주도 GameplayEffect 세트(코스트/쿨다운/데미지/상태 버프)와 DataTable 기반 수치 조회
- 락온 타겟팅, AnimNotify 기반 히트박스·이벤트, 히트스톱, 소환물·투사체 스폰

**경계 (비담당)**
- 공용 정의(팀·UI 데이터 인터페이스 `IWxUIData` 등)는 [[WxCore]]에 둔다
- HUD·어빌리티 아이콘 등 실제 표시는 [[WxUI]]가 담당하고, 이 모듈은 `IWxUIData`로 데이터만 제공
- AI의 조준·발동 결정은 [[WxAI]] (AIController가 락온 타겟을 채움)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 입력 라우팅·몽타주·AbilitySet 부여의 허브 ASC | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 Wx 어빌리티의 베이스. 발동 그룹·캔슬 창·쿨다운/코스트 규약 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 캐릭터에 어빌리티·이펙트·어트리뷰트 초기값을 일괄 부여하는 DataAsset | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | 전투 어트리뷰트 전체와 데미지→HP 반영 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxCombatLibrary` | 데미지/이펙트 적용·적대 판정의 공용 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `UWxInputBufferComponent` | 실패한 입력을 기억했다 캔슬 창에서 재시도(선입력) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxInputBufferComponent.h` |
| `UWxLockOnComponent` | 겨누는 대상을 복제 보관, 소비처 공용 조회 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnComponent.h` |
| `UWxMinionSubsystem` | 소환물 서버 권위 스폰·주인별 로스터·명령 중계 | `Plugins/WxCombat/Source/WxCombat/Public/Minion/WxMinionSubsystem.h` |

## 확장 포인트 / 규약
- 새 어빌리티는 `UWxAbilityBase`(또는 `WxAbility_*` 계열)를 상속하고, 쿨다운/코스트 수치는 `AbilityDataRow`(`FWxAbilityTableRow`)에서 읽는다. 코스트는 공용 `UWxEffect_Cost`, 쿨다운은 어빌리티가 지정한 `UWxEffect_Cooldown` 파생 GE가 그 수치를 소비한다.
- 데미지는 DataTable 주도다: `FWxDamageTableRow`가 계수·크리티컬·가드/패리 가능 여부·추가 GE를 정의하고, `UWxCombatLibrary::ApplyDamage`가 `UWxEffect_Damage`/`UWxExecCalc_Damage`를 통해 SP·GP·IncomingDamage 순으로 반영한다.
- 상태/버프 GE는 `UWxEffectComponent_Table` + `FWxEffectTableRow`로 크기·지속시간을 데이터화하며, 표시는 `IWxUIData` 조회 앵커를 거친다.
- 히트박스·이벤트·연출은 `WxAnimNotify(State)_*`가 몽타주에서 구동한다(콤보 창 개폐, 무기 어택, 이펙트 적용, 소환/투사체 스폰 등).
- 캐릭터는 `UWxAbilitySystemComponent`에 `UWxAbilitySet` 배열을 넣어 초기화한다(서버에서 일괄 부여).
- 리플리케이션/권한: 서버 권위 모델(최대 4인 멀티). 락온 대상·소환물은 서버가 권위를 쥐되 소유 클라 예측을 허용한다.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 발동 그룹/캔슬 창/쿨다운 규약이 전투 어빌리티 전체의 골격이다
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력·몽타주·부여가 모이는 허브
3. `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` — 데미지/이펙트 적용의 실제 진입점과 데이터 흐름

## 관련
- 상위: 캐릭터/폰이 `UWxAbilitySet`으로 이 모듈을 소비. [[WxCore]](공용 정의), [[WxUI]](표시), [[WxAI]](AI 조준·발동)와 함께 본다

---
*문서 기준 커밋 `ba86cff` · 생성일 2026-09-08 · 소스 174파일 — `/readme-writer`로 갱신*
