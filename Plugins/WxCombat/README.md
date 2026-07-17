# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 구축한 액션 RPG 전투의 핵심 도메인. 어빌리티·어트리뷰트·이펙트, 대미지 파이프라인, 무기/투사체, 락온, 타임 슬로우까지 실제 전투 로직 전반을 담당한다.

## 책임
**담당**
- GAS 런타임: ASC, AttributeSet, AbilitySet 부여, 어빌리티(공격/회피/가드/스킬/궁극기/피격/사망/AI 패턴 등)와 각종 GameplayEffect/Cue/ExecCalc/MMC
- 대미지 파이프라인: `FWxDamageInfo` 설계 데이터 → GE Spec 변환 → ExecCalc로 최종 대미지 산출 → AttributeSet 반영
- 무기·투사체 히트 판정(`AWxWeaponBase`, `AWxProjectileBase`)과 AnimNotify 기반 공격 구간/콤보 윈도우/무적/이벤트 발송
- 락온 타게팅(`TargetingSystem` 필터 태스크 + `UWxLockOnManagerComponent`)과 MotionWarping 스냅
- 히트스톱/슬로우모션 등 전투 연출용 시간 조작(`UWxTimeDilationComponent`)

**경계 (비담당)**
- Gameplay Tag 정의는 [[WxCore]]의 `WxGameplayTags.h`에 있고 여기서는 소비만 한다
- UI 표시(어빌리티 아이콘 등)는 [[WxUI]]의 `UWxAbilityComponent` 파생으로 위임
- 캐릭터 클래스·팀/컨트롤러 등 액터 골격은 게임 모듈([[WxGame]])에서 소유

## 의존성
- **주요 의존**: `WxCore`, `GameplayAbilities`, `GameplayTags`, `GameplayTasks`, `ModularGameplay`, `TargetingSystem`, `MotionWarping`, `EnhancedInput`, `AIModule`, `Niagara`(연출), `LevelSequence`/`MovieScene`(스킬 컷신)
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (Wx 크로스모듈 include는 `WxCore`의 `WxGameplayTags.h`뿐)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 캐릭터 ASC. 입력 태그 → 어빌리티 라우팅, AbilitySet 부여 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스(쿨다운/코스트/데이터테이블 통합) | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | Ability/Effect/Attribute 초기 데이터를 묶어 ASC에 일괄 부여하는 DataAsset | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | HP/SP/DP/MP/UP/ATK/DEF 등 전투 스탯. IncomingDamage 처리 | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터 → GE Spec 배열 변환 | `Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxCombatLibrary` | 무기/투사체 외 경로의 대미지·적대 판정 BP 진입점 | `Source/WxCombat/Public/WxCombatLibrary.h` |
| `AWxWeaponBase` | 무기 액터. Overlap 기반 스윙 히트 판정 | `Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxLockOnManagerComponent` | 복제되는 락온 대상(SceneComponent 단위) 관리 | `Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- 새 어빌리티는 `UWxAbilityBase`(Abstract/Blueprintable) 상속. 쿨다운/코스트는 프로퍼티로 설정하며 공용 `UWxEffect_Cooldown`/`UWxEffect_Cost` GE를 재사용한다. 활성화 정책은 `EWxAbilityActivationPolicy`(OnInputTriggered / OnGranted).
- 데이터 주도: `UWxAbilitySet`이 어빌리티+`FWxAbilitySet_GameplayAbility`(InputTag 매핑)+`FWxCombatAttributeInitTableRow` 초기값을 한 에셋으로 부여. 대미지 수치는 `FWxDamageTableRow`, 어빌리티 수치는 `FWxAbilityTableRow`로 테이블화.
- 대미지 산출은 `UWxExecCalc_Damage`/`UWxExecCalc_Burn` + `UWxMMC_LinearDrain`가 담당하고, 최종값은 AttributeSet의 `IncomingDamage`(Meta) → `PostGameplayEffectExecute`에서 HP 차감.
- 리플리케이션: AttributeSet 대부분 복제, 락온 대상은 서버 권위+소유 클라 예측. 입력 태그도 서버 RPC로 동기화.

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` — 캐릭터가 무엇을(어빌리티/이펙트/어트리뷰트) 어떻게 부여받는지, 입력 라우팅의 시작점
2. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티 공통 규약(쿨다운·코스트·데이터테이블)을 이해하면 개별 `WxAbility_*`는 파생 차이만 보면 된다
3. `Source/WxCombat/Public/WxDamageInfo.h` + `Source/WxCombat/Private/AbilitySystem/Effect/WxExecCalc_Damage.cpp` — AnimNotify에서 시작된 히트가 최종 HP 차감까지 흐르는 대미지 파이프라인

## 관련
- 상위: 캐릭터/플레이어 액터를 소유한 [[WxGame]]가 ASC·AbilitySet을 장착해 사용. 태그는 [[WxCore]], 어빌리티 UI 표시는 [[WxUI]]와 함께 본다.

---
*문서 기준 커밋 `465b77a` · 생성일 2026-07-17 · 소스 153파일 — `/readme-writer`로 갱신*
