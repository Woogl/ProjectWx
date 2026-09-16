# WxCombat — 전투 시스템

> Gameplay Ability System(GAS) 위에 세운 액션 RPG 전투 전반을 담당한다. 어빌리티·어트리뷰트·게임플레이 이펙트·대미지 파이프라인·락온/타겟팅·무기/투사체/소환물·처형까지 실제 전투 루프를 이 모듈이 굴린다.

## 책임
**담당**
- ASC 허브와 입력 라우팅(라이브 경로 + 선입력 버퍼), AbilitySet 기반 어빌리티·이펙트·어트리뷰트 일괄 부여
- 전투 어트리뷰트(HP/SP/GP/MP/UP, ATK/DEF, Crit, SPD/ASPD, GuardReductionScale)와 상한 클램프·사망·그로기 규칙
- 대미지 파이프라인: Hit Wrapper GE → ExecCalc → 가드/퍼펙트가드/무적/반사 판정과 후속 반응
- 어빌리티 발동 배타성(ActivationGroup)과 발동 중 캔슬 창(ActionPhase: Blocking→ComboWindow→Recovery)
- 무기 히트박스 스윕, 투사체·소환물 서버 권위 스폰, 처형(Finisher) 피해
- 락온/타겟팅(TargetingSystem 필터·소터 태스크), 모션 워핑 스냅/러시, 히트스톱·슬로우타임·스킬 컷신 연출 훅

**경계 (비담당)**
- 어빌리티·이펙트의 표시 데이터(제목/설명/아이콘)는 `IWxUIData`로 노출만 하고 실제 위젯은 [[WxUI]]
- 공용 GameplayTag 선언(`WxGameplayTags`)·콜리전 채널·`IWxUIData` 같은 공용 정의는 [[WxCore]] (이 모듈은 네이티브 태그를 직접 선언하지 않는다)
- 적·소환물의 행동 결정과 퍼셉션 타겟 주입은 [[WxAI]] (전투는 `UWxLockOnComponent`라는 그릇과 스폰 경로만 제공)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | ASC 허브. 입력→어빌리티 라우팅의 유일한 진입점이자 AbilitySet 부여·몽타주 재생속도·틱 정책의 주인 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilitySet` | 캐릭터 BP가 ASC에 꽂는 DataAsset. 어빌리티·이펙트·어트리뷰트 초기화 행을 묶어 서버에서 일괄 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스이자 배타/캔슬 모델의 정의처. 쿨·코스트를 데이터 행에서 읽는 규약도 여기 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | 전투 수치의 단일 보관처. 클램프·사망·그로기 판정과 ExecCalc의 메타 어트리뷰트 착지점 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxCombatLibrary` | 모듈 바깥이 전투에 말을 거는 정적 진입점(`ApplyDamage`/`ApplyEffect`) | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `FWxHitEffectContext` | 대미지 GE가 파이프라인 전 구간에 실어 나르는 컨텍스트. 방어 판정과 실행 결과가 여기 모인다 | `Plugins/WxCombat/Source/WxCombat/Public/Damage/WxHitEffectContext.h` |
| `UWxInputBufferComponent` | 선입력 정책의 주인. ASC가 라우팅만 하는 대신 "무엇을 얼마나 기억할지"를 여기서 정한다 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxInputBufferComponent.h` |
| `AWxWeaponBase` | 몽타주 노티파이가 켜고 끄는 히트 판정 주체. 대미지 행을 받아 `ApplyDamage`로 흘려보낸다 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase` 상속(`Public/AbilitySystem/Ability/WxAbility_*`). `AbilityDataRow`(`FWxAbilityTableRow`)에서 쿨다운·코스트를 읽고, `EWxAbilityActivationGroup`(Independent/Exclusive/Override)과 활성화마다 Blocking에서 다시 시작하는 `EWxAbilityActionPhase`를 따른다. 코스트는 공용 `UWxEffect_Cost`, 쿨다운은 `UWxEffect_Cooldown` 파생 GE가 그 행 수치를 쓴다. 콤보는 `UWxAbility_Attack`처럼 콤보 창 안의 재발동으로 진행한다.
- **새 이펙트**: `UGameplayEffect` 파생(`Public/AbilitySystem/Effect/WxEffect_*`). 수치·표시 데이터를 스펙에 싣지 않고 `UWxEffectComponent_Table`(+`FWxEffectTableRow`)을 GE에 붙이면 `UWxMMC_EffectMagnitude`/`UWxMMC_EffectDuration`이 계산 시점에 GE 정의에서 행을 조회한다. 이 컴포넌트가 `UGameplayEffectUIData` 파생인 것은 WxUI가 WxCombat을 참조할 수 없기 때문이며, 그것이 양쪽이 아는 유일한 조회 앵커다.
- **데이터 주도**: 부여는 `UWxAbilitySet` DataAsset, 수치는 DataTable 행(`FWxAbilityTableRow`·`FWxEffectTableRow`·`FWxDamageTableRow`·`FWxCombatAttributeInitTableRow`). 한 캐릭터에 여러 세트를 꽂으면 뒤 세트의 어트리뷰트 행이 앞 세트를 덮는다.
- **리플리케이션/권한(최대 4인 멀티)**: 대미지·이펙트 적용·투사체/소환물 스폰은 모두 서버 권위. 락온 대상은 서버 권위로 복제하되 선택 자체는 클라이언트 신뢰(서버 미재검증, 소유 클라의 락온 태스크가 무효화를 폴링해 다시 올린다).
- **연출 훅**: `UWxCueNotify_*`(GameplayCue)와 `Public/AnimNotify/WxAnimNotify(State)_*`(무기 판정·GE 적용·소환·투사체·모션워핑·슬로우타임 트리거). `UWxAbilitySystemGlobals`를 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`에 등록해야 큐 위치가 히트 결과에서 채워진다 — 빠지면 큐가 원점에서 터진다.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` — 캐릭터에 무엇이 어떻게 부여되는지. 시스템 진입 구조가 여기서 잡힌다
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 발동/배타/캔슬 창 모델. 콤보·후딜·선입력이 모두 이 모델 위에 얹혀 있다
3. `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` — 피해 진입점. 여기서 `WxEffect_Hit`(Wrapper) → `WxExecCalc_Damage` → `WxEffectComponent_Hit`/`_DamageResponse` 순으로 따라가면 대미지 파이프라인 전체가 보인다

## 관련
- 상위: [[WxCore]] · 협력: [[WxUI]], [[WxAI]]

---
*문서 기준 커밋 `047197a` · 생성일 2026-09-16 · 소스 187파일 — `/readme-writer`로 갱신*
