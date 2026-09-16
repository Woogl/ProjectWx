# WxCombat — 전투 시스템

> Gameplay Ability System(GAS) 위에 세운 액션 RPG 전투 전반을 담당한다. 어빌리티·어트리뷰트·게임플레이 이펙트·대미지 판정·락온/타겟팅·무기/투사체/소환물·처형까지 실제 전투 루프를 이 모듈이 굴린다.

## 책임
**담당**
- ASC 허브와 입력 라우팅(라이브/선입력 버퍼), AbilitySet 기반 어빌리티·이펙트·어트리뷰트 일괄 부여
- 전투 어트리뷰트(HP/SP/GP/MP/UP, ATK/DEF, Crit, SPD/ASPD)와 그 상한·사망·그로기 규칙
- 대미지 파이프라인: Hit Wrapper GE → ExecCalc → 방어/무적/반사 판정과 후속 반응
- 어빌리티 발동 배타성(ActivationGroup)과 발동 중 캔슬 창(ActionPhase: Blocking→ComboWindow→Recovery)
- 무기 히트박스 스윕, 투사체·소환물 서버 권위 스폰, 처형(Finisher) 피해
- 락온/타겟팅(TargetingSystem 필터·소터 태스크), 모션 워핑 스냅/러시, 히트스톱·슬로우타임 연출 훅

**경계 (비담당)**
- 어빌리티·이펙트의 표시 데이터(제목/설명/아이콘)는 `IWxUIData`로 노출만 하고 실제 위젯은 [[WxUI]]
- 공용 GameplayTag 선언(`WxGameplayTags`)과 공용 정의·인터페이스(`IWxUIData`)는 [[WxCore]]
- 소환물/적의 행동 결정과 퍼셉션 타겟 주입은 [[WxAI]] (전투는 락온 컴포넌트라는 그릇만 제공)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | ASC 허브. 입력→어빌리티 라우팅, AbilitySet 부여, 몽타주 재생속도·틱 정책 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilitySet` | 캐릭터 BP가 ASC에 꽂는 DataAsset. 어빌리티·이펙트·어트리뷰트 초기화 행을 묶어 일괄 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 활성화 정책/배타 그룹/캔슬 페이즈, 쿨·코스트 데이터 행 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | 전투 어트리뷰트 집합과 클램프·사망·그로기 처리 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxCombatLibrary` | `ApplyDamage`/`ApplyEffect` 정적 진입점(서버 권위 대미지·구간 상태 부여) | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `UWxInputBufferComponent` | 선입력. 실패한 입력을 기억했다 캔슬 창/어빌리티 종료에 재시도 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxInputBufferComponent.h` |
| `AWxWeaponBase` | 무기 히트박스(Overlap+매 틱 Sweep) 판정, 스윙당 액터 1회 피격 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxMinionSubsystem` | 소환물 서버 권위 스폰·주인별 로스터·수명 관리 | `Plugins/WxCombat/Source/WxCombat/Public/Minion/WxMinionSubsystem.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase` 상속(`Public/AbilitySystem/Ability/WxAbility_*`). `AbilityDataRow`(`FWxAbilityTableRow`)에서 쿨다운·코스트 읽고, `EWxAbilityActivationGroup`(Independent/Exclusive/Override)·`EWxAbilityActionPhase`를 선언한다. 코스트는 공용 `UWxEffect_Cost`, 쿨다운은 `UWxEffect_Cooldown` 파생 GE가 그 행 수치를 쓴다.
- **새 이펙트**: `UGameplayEffect` 파생(`Public/AbilitySystem/Effect/WxEffect_*`). 수치/표시 데이터는 스펙에 싣지 않고 `UWxEffectComponent_Table`(+`FWxEffectTableRow`)를 GE에 붙여 MMC가 계산 시점에 조회한다. 대미지 계산은 `UWxExecCalc_Damage`.
- **데이터 주도**: 부여는 `UWxAbilitySet` DataAsset, 수치는 DataTable 행(`FWxAbilityTableRow`·`FWxEffectTableRow`·`FWxDamageTableRow`·`FWxCombatAttributeInitTableRow`).
- **리플리케이션/권한**: 대미지·이펙트 적용·투사체/소환물 스폰은 모두 서버 권위. 락온 대상 선택은 클라이언트 신뢰(서버 미재검증, 소유 클라 태스크가 무효화 폴링). 대미지 컨텍스트는 `FWxHitEffectContext`로 방어/반사/적용 결과를 실어 나른다.
- **연출 훅**: `UWxCueNotify_*`(GameplayCue), `Public/AnimNotify/WxAnimNotify(State)_*`(무기 판정·이펙트·소환·투사체·모션워핑 트리거). `UWxAbilitySystemGlobals`를 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`에 등록해야 큐 위치가 히트 결과에서 채워진다.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` — 캐릭터에 무엇이 어떻게 부여되는지, 시스템 진입 구조가 여기서 잡힌다
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티 발동/배타/캔슬 창 모델. 콤보·후딜·선입력의 근거
3. `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` — 대미지·상태 부여 진입점. 여기서 `WxEffect_Hit`(Wrapper)→`WxExecCalc_Damage`로 흐름을 따라가면 피해 파이프라인 전체가 보인다

## 관련
- 상위: [[WxCore]] · 협력: [[WxUI]], [[WxAI]]

---
*문서 기준 커밋 `dc08752` · 생성일 2026-09-14 · 소스 187파일 — `/readme-writer`로 갱신*
