# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 올린 액션 RPG 전투의 도메인 플러그인. 어빌리티·이펙트·어트리뷰트로 공격/가드/회피/락온/대미지 판정을 구성하고, 무기·투사체·히트스톱·시간 지연 연출까지 담는다.

## 책임
**담당**
- GAS 통합: 커스텀 ASC(`UWxAbilitySystemComponent`), AbilitySet 부여 흐름, 어트리뷰트 세트, GE/ExecCalc/MMC/Cue 계층
- 어빌리티 계층: 공격·스킬·궁극기·회피·가드·피니셔·히트리액트·그로기·락온·질주·AI 패턴 등 `UWxAbilityBase` 파생
- 대미지 파이프라인: `UWxCombatLibrary::ApplyDamage` 단일 진입점 → Damage GE + `UWxExecCalc_Damage` 판정 → `FWxCombatEffectContext`로 결과 전달 → Cue/이벤트 발행
- 무기·투사체 액터, AnimNotify(공격 구간·무적·퍼펙트가드·투사체 스폰 등), 락온 타게팅, 히트스톱·글로벌 시간 지연

**경계 (비담당)**
- 공용 GameplayTag·콜리전 채널 정의 → [[WxCore]] (`WxGameplayTags.h`, `WxCollisionChannels.h`)
- 캐릭터/폰·플레이어 입력 바인딩 주체·Experience 조립 → 게임 모듈(`WxGame`)·상위 콘텐츠
- AI 의사결정(패턴을 언제 발동할지) → [[WxAI]] (본 모듈은 발동 가능한 패턴 어빌리티만 제공)

## 의존성
- **주요 의존**: [[WxCore]] (유일한 Wx 의존). 엔진: GameplayAbilities, ModularGameplay, EnhancedInput, TargetingSystem, MotionWarping, Niagara(Private)
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 커스텀 ASC. 입력→어빌리티 라우팅의 유일한 진입점, 대미지 적용 후 Cue/이벤트 발행, 히트스톱 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilitySet` | 어빌리티·이펙트·어트리뷰트 초기화 행을 ASC에 일괄 부여하는 PrimaryDataAsset | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxAbilityBase` | 전 어빌리티의 베이스. Row 기반 쿨다운·코스트, 입력 액션·활성화 정책, 캔슬 규약 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | HP/SP/DP/MP/UP·ATK/DEF·Crit·SPD/ASPD 스탯 어트리뷰트 세트 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxExecCalc_Damage` | 대미지 계산 ExecCalc. 무적/가드/퍼펙트가드/일반 분기, 판정을 EffectContext에 기록 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |
| `UWxCombatLibrary` | `ApplyDamage` — 무기·피니셔가 공유하는 대미지 적용 단일 진입점(예측 포함) | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `FWxDamageInfo` / `FWxCombatEffectContext` | 대미지 설계 데이터(Row→Spec) / ExecCalc↔ASC 판정 결과 전달 통로 | `Plugins/WxCombat/Source/WxCombat/Public/Damage/` |
| `AWxWeaponBase` / `AWxProjectileBase` | 무기 히트 콜리전(ANS 구간 refcount) / 서버 권위 투사체 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent 단위)을 서버 권위로 복제·관리 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase` 파생 → `ActivationInputAction`(입력형만), `AbilityDataRow`(쿨다운·코스트), `ActivationPolicy`(OnTriggered/OnGranted) 지정. 쿨다운/코스트 GE 기본값은 공용 GE를 가리키는 "마커"이며 그대로 두면 Row 기반, 다른 GE로 교체하면 엔진 순정 경로.
- **부여**: 캐릭터 BP가 `UWxAbilitySet` 에셋 지정 → `InitAbilitySystem` 시점 서버에서 일괄 Grant. 입력 바인딩 목록은 각 어빌리티 CDO의 `ActivationInputAction`을 AbilitySet이 모아 만든다.
- **캔슬 규약**: 후딜레이 구간 = 캔슬 가능 구간. 어빌리티가 건 `BlockAbilitiesWithTag`를 후딜에 풀어 다른 어빌리티가 인터럽트 진입.
- **대미지 확장**: 무기·투사체는 `WxDamageTableRow`로 `FWxDamageInfo` 저작 → `MakeSpecs`가 SetByCaller/DynamicAssetTags로 변환. 부수효과는 DamageInfo의 `AdditionalEffects`로 붙인다.
- **필수 등록**: `UWxAbilitySystemGlobals`를 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`에 등록해야 `FWxCombatEffectContext`가 할당된다(누락 시 `UWxExecCalc_Damage`가 ensure).
- **GameplayTag/콜리전 채널**은 이 모듈에서 선언하지 않고 [[WxCore]]의 `WxGameplayTags.h`·`WxCollisionChannels.h`를 사용한다.
- **리플리케이션**: 어트리뷰트·락온·시간 지연은 서버 권위 복제. 대미지 GE는 Instant+Execution이라 예측 시 execution은 건너뛰고 지속형 AdditionalEffects만 예측된다.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력 라우팅·대미지 후처리·히트스톱, 모듈의 허브
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티 공통 규약(Row·입력·캔슬)을 잡고 파생 클래스로 내려간다
3. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` + `Public/Damage/WxCombatEffectContext.h` — 대미지 판정과 연출 발행이 분리된 이유·경로

## 관련
- 상위: [[WxCore]] (공용 태그·채널 foundation), 게임 모듈 `WxGame`이 Experience로 조립

---
*문서 기준 커밋 `dfd2174` · 생성일 2026-08-12 · 소스 153파일 — `/readme-writer`로 갱신*
