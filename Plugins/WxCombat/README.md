# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 세운 액션 RPG 전투 런타임. 어빌리티·어트리뷰트·데미지 파이프라인, 락온/타겟팅, 소환물·투사체, 몽타주 구동 판정을 서버 권위로 굴린다.

## 책임
**담당**
- GAS 확장: 커스텀 ASC(입력 라우팅·몽타주 재생 정책), `UWxAbilityBase`(발동 그룹/캔슬 창), 전투 어트리뷰트 세트, 데미지 ExecCalc.
- 데미지 파이프라인: `FWxDamageTableRow` → GE Spec 생성 → SP/GP/HP 반영 → 가드·퍼펙트가드·회피·크리티컬 반응.
- 입력 처리: Enhanced Input을 어빌리티 발동으로 잇는 라우팅과 선입력 버퍼.
- 타겟팅/락온, 소환물(`UWxMinionSubsystem`)·투사체(`UWxProjectileSubsystem`) 생성, 몽타주 AnimNotify로 여는 판정 창.

**경계 (비담당)**
- 팀 관계·공용 정의·UI 데이터 인터페이스(`IWxUIData`)의 원본은 [[WxCore]].
- 어빌리티 아이콘/설명의 실제 화면 표시는 [[WxUI]].
- AI의 락온 대상 주입·의사결정은 [[WxAI]] (`AWxAIController`가 퍼셉션 타겟을 `UWxLockOnComponent`에 채운다).

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 라이브 입력 라우팅·몽타주 재생 속도·어빌리티 세트 부여의 단일 창구 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 전투 어빌리티의 베이스. 발동 그룹(Exclusive/Override)과 캔슬 창(Blocking→ComboWindow→Recovery) 규약 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | HP/SP/GP/MP/UP·ATK/DEF·크리·속도 등 전투 수치와 클램프·메타 어트리뷰트 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxAbilitySet` | 캐릭터 BP가 넣으면 어트리뷰트 초기화·어빌리티·GE를 서버에서 일괄 부여하는 데이터 에셋 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatLibrary` | 적대 판정·데미지 성립 검사·데미지/상태 GE 적용의 static 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `FWxDamageTableRow` | 데미지 계수·피격반응 태그·가드/패리 규칙·추가 GE의 데이터 주도 정의 | `Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageTableRow.h` |
| `UWxLockOnComponent` | 겨누는 대상(SceneComponent)을 서버 권위로 복제해 방향·스냅·필터 소비처에 공급 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnComponent.h` |
| `UWxMinionSubsystem` | 소환물을 서버 권위로 생성·주인별 로스터 관리·일괄 어빌리티 명령 | `Plugins/WxCombat/Source/WxCombat/Public/Minion/WxMinionSubsystem.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase` 파생. `AbilityDataRow`(쿨다운·코스트)·`ActivationInputAction`(입력 키)·`ActivationGroup`을 CDO에 선언하고, 필요 시 `EWxAbilityActionPhase` 창을 몽타주 노티파이로 전이시킨다.
- 새 데미지/상태 효과: `Effect/`의 `UWxEffect_*` 파생(GE) 또는 `FWxDamageTableRow.AdditionalEffects`로 데이터에서 붙인다. 최종 데미지는 `UWxExecCalc_Damage`가 SP·GP·IncomingDamage 순으로 반영한다.
- 캐릭터 구성은 코드가 아니라 `UWxAbilitySet` 에셋(+ `WxCombatAttributeInitTableRow`·`WxAbilityTableRow`·`WxEffectTableRow` 데이터테이블)으로 주도한다.
- 타겟팅 필터/정렬은 `Targeting/`의 `WxTargetingFilterTask_*`·`WxTargetingSorterTask_*`(TargetingSystem 플러그인 태스크)로 확장한다.
- 리플리케이션: 데미지·소환·투사체 생성은 서버 권위, 락온 대상은 소유 클라 예측 후 서버 반영. 어트리뷰트는 복제, 메타(IncomingDamage/Reflect)는 비복제.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 발동 그룹·캔슬 창·쿨다운 규약. 전투 흐름의 골격.
2. `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` — 히트가 데미지 GE로 이어지는 진입점(`ApplyDamage`/`CheckDamage`).
3. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력이 어떻게 어빌리티로 라우팅되는지(`UWxInputBufferComponent`와 함께).
4. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 수치 정의와 데미지 반영 지점.

## 관련
- 상위: 캐릭터/폰이 ASC와 `UWxAbilitySet`을 통해 이 모듈을 소비한다. 함께 보기 — [[WxCore]](팀·`IWxUIData` 원본), [[WxAI]](AI 락온·명령), [[WxUI]](어빌리티 표시).

---
*문서 기준 커밋 `d1674fa` · 생성일 2026-09-12 · 소스 181파일 — `/readme-writer`로 갱신*
