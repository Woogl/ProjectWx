# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 올린 액션 RPG 전투의 핵심 도메인. 어빌리티·어트리뷰트·대미지·락온·무기 히트·선입력·연출(Cue/HitStop)을 한데 묶어, 캐릭터가 무엇을 하고 무엇에 맞는지를 서버 권위로 결정한다.

## 책임
**담당**
- 어빌리티 발동 모델: 활성화 정책(`OnTriggered`/`OnGiven`)·배타 그룹(`EWxAbilityActivationGroup`)·발동 중 캔슬 창(`EWxAbilityActionPhase`: Blocking → ComboWindow → Recovery)
- 어트리뷰트(HP/SP/GP/MP/UP + ATK/DEF/Crit/SPD/ASPD)와 대미지 판정·크리 전달·사망/그로기 트리거
- 라이브 입력 라우팅과 선입력 버퍼(발동 실패 시 기억했다 캔슬 창에서 재시도)
- 무기 히트박스 스윕/오버랩, 투사체·소환수·처형(Finisher) 피해 적용
- 락온(SceneComponent 단위, 서버 복제)과 타겟팅 필터/정렬 태스크
- 전투 연출: GameplayCue, 히트스톱, 카메라/몽타주 스냅 AnimNotify

**경계 (비담당)**
- 표시용 문자열/아이콘 데이터의 소비(UI 렌더)는 [[WxUI]]. 이 모듈은 `IWxUIData`([[WxCore]] 정의)를 구현해 데이터만 노출한다
- 적 행동 결정(패턴 선택·이동)은 [[WxAI]]. 이 모듈은 AI가 부를 어빌리티(`WxAbility_Pattern` 등)와 소환 빙의 후크까지만
- 팀/적대 판정의 근원 데이터는 [[WxCore]]

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 전투 ASC. 라이브 입력 라우팅·몽타주 재생·배타 어빌리티 캔슬의 유일 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilitySet` | 캐릭터 BP가 지정하는 데이터 에셋. init 시점 서버에서 어빌리티·GE·어트리뷰트 초기값 일괄 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxAbilityBase` | 모든 어빌리티의 추상 베이스. 활성화 정책·배타 그룹·`AbilityDataRow`(쿨/코스트) 저작 지점 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | 전 어트리뷰트 정의와 `PostGameplayEffectExecute`의 대미지→사망/그로기 처리 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxCombatLibrary` | `ApplyDamage`/`CheckDamage`/`IsHostile` — 히트 성립 판정과 대미지 적용의 공용 헬퍼 | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `FWxDamageTableRow` | 공격 1건의 대미지 저작(계수·크리/가드/패리 허용·피격 반응 태그·추가 GE) | `Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageTableRow.h` |
| `AWxWeaponBase` | 무기 액터. BP가 붙인 ShapeComponent를 매 틱 스윕해 스윙당 1회 히트 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxInputBufferComponent` | 선입력. 발동 실패 입력을 기억했다 캔슬 창 전이에서 재시도 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxInputBufferComponent.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase` 파생(대개 `WxAbility_*` 중 하나를 이어) → `UWxAbilitySet::GrantedAbilities`에 등록. 쿨다운·코스트는 `AbilityDataRow`(`FWxAbilityTableRow`)에서 읽고, 코스트는 공용 `UWxEffect_Cost`가, 쿨다운은 `UWxEffect_Cooldown` 파생 GE가 그 값을 소비한다.
- **새 상태/대미지 GE**: `UGameplayEffect`에 `UWxEffectComponent_Table`을 붙여 `FWxEffectTableRow`를 지목하면 MMC가 계산 시점에 행을 읽는다(스펙에 값을 싣지 않는다). 표시 데이터는 이 컴포넌트가 `IWxUIData`로 노출.
- **데이터 주도 저작**: 어빌리티는 `FWxAbilityTableRow`, 공격은 `FWxDamageTableRow`, GE 수치는 `FWxEffectTableRow`, 어트리뷰트 초기값은 `WxCombatAttributeInitTableRow`가 구동한다.
- **크리 전달**: 어트리뷰트로 못 싣는 크리 판정은 `FWxCombatEffectContext`에 실린다. `UWxAbilitySystemGlobals`를 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`에 등록해야 이 컨텍스트가 만들어진다(누락 시 `UWxExecCalc_Damage`가 ensure).
- **리플리케이션/권위**: 대미지·소환·처형은 서버 권위. 락온은 서버 권위로 전 머신 복제하되 소유 클라가 로컬 예측 후 요청하며(대상 선택은 클라 신뢰), 히트스톱은 GE 인스턴스/복제 태그로 각 머신이 자기 판정.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 발동 정책·배타 그룹·캔슬 창 3개 enum이 전투 흐름의 문법이다. 여기부터.
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력이 어떻게 어빌리티로, 실패가 어떻게 버퍼로 가는지 라우팅의 중심.
3. `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` + `Damage/WxDamageTableRow.h` — 히트 성립 판정과 대미지 저작이 만나는 대미지 파이프라인 입구.

## 관련
- 상위: 캐릭터/게임 조립은 [[WxGame]], 콘텐츠 활성화는 GameFeature 플러그인. 어트리뷰트·데이터 표시는 [[WxUI]], 적 행동은 [[WxAI]]가 이 모듈의 어빌리티를 구동한다. 공용 정의·인터페이스는 [[WxCore]].

---
*문서 기준 커밋 `f0aad4c` · 생성일 2026-09-03 · 소스 169파일 — `/readme-writer`로 갱신*
