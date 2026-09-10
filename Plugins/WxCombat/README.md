# WxCombat — 전투 시스템

> Gameplay Ability System(GAS) 위에 얹은 액션 RPG 전투의 핵심 모듈. 어빌리티 발동·입력 라우팅·어트리뷰트·피해 판정·락온·소환/투사체까지 전투 한 판이 도는 데 필요한 서버 권위 로직을 담는다.

## 책임
**담당**
- ASC 확장과 입력 라우팅: 라이브 키 입력 → 어빌리티 발동, 실패 입력의 선입력 버퍼링.
- 어빌리티 골격: 발동 그룹(Independent/Exclusive/Override)과 발동 중 캔슬 창(Blocking→ComboWindow→Recovery) 상태 기계.
- 전투 어트리뷰트(HP/SP/GP/MP/UP/ATK/DEF/치명타/속도 등)와 피해 산출·반영.
- 무기 히트박스 스윕 판정 → 피해 GE 적용 → 피격 반응·플로터·퍼펙트 가드 반사.
- 다수의 상태·비용·쿨다운 GameplayEffect, GameplayCue 연출, AnimNotify 구동.
- 락온 대상 보유/복제, TargetingSystem 필터·정렬 태스크, 소환물·투사체 서버 스폰.

**경계 (비담당)**
- 공용 태그·팀·UI 데이터 인터페이스 등 도메인 공통 정의는 [[WxCore]]에 위임(`IWxUIData`, 팀 조회 등).
- 어빌리티를 실제로 언제 트리거할지(플레이어 입력 바인딩·AI 패턴 결정)는 상위 소비자([[WxGame]] 캐릭터, AI) 몫. 이 모듈은 트리거를 받아 처리한다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 입력 라우팅·AbilitySet 부여·몽타주 재생의 ASC 확장 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 발동 그룹/캔슬 창 상태 기계 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 어빌리티·GE·어트리뷰트 초기값을 ASC에 일괄 부여하는 DataAsset | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | 전투 어트리뷰트 정의·클램프·피해 메타 반영 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxCombatLibrary` | 피해 성립 판정과 피해/상태 GE 적용 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `AWxWeaponBase` | 무기 히트박스 Overlap+Sweep 판정, 스윙당 1회 피격 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxInputBufferComponent` | 실패 입력을 기억했다 캔슬 창에서 재시도(선입력) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxInputBufferComponent.h` |
| `UWxLockOnComponent` | 겨누는 대상(SceneComponent) 보유·복제 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnComponent.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase` 파생(`Private/AbilitySystem/Ability/`의 `WxAbility_*`가 예시). 쿨다운·코스트 수치는 `AbilityDataRow`(`WxAbilityTableRow`)에서 읽고, 쿨다운은 어빌리티가 지정한 `UWxEffect_Cooldown` 파생 GE, 코스트는 공용 `UWxEffect_Cost`가 그 수치를 쓴다. 발동 방식은 `ActivationPolicy`(OnTriggered/OnGiven)로 정한다.
- **새 GameplayEffect**: `Public/AbilitySystem/Effect/`의 `WxEffect_*`(무적·초월·가드경감·역경직 등)를 따른다. GE에 붙이는 로직은 `UWxEffectComponent_*`(`_Table`, `_DamageResponse`)로 확장한다.
- **데이터 주도**: 어빌리티는 `WxAbilityTableRow`, 피해는 `WxDamageTableRow`(계수·HitReact 태그·크리·가드/패리 가부·추가 GE), 어트리뷰트 초기값은 `WxCombatAttributeInitTableRow`를 DataTable로 저작한다. 캐릭터 BP는 `UWxAbilitySet` 에셋만 ASC에 꽂으면 된다.
- **AnimNotify로 구동**: 공격 판정·피해·스폰·이벤트·콤보 창 전이는 `Public/AnimNotify/`의 노티파이가 몽타주 타임라인에서 낸다(`WxAnimNotifyState_WeaponAttack`, `WxAnimNotify_SpawnProjectile` 등).
- **리플리케이션/권한**: GAS 기반 서버 권위. 어트리뷰트는 `ReplicatedUsing`으로 복제, 피해·소환물·투사체 스폰은 서버에서 실행한다. 락온만 클라이언트를 신뢰하는 선택 정책(소유 클라 로컬 반영 후 `Server` RPC로 권위 설정, 서버 재검증 없음).

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 발동 그룹·캔슬 창 상태 기계가 전투 흐름의 뼈대다. 여기 주석이 배타/캔슬 규칙을 설명한다.
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` + `WxInputBufferComponent.h` — 입력이 어떻게 어빌리티로 가고, 실패 입력이 어떻게 선입력으로 재시도되는지.
3. `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` → `AbilitySystem/Effect/WxEffect_Damage.h` → `Effect/WxEffectComponent_DamageResponse.h` — 피해가 성립 판정부터 어트리뷰트 반영, 피격 반응까지 흐르는 경로.
4. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 전투 수치 전부와 클램프/최대치 조정 규칙.

## 관련
- 상위: [[WxGame]] 게임 모듈의 캐릭터가 ASC·`UWxAbilitySet`을 사용하고, AI가 패턴 어빌리티·락온을 구동한다. `WxCore` 외 다른 Wx 플러그인은 참조하지 않는다.

---
*문서 기준 커밋 `ffc6360` · 생성일 2026-09-10 · 소스 176파일 — `/readme-writer`로 갱신*
