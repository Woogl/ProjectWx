# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 올린 액션 RPG 전투의 런타임. 어빌리티 발동·입력 라우팅, 어트리뷰트와 데미지 파이프라인, 무기/투사체 히트 판정, 락온, 애님 노티파이 구동을 담당한다.

## 책임
**담당**
- ASC 커스터마이즈: 입력 라우팅, 입력 버퍼, 히트스톱, 발동 그룹/캔슬 창 점유 (`UWxAbilitySystemComponent`, `UWxAbilityBase`)
- 전투 어트리뷰트(HP/SP/GP/MP/UP/ATK/DEF/Crit/SPD/ASPD)와 데미지·퍼펙트가드 실행 (`UWxCombatAttributeSet`, `UWxExecCalc_Damage`)
- 데이터 주도 GE·쿨다운·코스트 (`UWxEffectComponent_Table`, `FWxAbilityTableRow`, `FWxDamageTableRow`)
- 무기 히트박스 스윕/투사체, 피니셔 처형 피해 (`AWxWeaponBase`, `WxProjectile*`, `UWxFinisherDamageComponent`)
- 락온/타겟팅 필터, 몽타주 타겟 스냅(MotionWarping), 시간 감속 (`UWxLockOnComponent`, `WxTargetingFilterTask_*`, `WxRootMotionModifier_SnapToTarget`)
- 전투 애님 노티파이(콤보 창·무기 공격·이펙트 적용·투사체 스폰·카메라) (`WxAnimNotify*`)

**경계 (비담당)**
- 공용 정의·팀/태그 기반 유틸 등 foundation은 [[WxCore]]에 위임
- 캐릭터·컨트롤러·입력 바인딩 등 게임 조립은 소비 측([[WxGame]] 및 GameFeature)에 위임 — 이 모듈은 ASC/어빌리티/컴포넌트만 제공

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 어빌리티 라우팅 유일 진입점 — 입력 버퍼·히트스톱·발동 그룹 점유 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 Wx 어빌리티 베이스 — 발동 그룹/캔슬 창, 데이터행 기반 쿨다운·코스트 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 캐릭터에 어빌리티·GE·어트리뷰트 초기값을 일괄 부여하는 DataAsset | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | 전투 스탯 전체 + IncomingDamage/Reflect 메타 통로 | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxExecCalc_Damage` | ATK·크리·가드를 종합해 IncomingDamage로 출력하는 데미지 계산 | `Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h` |
| `AWxWeaponBase` | 무기 히트박스 Overlap+틱 Sweep, 스윙당 1회 피격 | `Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxLockOnComponent` | SceneComponent 단위 락온 대상, 서버 권위 복제 | `Source/WxCombat/Public/Targeting/WxLockOnComponent.h` |
| `UWxCombatLibrary` | 데미지 성립 판정·GE 적용 진입 유틸(BP 노출) | `Source/WxCombat/Public/WxCombatLibrary.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase`(또는 `UWxAbility_Attack`/`_Skill`/`_Dodge` 등 특화 베이스)를 상속하고 `ActivationInputAction`·`ActivationPolicy`·`ActivationGroup`을 지정한다. 쿨다운·코스트·아이콘은 `AbilityDataRow`(`FWxAbilityTableRow`)에서 읽으므로 GE를 따로 만들지 않는다.
- **부여**: 캐릭터 BP가 `UWxAbilitySet`을 지정하면 서버에서 `GrantedAbilities`/`GrantedEffects`/`AttributeInitRow`가 ASC에 일괄 부여된다. 입력 라우팅 키는 각 어빌리티 CDO의 `ActivationInputAction`이 쥔다.
- **새 GE 수치**: 인스턴스별 값은 `UGameplayEffect`에 `UWxEffectComponent_Table`을 붙여 `FWxEffectTableRow`를 지목하고, `UWxMMC_EffectMagnitude`/`_EffectDuration`이 계산 시점에 읽는다.
- **데미지 저작**: `FWxDamageTableRow`(계수·HitReact 태그·가드/패리 허용·추가 GE)를 행으로 두고, `WxAnimNotifyState_WeaponAttack`이나 `UWxCombatLibrary::ApplyDamage`로 구동한다.
- **권위/복제**: 서버 권위 모델. 데미지 경로는 `FWxCombatEffectContext`(크리 판정 운반)를 쓰며, 이를 위해 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`에 `UWxAbilitySystemGlobals`를 등록해야 한다(누락 시 ExecCalc가 ensure). 락온은 서버 권위로 복제하되 대상 선택은 소유 클라를 신뢰한다.

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력이 어빌리티로 흐르는 유일 진입점과 발동 그룹/버퍼/히트스톱 규약
2. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 발동 그룹(Independent/Exclusive/Override)·캔슬 창(Blocking→ComboWindow→Recovery) 규칙
3. `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 스탯 정의와 데미지/퍼펙트가드가 HP에 반영되는 메타 흐름
4. `Source/WxCombat/Public/Damage/WxDamageTableRow.h` + `WxEffect_Damage.h` — 한 대의 히트가 GE 스펙이 되어 계산되는 파이프라인

## 관련
- 상위: 캐릭터/컨트롤러가 있는 [[WxGame]]과 콘텐츠 GameFeature 플러그인이 이 모듈의 ASC·어빌리티를 소비한다.
- foundation: [[WxCore]]

---
*문서 기준 커밋 `b3f982b` · 생성일 2026-08-31 · 소스 159파일 — `/readme-writer`로 갱신*
