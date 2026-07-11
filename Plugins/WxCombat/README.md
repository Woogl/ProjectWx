# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반 액션 RPG 전투의 코어. 어빌리티·이펙트·어트리뷰트·대미지 파이프라인부터 락온, 히트 판정, 시간 감속까지 캐릭터가 "싸우는" 데 필요한 런타임을 담당한다.

## 책임
**담당**
- GAS 골격: 커스텀 ASC(`UWxAbilitySystemComponent`), 스탯 `UWxCombatAttributeSet`, AbilitySet 부여 파이프라인
- 어빌리티 로스터: 공격/콤보·스킬·궁극기·회피·가드·피격·그로기·처형·질주·락온, AI 패턴 어빌리티(`WxAbility_Pattern*`)
- 대미지 파이프라인: `FWxDamageInfo` 설계 데이터 → GE Spec → `UWxExecCalc_Damage` 판정(크리/가드/퍼펙트가드/무적/반사)
- GameplayEffect·GameplayCue·MMC/ExecCalc 라이브러리 (버프·번·쿨다운·코스트·자원 회복 등)
- 히트 소스: 무기(`AWxWeaponBase`)·투사체(`AWxProjectileBase`)·이펙트 존(`AWxEffectZone`)의 콜리전/오버랩 히트 처리
- 전투용 AnimNotify(State): 무기 공격 창, 콤보 윈도우, 무적, 퍼펙트가드, 타겟 스냅, 투사체 스폰 등
- 락온/타게팅: `UWxLockOnManagerComponent`와 TargetingSystem 필터 태스크들
- 서버 권위 시간 감속(`UWxTimeDilationComponent`)

**경계 (비담당)**
- 캐릭터 클래스·입력 바인딩·무브먼트 본체는 [[WxGame]] 등 소비 측 (WxCombat은 컴포넌트/어빌리티만 제공)
- 어빌리티 UI 표시 데이터·아이콘은 [[WxUI]]의 `UWxAbilityComponent` 파생으로 위임 (여기선 베이스 훅만)
- 공용 정의(팀 인터페이스, 공통 타입 등)는 [[WxCore]]

## 의존성
- **주요 의존**: [[WxCore]] · 엔진: `GameplayAbilities`, `GameplayTags`, `GameplayTasks`, `ModularGameplay`, `TargetingSystem`, `EnhancedInput`, `MotionWarping`(Private), `Niagara`(Private)
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (`.uplugin`·`Build.cs` 모두 Wx 의존은 `WxCore` 단독)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 커스텀 ASC. 입력 태그→어빌리티 라우팅, AbilitySet 부여, 래그돌 복제 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 전 어빌리티의 베이스. 공용 쿨다운/코스트/데이터테이블·후딜(캔슬) 규약 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | Ability/Effect/Attribute 초기 데이터를 한 에셋으로 묶어 ASC에 일괄 부여 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | HP/SP/DP/MP/UP·ATK/DEF/Crit·SPD/ASPD·IncomingDamage 스탯 정의 | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터 → GE Spec 변환의 시작점 | `Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxExecCalc_Damage` | 최종 대미지 판정 심장부(크리/가드/퍼펙트가드/무적/반사/큐) | `Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |
| `UWxCombatLibrary` | BP용 진입점: `ApplyDamage`/`ApplyRawDamage`/`IsHostile` | `Source/WxCombat/Public/WxCombatLibrary.h` |
| `AWxWeaponBase` | 무기 히트 소스(스윙 콜리전·1히트/액터·HitStop) | `Source/WxCombat/Public/Weapon/WxWeaponBase.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase` 상속. 쿨다운은 `CooldownTime`/`MaxRecharges`, 코스트는 `MPCost`/`UPCost` 프로퍼티로 선언(공용 `UWxEffect_Cooldown`/`UWxEffect_Cost` GE를 소스 CDO로 구분). 발동 시 자기 버프는 `OnActivateEffects`. 활성화 정책은 `EWxAbilityActivationPolicy`(입력 트리거 / 부여 즉시).
- **데이터 주도**: 어빌리티 수치는 `AbilityDataRow`(`FWxAbilityTableRow`), 대미지는 `FWxDamageTableRow`, 어트리뷰트 초기값은 `FWxCombatAttributeInitTableRow`를 `UWxAbilitySet`의 `AttributeInitRow`로 지정. 전부 DataTable Row 핸들 기반.
- **새 이펙트/큐/계산**: `Effect/`(GE·MMC·ExecCalc), `Cue/`(GameplayCueNotify) 디렉터리에 파생 추가. 대미지 판정 로직 변경은 `UWxExecCalc_Damage`.
- **후딜=캔슬 구간 규약**: 어빌리티는 `StartRecovery()`(또는 `ANS_StartRecovery`)로 후딜에 진입하며 이때 자신의 하드 차단을 풀어 캔슬 진입을 허용한다.
- **리플리케이션/권한**: 대미지·상태 변화는 서버 권위(GAS 표준). 락온 대상, 래그돌, 글로벌 타임딜레이션은 서버 권위로 복제되며 소유 클라는 응답성 위해 로컬 예측 후 서버 정합.

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 모든 어빌리티가 공유하는 쿨다운/코스트/후딜 규약. 여기를 알면 개별 어빌리티가 읽힌다.
2. `Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` — 헤더 주석의 6단계 판정 흐름이 전투 규칙의 요약본. 대미지가 어떻게 결정되는지의 단일 진실.
3. `Source/WxCombat/Public/WxDamageInfo.h` + `Source/WxCombat/Public/Weapon/WxWeaponBase.h` — AnimNotify에서 편집된 `FWxDamageInfo`가 무기 스윙을 거쳐 GE Spec으로 흐르는 히트 경로.
4. `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` — 캐릭터가 부팅 시 무엇을 어떻게 부여받는지(어빌리티/이펙트/어트리뷰트).

## 관련
- 상위: 캐릭터에 ASC/컴포넌트를 붙이고 AbilitySet을 지정해 소비하는 [[WxGame]], AI 패턴 어빌리티를 구동하는 [[WxAI]]
- 함께: 어빌리티 표시 데이터를 확장하는 [[WxUI]], 공용 정의 [[WxCore]]

---
*문서 기준 커밋 `83a7315` · 생성일 2026-07-11 · 소스 151파일 — `/readme-writer`로 갱신*
