# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 구축한 액션 RPG 전투의 핵심 도메인. 어빌리티·어트리뷰트·이펙트, 대미지 파이프라인, 무기/투사체, 락온, 타임 슬로우까지 실제 전투 로직 전반을 담당한다.

## 책임
**담당**
- GAS 런타임: ASC, AttributeSet, AbilitySet 부여, 어빌리티(공격/회피/가드/스킬/궁극기/피격/사망/AI 패턴 등)와 각종 GameplayEffect/Cue/ExecCalc/MMC
- 대미지 파이프라인: `FWxDamageInfo` 설계 데이터 → GE Spec 변환 → ExecCalc로 최종 대미지 산출 → AttributeSet 반영
- 무기·투사체 히트 판정(`AWxWeaponBase`, `AWxProjectileBase`, `AWxEffectZone`)과 AnimNotify 기반 공격 구간/콤보 윈도우/무적/이벤트 발송
- 락온 타게팅(`TargetingSystem` 필터 태스크 + `UWxLockOnManagerComponent`)과 MotionWarping 스냅
- 히트스톱/슬로우모션 등 전투 연출용 시간 조작(`UWxTimeDilationComponent`)

**경계 (비담당)**
- Gameplay Tag의 C++ 네이티브 정의는 [[WxCore]]의 `WxGameplayTags.h`에 있고 여기서는 `WxGameplayTags::` 네임스페이스로 소비만 한다 (이 모듈은 태그를 선언하지 않음)
- UI 표시(어빌리티 아이콘 등)는 [[WxUI]]의 `UWxAbilityComponent` 파생으로 위임 — `UWxAbilityBase::Components`에 EditInline으로 부착
- 캐릭터 클래스·팀/컨트롤러 등 액터 골격과 입력 바인딩은 게임 모듈([[WxGame]])에서 소유. 이 모듈은 입력 태그(`FGameplayTag`)만 받아 어빌리티로 라우팅

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존), `GameplayAbilities`, `GameplayTags`, `GameplayTasks`, `ModularGameplay`, `TargetingSystem`, `MotionWarping`, `EnhancedInput`, `AIModule`, `NavigationSystem`, `UMG`, `NetCore` / (Private) `Niagara`(연출), `LevelSequence`·`MovieScene`(스킬 컷씬), `InputCore`
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (Build.cs·uplugin·include 전수 확인. 유일한 Wx 크로스모듈 include는 `WxCore`의 `WxGameplayTags.h`)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 프로젝트 ASC. 입력 태그 → 어빌리티 라우팅, AbilitySet 부여 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 쿨다운/코스트를 DataRow 기반 공용 GE로 처리, 히트스톱·후딜캔슬 훅 제공 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | Ability/Effect/Attribute 초기값을 한 DataAsset으로 묶어 ASC에 일괄 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | HP/SP/DP/MP/UP·ATK/DEF/Crit·SPD/ASPD + IncomingDamage 메타. 대미지→HP 차감의 종점 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터 → GE Spec 배열(SetByCaller/AssetTags) 변환의 중심 | `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxCombatLibrary` | 무기/투사체 외 경로의 대미지 적용 BP 진입점(`ApplyDamage`/`ApplyRawDamage`) | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `AWxWeaponBase` | 무기 액터. ANS_WeaponAttack이 여는 Overlap/Sweep 스윙 히트 구간 관리 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxLockOnManagerComponent` | 복제되는 락온 대상(SceneComponent 단위)을 서버 권위+클라 예측으로 관리 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase`(Abstract/Blueprintable) 상속(예: `WxAbility_Attack/Skill/Dodge/Guard/Ultimate/HitReact/Death/Pattern`…). 활성화 정책은 `EWxAbilityActivationPolicy`(OnInputTriggered / OnGranted). 수치는 코드가 아닌 `AbilityDataRow`(`FWxAbilityTableRow`)로 저작.
- **쿨다운/코스트 규약**: 기본값은 공용 `UWxEffect_Cooldown`/`UWxEffect_Cost`를 가리키는 "마커". 마커 그대로면 DataRow 기반(충전=쿨다운 GE 스택), 다른 GE로 바꾸면 엔진 순정 경로(상호배타). 디테일 패널에서 어느 쪽인지 바로 읽힌다.
- **데이터 주도**: `UWxAbilitySet`이 어빌리티+`FWxAbilitySet_GameplayAbility`(InputTag 매핑)+`FWxCombatAttributeInitTableRow` 초기값을 묶어 부여. 대미지 수치는 `FWxDamageTableRow`, 어빌리티 수치는 `FWxAbilityTableRow`로 테이블화. 새 상태이상은 `FWxDamageInfo.AdditionalEffects`에 GE 추가.
- **대미지 산출**: `UWxExecCalc_Damage`(ATK/DEF/Crit/가드/히트스톱)·`UWxExecCalc_Burn`(도트) + `UWxMMC_LinearDrain`. 최종값은 AttributeSet의 `IncomingDamage`(Meta) → `PostGameplayEffectExecute`에서 HP 차감.
- **리플리케이션**: AttributeSet 대부분 OnRep 복제, IncomingDamage는 비복제 Meta. 락온 대상·무기/투사체 히트는 서버 권위, 소유 클라는 예측 후 정합. 입력 태그도 서버 RPC로 동기화.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` — 캐릭터가 무엇을(어빌리티/이펙트/어트리뷰트) 어떻게 부여받는지, 입력 라우팅의 시작점
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티 공통 규약(수명주기·쿨다운·코스트·히트스톱)이 헤더 주석에 정리돼 있어, 개별 `WxAbility_*`는 파생 차이만 보면 된다
3. `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` + `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp` — AnimNotify에서 시작된 히트가 최종 HP 차감까지 흐르는 대미지 파이프라인

## 관련
- 상위: 캐릭터/플레이어 액터가 ASC·AbilitySet을 장착해 사용하는 [[WxGame]], 어빌리티 UI 표시를 붙이는 [[WxUI]], 어빌리티를 구동하는 [[WxAI]]. 공용 태그/정의는 [[WxCore]] 참조.

---
*문서 기준 커밋 `12b1bba` · 생성일 2026-07-18 · 소스 153파일 — `/readme-writer`로 갱신*
