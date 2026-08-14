# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반 액션 RPG 전투의 코어. 어빌리티·어트리뷰트·대미지 판정·락온·무기/투사체·히트스톱을 한 모듈에서 책임진다.

## 책임
**담당**
- 어빌리티 파이프라인: 입력→어빌리티 라우팅, 부여(AbilitySet), 공용 베이스(`UWxAbilityBase`)와 공격/가드/닷지/스킬/궁극/AI 패턴 등 구체 어빌리티.
- 전투 어트리뷰트: HP/SP/DP/MP/UP·ATK/DEF·크리·이동/공격속도(`UWxCombatAttributeSet`)와 이를 조작하는 GameplayEffect·MMC 모음.
- 대미지 판정: `UWxExecCalc_Damage`가 무적/가드/퍼펙트가드/일반 피격을 가르고, `FWxCombatEffectContext`로 결과를 실어 Cue·반응 이벤트를 발행.
- 타게팅/락온(`TargetingSystem` 필터 태스크 + `UWxLockOnManagerComponent`), 무기 히트 판정(`AWxWeaponBase`)·투사체(`AWxProjectileBase`), 히트스톱과 전역 TimeDilation.
- AnimNotify로 몽타주 타임라인에서 콤보 윈도우·무기 판정·투사체 스폰·무적/퍼펙트가드 구간을 구동.

**경계 (비담당)**
- 캐릭터 클래스·플레이어 입력 컴포넌트 자체는 [[WxGame]]/도메인 밖 소유물. 이 모듈은 ASC·컴포넌트를 얹을 뿐.
- HP바·데미지 플로터 등 실제 위젯 구현은 [[WxUI]]. 여기선 Cue/이벤트만 발행.
- 어빌리티/대미지 밸런스 수치와 아이콘은 DataTable·DataAsset(콘텐츠)로 위임. C++는 Row 스키마만 정의.

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존). 엔진 서브시스템은 `GameplayAbilities`, `TargetingSystem`, `MotionWarping`, `ModularGameplay`, `EnhancedInput`, `AIModule`, `Niagara`, `LevelSequence`.
- 규칙: `WxCore` 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 입력→어빌리티 라우팅의 유일 진입점. 히트스톱과 GE 적용 후 Cue/반응 이벤트 발행 훅을 소유 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티의 Abstract 베이스. Row 기반 쿨다운/코스트·몽타주·후딜(StartRecovery) 규약 정의 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 캐릭터가 지정하는 부여 데이터애셋. 어트리뷰트 초기화 Row·어빌리티·효과를 서버에서 일괄 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | 전투 어트리뷰트 원장. IncomingDamage(Meta)→HP 이관과 사망/가드브레이크/그로기 트리거 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxExecCalc_Damage` | 대미지 판정 엔진. `CheckDamage` 선판정은 클라 예측·화상·히트스톱이 공유 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터. Row→Spec 변환 진입점, 무기·투사체가 운반 | `Plugins/WxCombat/Source/WxCombat/Public/Damage/WxDamageInfo.h` |
| `FWxCombatEffectContext` | ExecCalc와 ASC 발행 훅 사이의 유일한 결과 통로 | `Plugins/WxCombat/Source/WxCombat/Public/Damage/WxCombatEffectContext.h` |
| `UWxLockOnManagerComponent` | SceneComponent 단위 락온 대상을 서버 권위로 복제. 방향/스냅 소비처가 참조 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase`(또는 목적별 파생 `WxAbility_*`)를 상속하고 `ActivationInputAction`/`ActivationPolicy`를 지정, 밸런스는 `WxAbilityTableRow` Row로 채운다. 부여는 코드가 아닌 `UWxAbilitySet` 애셋에 등록.
- 데이터 주도: 어빌리티 수치=`FWxAbilityTableRow`, 대미지=`FWxDamageTableRow`(→`FWxDamageInfo`), 어트리뷰트 초기값=`FWxCombatAttributeInitTableRow`. 쿨다운/코스트 GE는 공용 마커 GE + MMC가 Row를 조회하는 구조라 GE를 교체하면 엔진 순정 경로로 전환된다.
- 대미지 결과 파이프라인은 `UWxAbilitySystemGlobals`가 `FWxCombatEffectContext`를 할당해야 성립한다 — `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`에 등록 필수(누락 시 ExecCalc가 ensure).
- 리플리케이션/권한: 스폰·대미지 Spec·락온·TimeDilation·ActivationOwnedEffect 제거는 모두 서버 권위. 클라 예측 경로는 ExecCalc를 건너뛰므로 `DamageResult=None`으로 남고 연출은 스킵, 대신 `CheckDamage` 선판정으로 히트스톱만 로컬 일치시킨다.
- 태그·Cue·반응 이벤트는 GE 적용 완료 후(어트리뷰트 확정 뒤) `HandleGameplayEffectAppliedToSelf`에서 발행한다. 새 연출은 이 훅에 얹는다.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력·히트스톱·연출 발행이 모이는 허브. 제어 흐름의 시작점.
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h` — 대미지 판정 규칙(무적/가드/퍼펙트/Unblockable)과 클라 예측 통로.
3. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 모든 어빌리티가 따르는 쿨다운/코스트/후딜 규약.
4. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 약어(HP/SP/DP/MP/UP…)와 상태 트리거의 사전.

## 관련
- 상위: 캐릭터·Experience 주입 설정으로 이 모듈을 조립하는 [[WxGame]]. 연출 소비처 [[WxUI]], AI 패턴 어빌리티를 구동하는 [[WxAI]].

---
*문서 기준 커밋 `6f60b14` · 생성일 2026-08-14 · 소스 153파일 — `/readme-writer`로 갱신*
