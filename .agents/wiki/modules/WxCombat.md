# WxCombat — 전투 시스템

> 상태: needs-review · 2026-09-20 이관 · 원문 기준 커밋: 2872e9a
> 기존 README를 이관했습니다. 전체 코드 재검증은 하지 않았습니다. 아래 과거 설명은 탐색에 사용하고 변경 전 원자료를 확인하세요. 에셋 내부는 미검증입니다.
> [Wiki 목차](../index.md) · [운영 절차](../maintenance.md)


> Gameplay Ability System(GAS) 위에 세운 액션 RPG 전투 전반을 담당한다. 어빌리티·어트리뷰트·이펙트·대미지 파이프라인·락온/타겟팅·무기/투사체/소환물·처형까지 실제 전투 루프를 이 모듈이 굴린다.

## 책임
**담당**
- ASC 허브와 입력 라우팅(라이브 경로 + 선입력 버퍼), `UWxAbilitySet` 기반 어빌리티·이펙트·어트리뷰트 일괄 부여
- 전투 어트리뷰트(HP/SP/GP/MP/UP, ATK/DEF, Crit, SPD/ASPD, GuardReductionScale)와 상한 클램프·사망·그로기 규칙
- 대미지 파이프라인: Hit Wrapper GE → ExecCalc → 가드/퍼펙트가드/무적/반사 판정 → 후속 반응·플로터
- 어빌리티 발동 배타성(`EWxAbilityActivationGroup`)과 발동 중 캔슬 창(`EWxAbilityActionPhase`: Blocking→ComboWindow→Recovery)
- 무기 히트박스 스윕, 투사체·소환물 서버 권위 스폰, 처형(Finisher) 피해
- 락온/타겟팅(TargetingSystem 필터·소터 태스크), 모션 워핑 스냅/러시, 히트스톱·슬로우타임·스킬 컷신 연출 훅

**경계 (비담당)**
- 어빌리티·이펙트의 표시 데이터(제목/설명/아이콘)는 `IWxUIData`로 노출만 하고 실제 위젯·HUD는 [WxUI](WxUI.md)
- 공용 GameplayTag 선언(`WxGameplayTags`)·콜리전 채널·`IWxUIData` 같은 공용 정의는 [WxCore](WxCore.md). 대부분 WxCore의 태그를 소비하지만, 현재 `WxEffect_IgnoreAbilityTags.h`에는 `WxCombatGameplayTags::Effect_IgnoreAbilityTags` 자체 선언이 있다. 통합 여부는 [Q-004](../questions/open-questions.md)에서 추적한다(2026-09-20 확인)
- 적·소환물의 행동 결정과 퍼셉션 타겟 주입은 [WxAI](WxAI.md) — 전투는 `UWxLockOnComponent`라는 그릇과 `UWxAbility_Pattern` 같은 발동 대상만 제공한다
- 캐릭터 액터·컴포넌트 조립(ASC/LockOn 컴포넌트 소유)과 입력 매핑은 `WxGame`의 `AWxCharacterBase` 계열

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | ASC 허브. 입력→어빌리티 라우팅의 유일한 진입점이자 AbilitySet 부여·몽타주 재생속도·메시 틱 정책의 주인 | [Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h) |
| `UWxAbilitySet` | 캐릭터 BP가 ASC에 꽂는 DataAsset. 어빌리티·이펙트·어트리뷰트 초기화 행을 묶어 서버에서 일괄 부여 | [Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h) |
| `UWxAbilityBase` | 모든 어빌리티의 베이스이자 배타/캔슬 모델의 정의처. 쿨·코스트를 데이터 행에서 읽는 규약도 여기 | [Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h) |
| `UWxCombatAttributeSet` | 전투 수치의 단일 보관처. 클램프·사망·그로기 판정과 ExecCalc가 실어 보낸 메타 어트리뷰트의 착지점 | [Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h) |
| `UWxCombatLibrary` | 모듈 바깥이 전투에 말을 거는 정적 진입점(`ApplyDamage`/`ApplyEffect`). 무기·투사체·노티파이가 전부 여기로 모인다 | [Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h](../../../Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h) |
| `FWxHitEffectContext` | 대미지 GE가 파이프라인 전 구간에 실어 나르는 컨텍스트. 대미지 행 지목·방어 판정·실행 결과가 여기 모인다 | [Plugins/WxCombat/Source/WxCombat/Public/Damage/WxHitEffectContext.h](../../../Plugins/WxCombat/Source/WxCombat/Public/Damage/WxHitEffectContext.h) |
| `UWxInputBufferComponent` | 선입력 정책의 주인. ASC가 라우팅만 하는 대신 "무엇을 얼마나 오래 기억할지"를 여기서 정한다 | [Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxInputBufferComponent.h](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxInputBufferComponent.h) |
| `AWxWeaponBase` | 몽타주 노티파이가 켜고 끄는 근접 히트 판정의 주체. 받은 대미지 행을 `ApplyDamage`로 흘려보낸다 | [Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h](../../../Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h) |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase` 상속(`Public/AbilitySystem/Ability/WxAbility_*`). `AbilityDataRow`(`FWxAbilityTableRow`)에서 쿨다운·충전 수·코스트를 읽고, `EWxAbilityActivationGroup`(Independent/Exclusive/Override)과 활성화마다 Blocking에서 다시 시작하는 `EWxAbilityActionPhase`를 따른다. 코스트는 공용 `UWxEffect_Cost`, 쿨다운은 `UWxEffect_Cooldown` 파생 GE가 같은 행 수치를 쓴다. 플레이어 콤보는 `UWxAbility_Attack`/`UWxAbility_Skill`처럼 콤보 창 안의 재발동으로, AI 패턴은 `UWxAbility_Pattern`처럼 한 번의 발동이 배열 전체를 재생하는 방식으로 나뉜다.
- **새 이펙트**: `UGameplayEffect` 파생(`Public/AbilitySystem/Effect/WxEffect_*`). 수치·표시 데이터를 스펙에 싣지 않고 `UWxEffectComponent_Table`(+`FWxEffectTableRow`)을 GE에 붙이면 `UWxMMC_EffectMagnitude`/`UWxMMC_EffectDuration`이 계산 시점에 GE 정의에서 행을 조회한다. 이 컴포넌트가 `UGameplayEffectUIData` 파생인 것은 WxUI가 WxCombat을 참조할 수 없어 양쪽이 아는 엔진 클래스가 유일한 조회 앵커이기 때문이다.
- **데이터 주도**: 부여는 `UWxAbilitySet` DataAsset, 수치는 DataTable 행(`FWxAbilityTableRow`·`FWxEffectTableRow`·`FWxDamageTableRow`·`FWxCombatAttributeInitTableRow`). 한 캐릭터에 여러 세트를 꽂으면 뒤 세트의 어트리뷰트 행이 앞 세트를 덮는다. 공격 1건의 성질(계수·크리/가드/패리 허용·HitReact 태그·추가 GE)은 전부 `FWxDamageTableRow` 한 행이 쥔다.
- **리플리케이션/권한(최대 4인 멀티)**: 대미지 적용·상태 GE·투사체/소환물 스폰은 모두 서버 권위. 락온 대상만은 서버 권위로 복제하되 선택 자체는 클라이언트 신뢰(서버 미재검증, 소유 클라의 락온 태스크가 무효화를 폴링해 다시 올린다). 히트스톱·ImpactFX 등 연출은 복제된 태그/로컬 충돌로 각 머신이 따로 재생한다.
- **연출 훅**: `UWxCueNotify_*`(GameplayCue)와 `Public/AnimNotify/WxAnimNotify(State)_*`(무기 판정·AOE 대미지·GE 적용·소환·투사체·모션워핑·슬로우타임·콤보 창/후딜 전이). `UWxAbilitySystemGlobals`를 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`에 등록해야 큐 위치가 히트 결과에서 채워진다 — 빠지면 큐가 원점에서 터진다.

## 여기서부터 읽어라
1. [Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h) — 캐릭터에 무엇이 어떻게 부여되는지. 시스템 진입 구조가 여기서 잡힌다
2. [Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h) — 발동/배타/캔슬 창 모델. 콤보·후딜·선입력이 모두 이 모델 위에 얹혀 있다
3. [Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h](../../../Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h) — 피해 진입점. 여기서 `WxEffect_Hit`(Wrapper) → `WxExecCalc_Damage` → `WxEffectComponent_Hit`/`WxEffectComponent_DamageResponse` 순으로 따라가면 대미지 파이프라인 전체가 보인다

## 관련
- 상위: `WxGame`(유일한 소비 모듈 — 캐릭터가 ASC·락온 컴포넌트를 소유하고 어빌리티를 꽂는다)
- 기반: [WxCore](WxCore.md) · 협력: [WxUI](WxUI.md), [WxAI](WxAI.md)

---
*문서 기준 커밋 `2872e9a` · 생성일 2026-09-17 · 소스 187파일 — `/readme-writer`로 갱신*
