# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 올린 액션 RPG 전투의 코드 기반. 어빌리티 발동·라우팅, 어트리뷰트/대미지 처리, 락온·타게팅, 무기/투사체 히트 판정, 몽타주 구동 노티파이를 담당한다.

## 책임
**담당**
- 어빌리티 발동·입력 라우팅·입력 버퍼·배타 점유(Activation Group) — `UWxAbilitySystemComponent`
- 전투 어트리뷰트(HP/SP/GP/MP/UP, ATK/DEF, Crit, ASPD 등)와 대미지·회복·사망 처리 — `UWxCombatAttributeSet`
- 대미지 파이프라인: 성립 판정 → GE 적용 → ExecCalc 산출 → 크리 컨텍스트 전달 — `UWxCombatLibrary`, `UWxExecCalc_Damage`
- 락온/타게팅, 무기 히트박스 스윕, 투사체, 히트스톱·슬로모(TimeDilation)
- 몽타주 타임라인에서 콤보 창·후딜·무기 판정·GameplayEvent를 여는 AnimNotify 계열

**경계 (비담당)**
- 어트리뷰트/어빌리티를 소유·초기화하는 캐릭터·폰과 입력 바인딩은 게임 모듈(캐릭터 BP가 `UWxAbilitySet` 지정)
- 공용 정의·팀/식별 등 foundation은 [[WxCore]]에 위임
- HUD·데미지 플로터의 실제 위젯 표현은 [[WxUI]] (이 모듈은 CueNotify로 신호만 낸다)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 어빌리티 라우팅의 유일한 진입점(입력 트리거→발동, 입력 버퍼, 배타 점유, 히트스톱) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 16종 어빌리티의 베이스. ActivationPolicy/Group·InputAction·DataRow 규약을 정의 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 캐릭터가 지정하는 DataAsset. ASC에 어트리뷰트·어빌리티·GE 일괄 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | 전투 스탯 전부와 대미지/사망 후처리 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxCombatLibrary` | 대미지·이펙트 적용의 공용 진입 BFL(`ApplyDamage`, `CheckDamage`) | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `UWxExecCalc_Damage` | 최종 대미지 산출(ATK·DEF·크리). 결과를 EffectContext에 실음 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h` |
| `UWxLockOnComponent` | 서버 권위·클라 예측으로 복제되는 락온 대상(SceneComponent 단위) | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnComponent.h` |
| `AWxWeaponBase` | 무기 액터. ShapeComponent 히트박스 스윕/오버랩으로 히트 판정 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase` 파생 → `ActivationPolicy`(OnTriggered/OnGiven)·`ActivationGroup`(Independent / Exclusive / Override)·`ActivationInputAction` 지정 → `UWxAbilitySet::GrantedAbilities`에 등록. 입력 라우팅 키는 각 CDO의 `ActivationInputAction`이 쥔다.
- **데이터 주도 설정**: `FWxAbilityTableRow`(쿨다운·코스트·아이콘), `FWxDamageTableRow`(ATK 계수·HitReact·가드/패리 가능 여부·추가 GE), `FWxCombatAttributeInitTableRow`(어트리뷰트 초기값). 어빌리티/AbilitySet은 `FDataTableRowHandle`로 참조.
- **대미지 컨텍스트**: `FWxCombatEffectContext`(크리 판정 등 어트리뷰트로 못 싣는 값)를 쓰려면 `UWxAbilitySystemGlobals`를 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`에 등록해야 한다(누락 시 `UWxExecCalc_Damage`가 ensure로 알림).
- **타게팅 필터**: `Targeting/WxTargetingFilterTask_*`(Team·LineTrace·ScreenBounds·InputDirection·GameplayTag) — TargetingSystem 플러그인 태스크를 파생해 추가.
- **리플리케이션/권한**: 서버 권위 모델. 어트리뷰트·어빌리티 부여는 서버에서, 락온은 소유 클라 로컬 예측 후 서버 권위 복제로 정합한다.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력이 어떻게 어빌리티로 라우팅되고 배타 점유·버퍼가 도는지, 전투의 심장
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 모든 어빌리티가 공유하는 Activation Group/Policy 축과 DataRow 규약
3. `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` + `AbilitySystem/Effect/WxEffect_Damage.h` — 히트에서 대미지 성립·산출까지의 파일 횡단 흐름
4. `Plugins/WxCombat/Source/WxCombat/Public/AnimNotify/` — 몽타주 타임라인이 콤보 창·후딜·무기 판정을 여는 지점(전투 타이밍의 실제 구동부)

## 관련
- 상위: 캐릭터·폰이 ASC와 `UWxAbilitySet`을 소유하는 게임 모듈(`WxGame`), foundation은 [[WxCore]], 전투 신호를 표현하는 [[WxUI]]

---
*문서 기준 커밋 `bb06a17` · 생성일 2026-08-30 · 소스 152파일 — `/readme-writer`로 갱신*
