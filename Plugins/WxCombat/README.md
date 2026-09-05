# WxCombat — 전투 시스템

> GAS(Gameplay Ability System)를 토대로 어빌리티 발동·데미지 판정·어트리뷰트·락온·타겟팅·투사체까지 액션 전투의 런타임 규칙을 담는 도메인 플러그인이다.

## 책임
**담당**
- 어빌리티 파이프라인: 발동 그룹(Independent/Exclusive/Override)·캔슬 창(Blocking→ComboWindow→Recovery) 배타 점유, 입력 라우팅·선입력 버퍼, 어빌리티셋 일괄 부여
- 어트리뷰트·데미지: HP/SP/GP/MP/UP·ATK/DEF·크리 등 `UWxCombatAttributeSet`, ExecCalc 데미지 산출과 가드·퍼펙트가드·히트스톱 반응
- 상태·연출 이펙트: 다수의 `UWxEffect_*` GameplayEffect, `UWxCueNotify_*` 큐, `UWxAnimNotify*` 몽타주 노티파이(콤보 창·무기 히트·스냅 등)
- 조준·타겟팅: `UWxLockOnComponent` 락온 대상 복제, TargetingSystem 필터/소터 태스크, MotionWarping 스냅
- 무기·피니셔 컴포넌트, 투사체·소환물의 서버 권위 스폰(월드 서브시스템)

**경계 (비담당)**
- 공용 정의(팀·`WxUIData` 등)는 [[WxCore]]에 위임
- AI 컨트롤러의 퍼셉션 타겟 주입·행동 결정은 [[WxAI]] (락온 컴포넌트는 그 값을 받기만 함)
- UI 위젯 표현은 [[WxUI]] (여기선 `IWxUIData` 구현·데미지 플로터 큐까지만)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 라이브 입력 라우팅·어빌리티셋 부여·몽타주 재생의 ASC 진입점 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 발동 그룹·캔슬 창·쿨다운/코스트 테이블 규약 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 캐릭터 BP가 지정하는 부여 데이터 에셋(어빌리티·GE·어트리뷰트 초기화) | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | 전투 어트리뷰트 정의와 데미지/퍼펙트가드 후처리 | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxCombatLibrary` | 히트→데미지/이펙트 적용의 공용 진입점(예측·투사체 임팩트 공유) | `Source/WxCombat/Public/WxCombatLibrary.h` |
| `UWxInputBufferComponent` | 발동 실패 입력의 선입력 버퍼(캔슬 창에서 재시도) | `Source/WxCombat/Public/AbilitySystem/WxInputBufferComponent.h` |
| `UWxLockOnComponent` | 겨누는 대상(SceneComponent)을 서버 권위로 복제 보관 | `Source/WxCombat/Public/Targeting/WxLockOnComponent.h` |
| `UWxProjectileSubsystem` | 몽타주 노티파이가 부르는 서버 권위 투사체 스폰 진입점(월드 서브시스템) | `Source/WxCombat/Public/Weapon/WxProjectileSubsystem.h` |
| `UWxMinionSubsystem` | 주인별 소환물 로스터·상한·명령·주인 소멸 시 정리(월드 서브시스템). 소환물이 살아 있는 동안 주인 ASC에 `State.Minion.Active`를 발행하고, 소환물은 소환자가 될 수 없다 | `Source/WxCombat/Public/Minion/WxMinionSubsystem.h` |
| `FWxCombatEffectContext` | 크리 판정 등 어트리뷰트로 못 싣는 데미지 메타 전달 컨텍스트 | `Source/WxCombat/Public/Damage/WxCombatEffectContext.h` |

## 확장 포인트 / 규약
- 새 어빌리티는 `UWxAbilityBase`(또는 `WxAbility_*` 파생) 상속. 쿨다운·코스트 수치는 코드가 아니라 `AbilityDataRow`(`FWxAbilityTableRow`)에서 읽으며, 쿨다운 GE가 없으면 "쿨다운 없음"으로 취급한다.
- 데미지는 `FWxDamageTableRow` 한 행으로 저작한다(계수·히트리액트 태그·가드/패리 허용·추가 GE). `UWxCombatLibrary::ApplyDamage`가 유일한 적용 경로.
- 상태·비용·연출은 `UWxEffect_*` GameplayEffect를 늘려 붙이고, 히트 연출은 `UWxCueNotify_*`, 타이밍은 `UWxAnimNotify*`로 몽타주에 심는다.
- 타겟팅 필터/소터는 `WxTargetingFilterTask_*`/`WxTargetingSorterTask_*`(TargetingSystem 태스크)로 확장한다.
- 캐릭터별 상태가 없는 서버 권위 스폰(투사체·소환물)은 캐릭터 컴포넌트가 아니라 월드 서브시스템이 맡는다. 몽타주 노티파이가 클래스·위치를 풀어 직접 호출하고, 권위 판정은 서브시스템이 한다. 소환물 명령(`UWxAnimNotify_CommandMinion`)도 같은 방식이다.
- 리플리케이션: 어트리뷰트는 서버 권위 복제, 락온 대상은 서버 권위+소유 클라 선반영, 데미지 판정은 어트리뷰트를 보지 않아 예측·임팩트가 같은 결론을 쓴다.
- `UWxAbilitySystemGlobals`를 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`에 등록해야 `FWxCombatEffectContext`가 만들어진다(누락 시 ExecCalc가 ensure).

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 발동 그룹·캔슬 창·테이블 규약이 전투 흐름의 뼈대
2. `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 어떤 자원·수치가 있고 데미지가 어디로 흐르는지
3. `Source/WxCombat/Public/WxCombatLibrary.h` + `Source/WxCombat/Public/Damage/WxDamageTableRow.h` — 히트→데미지 적용의 데이터·제어 흐름

## 관련
- 상위: [[WxAI]](락온·타겟 주입), [[WxUI]](데미지 플로터·`IWxUIData` 표현), Experience/캐릭터 BP가 `UWxAbilitySet`으로 결합
- 기반: [[WxCore]]

---
*문서 기준 커밋 `a1df17d` · 생성일 2026-09-04 · 소스 169파일 — `/readme-writer`로 갱신*
