# WxCombat — 전투 시스템

작업 단계: 구현

> 상태: current · 범위: 본문 핵심 계약·주요 C++ 경로의 정적 재검증 · 2026-09-20 · 기준 커밋: 5a3f282e3bd71fd85d263021c113cd30558820b7
> 아래 검증 범위와 근거에 명시한 경로를 현재 작업 트리에서 확인했습니다. 전체 소스의 결함 검토·빌드·게임 실행·BP/WBP·DataTable·BT/StateTree 에셋 내부는 미검증입니다.
> [Wiki 목차](../index.md) · [운영 절차](../maintenance.md)


> Gameplay Ability System(GAS) 위에 세운 액션 RPG 전투 전반을 담당한다. 어빌리티·어트리뷰트·이펙트·대미지 파이프라인·락온/타겟팅·무기/투사체/소환물·처형까지 실제 전투 루프를 이 모듈이 굴린다.

## 책임
**담당**
- ASC 허브와 입력 라우팅(라이브 경로 + 선입력 버퍼), `UWxAbilitySet` 기반 어빌리티·이펙트·어트리뷰트 일괄 부여
- 전투 어트리뷰트(HP/SP/GP/MP/UP, ATK/DEF, Crit, SPD/ASPD, GuardReductionScale)와 상한 클램프·사망·그로기 규칙
- 대미지 파이프라인: Hit Wrapper의 적대·권한·무적 및 가드 판정 → Damage GE의 ExecCalc·속성 반영·결과 수집 → Hit의 후속 반응·추가 효과
- 어빌리티 발동 배타성(`EWxAbilityActivationGroup`)과 발동 중 캔슬 창(`EWxAbilityActionPhase`: Blocking→ComboWindow→Recovery)
- 무기 히트박스 스윕, 투사체·소환물 서버 권위 스폰, 처형(Finisher) 피해
- 락온/타겟팅(TargetingSystem 필터·소터 태스크), 모션 워핑 스냅/러시, 히트스톱·슬로우타임·스킬 컷신 연출 훅

**경계 (비담당)**
- 어빌리티·이펙트의 표시 데이터(제목/설명/아이콘)는 `IWxUIData`로 노출만 하고 실제 위젯·HUD는 [WxUI](WxUI.md)
- 공용 GameplayTag 선언(`WxGameplayTags`)·콜리전 채널·`IWxUIData` 같은 공용 정의는 [WxCore](WxCore.md). 자체 태그는 선언하지 않고 WxCore의 태그만 소비한다(2026-09-21 확인)
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
- **리플리케이션/권한**: `ApplyDamage`와 소환물 생성은 서버 권위를 검사한다. `UWxAbilityBase`의 기본 실행 정책은 LocalPredicted이며, `ApplyEffect`도 예측 키를 사용하므로 모든 GE가 서버 전용인 것은 아니다. 락온 대상은 서버에서 복제하지만 선택 자체는 클라이언트를 신뢰한다. 각 어빌리티·연출의 실제 멀티 동작은 별도 실행 검증이 필요하다.
- **연출 훅**: `UWxCueNotify_*`(GameplayCue)와 `Public/AnimNotify/WxAnimNotify(State)_*`(무기 판정·AOE 대미지·GE 적용·소환·투사체·모션워핑·슬로우타임·콤보 창/후딜 전이). `UWxAbilitySystemGlobals`를 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`에 등록해야 큐 위치가 히트 결과에서 채워진다 — 빠지면 큐가 원점에서 터진다.

## 여기서부터 읽어라
1. [Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h) — 캐릭터에 무엇이 어떻게 부여되는지. 시스템 진입 구조가 여기서 잡힌다
2. [Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h](../../../Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h) — 발동/배타/캔슬 창 모델. 콤보·후딜·선입력이 모두 이 모델 위에 얹혀 있다
3. [Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h](../../../Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h) — `ApplyDamage` → `WxEffect_Hit`/`WxEffectComponent_Hit` → `WxEffect_Damage`/`WxExecCalc_Damage` → `WxEffectComponent_DamageResponse`로 결과를 수집한 뒤 Hit의 후속 처리로 돌아온다.

## 검증 범위와 근거

[Build.cs](../../../Plugins/WxCombat/Source/WxCombat/WxCombat.Build.cs)·[descriptor](../../../Plugins/WxCombat/WxCombat.uplugin), [ASC](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp)·[AbilitySet](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp)의 부여·입력 경로, [AbilityBase](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp)의 발동·종료·비용·쿨다운 경로를 확인했다. AbilitySet 자체가 권한을 검사하는 것은 아니며, 현재 캐릭터의 호출부가 서버에서 부여한다.

- [Hit 처리](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_Hit.cpp)와 [결과 수집](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffectComponent_DamageResponse.cpp)을 대조해 위 처리 순서를 정정했다. 피해·쿨다운 수치가 들어 있는 실제 DataTable은 확인하지 않았다.
- [MinionComponent](../../../Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionComponent.cpp)·[MinionSubsystem](../../../Plugins/WxCombat/Source/WxCombat/Private/Minion/WxMinionSubsystem.cpp): 네이티브 CDO 컴포넌트가 상한·주인 상태 태그·소환/취소 조건을 선언한다. 주인은 Instigator에서 찾고, 같은 주인 상태 태그의 소환물에 상한을 적용한다. 0은 무제한이며 초과 시 오래된 소환물을 제거한다. 소환물의 재소환은 거부한다.
- [AbilityBase](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Ability/WxAbilityBase.cpp): `Effect.IgnoreCosts`·`Effect.IgnoreCooldowns`가 검사와 적용을 모두 우회한다. `Effect.IgnoreAbilityTags`는 태그 조건을 우회하지만 활성화 그룹 검사까지 제거하지 않는다. `ActivationOwnedEffects`는 활성화에서 적용하고 종료 시 서버에서 제거한다.
- [Combat 설정](../../../Plugins/WxCombat/Source/WxCombat/Public/System/WxCombatDeveloperSettings.h)의 `DefenseConstant`는 [피해 계산](../../../Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxEffect_Damage.cpp)에서 읽는다. AnimNotify 표시색도 이 설정을 사용한다.

그로기의 검증 범위는 [시스템 페이지](../systems/groggy.md)를 참조한다. 전체 어빌리티·노티파이·무기·컷신의 실행과 바이너리 에셋 배선은 이번 정적 확인에 포함하지 않았다.

## 관련
- 상위: `WxGame`(유일한 소비 모듈 — 캐릭터가 ASC·락온 컴포넌트를 소유하고 어빌리티를 꽂는다)
- 기반: [WxCore](WxCore.md) · 협력: [WxUI](WxUI.md), [WxAI](WxAI.md)

---
*이관 원문의 기준 커밋 `2872e9a` · 생성일 2026-09-17 · 소스 187파일 — 원문 출처 보존; 현재 확인 범위는 상단과 검증 절 참고*
