# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 올린 액션 RPG 전투 도메인. 어트리뷰트(HP/SP/DP/MP/UP)와 어빌리티, 대미지 판정, 락온, 무기·투사체, 히트스톱·슬로모까지 전투 한 판을 굴리는 요소를 모아 둔다.

## 책임
**담당**
- 전투용 ASC(`UWxAbilitySystemComponent`)와 어트리뷰트 세트, 입력→어빌리티 라우팅, 배타 점유(ActivationGroup) 판정
- GAS 어빌리티 계층(공격·스킬·궁극기·회피·질주·가드·피격·그로기·사망·락온·패턴)의 C++ 베이스
- 데이터 주도 대미지 파이프라인: `ApplyDamage` 진입점 → `UWxExecCalc_Damage` 판정(가드/퍼펙트가드/무적/크리) → 어트리뷰트 반영
- GameplayEffect 모음(`WxEffect_*`), GameplayCue(`WxCueNotify_*`), 전투 AnimNotify/AnimNotifyState, AbilityTask
- 락온 타게팅(`TargetingSystem` 필터 태스크 + 매니저 컴포넌트), MotionWarping 스냅, 무기·투사체 액터, 전역 시간 배율(슬로모)

**경계 (비담당)**
- 어트리뷰트 초기값·밸런스 수치의 실제 데이터는 DataTable/DataAsset 에셋에 있고, 코드는 Row 스키마만 정의한다
- 캐릭터 클래스·입력 매핑·Experience 주입 등 소유 액터 조립은 이 모듈 밖(게임 모듈/`WxCore` Experience)에서 한다
- 공용 정의·유틸은 [[WxCore]]에 의존한다

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 전투 ASC — 입력 라우팅·어빌리티 부여·히트스톱·배타 점유의 중심 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxCombatAttributeSet` | HP/SP/DP/MP/UP·ATK/DEF 등 전 어트리뷰트와 대미지 수신 후처리 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxAbilityBase` | 모든 Wx 어빌리티의 베이스 — ActivationPolicy/ActivationGroup, 데이터 주도 쿨다운·코스트 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 어트리뷰트·어빌리티·이펙트를 ASC에 일괄 부여하는 DataAsset | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatLibrary::ApplyDamage` | 무기·피니셔·투사체가 공유하는 대미지 단일 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `UWxExecCalc_Damage` | 가드/퍼펙트가드/무적/크리를 가르는 대미지 계산(예측·서버 공용 선판정 포함) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h` |
| `FWxDamageTableRow` | 공격별 밸런스(계수·HitReact·추가효과) DataTable Row | `Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageTableRow.h` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent 단위)을 서버 권위로 복제·브로드캐스트 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase` 파생 → `AbilityDataRow`(쿨다운·코스트·아이콘)와 `ActivationInputAction`/`ActivationGroup` 지정 → `UWxAbilitySet`의 `GrantedAbilities`에 등록. 입력 라우팅 키는 각 어빌리티 CDO의 `ActivationInputAction`이 쥔다.
- **새 GameplayEffect**: `WxEffect_*` 관례를 따른다(대미지·화상·가드·쿨다운·코스트·자원 회복 등). 대미지 계열은 `UWxExecCalc_Damage`가 결과를 `FWxCombatEffectContext`에 싣는다.
- **대미지 흐름**: `ApplyDamage` → `FWxDamageTableRow::MakeSpecs`가 하나의 `FWxCombatEffectContext`를 공유하는 Spec들을 만듦 → Instant+Execution 대미지 GE가 출력 모디파이어/컨텍스트에만 결과를 적고 → `UWxCombatAttributeSet::PostGameplayEffectExecute`가 HP/DP/SP 확정과 Cue/플로터를 담당(파일 횡단, 순서 의존).
- **EffectContext 등록 필수**: `UWxAbilitySystemGlobals`를 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`으로 등록해야 크리 여부가 실린다(누락 시 ensure).
- **몽타주 구동 전투**: `WxAnimNotify(State)_*`로 무기 판정 구간·콤보 윈도우·무적·스냅·후딜 전이·투사체 스폰·이벤트 송출을 몽타주에서 데이터로 건다.
- **락온 필터**: `WxTargetingFilterTask_*`(LineTrace/GameplayTag/InputDirection/ScreenBounds/Team)를 `TargetingSystem` 프리셋에 조합해 후보를 거른다.
- **리플리케이션**: 서버 권위 GAS. 락온·시간배율은 서버에서 설정하고 전 머신에 복제하며, 소유 클라는 응답성 위해 로컬 선반영 후 복제값으로 정합한다.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력이 어떻게 어빌리티로 라우팅되고 배타 점유가 어떻게 갈리는지, 전투의 제어 중심
2. `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` + `Private/AbilitySystem/Effect/WxEffect_Damage.cpp` — 대미지 한 방이 진입점에서 판정을 거쳐 어트리뷰트에 닿기까지의 파일 횡단 흐름
3. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — HP/SP/DP/MP/UP의 의미와 가드·그로기·사망 트리거 지점
4. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티 공통 계약(ActivationPolicy/Group, 데이터 주도 쿨다운·코스트)

## 관련
- 상위: 캐릭터·플레이어/AI 컨트롤러가 이 ASC와 어빌리티 세트를 장착해 사용한다. AI 패턴은 [[WxAI]], 무기/외형·전투 UI(HP바·플로터·락온 마커)는 [[WxUI]]와 연동
- 기반: [[WxCore]] (공용 정의·Experience 주입)

---
*문서 기준 커밋 `807a9da` · 생성일 2026-08-22 · 소스 152파일 — `/readme-writer`로 갱신*
