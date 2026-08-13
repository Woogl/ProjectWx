# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 구축한 액션 RPG 전투 도메인. 어빌리티·이펙트·어트리뷰트로 공격/방어/피격을 처리하고, 무기·투사체 히트 판정, 락온, 히트스톱, 시간 감속을 담당한다.

## 책임
**담당**
- GAS 확장: 커스텀 ASC(`UWxAbilitySystemComponent`), 어트리뷰트 세트, 어빌리티 베이스, AbilitySet 일괄 부여
- 전투 어빌리티군: 공격/스킬/궁극기/회피/가드/피니셔/그로기/피격반응/락온/질주 및 AI 패턴
- 대미지 파이프라인: `FWxDamageInfo` 설계 데이터 → Damage GE Spec → `UWxExecCalc_Damage` 판정 → 커스텀 EffectContext에 결과 적재 → ASC가 Cue·이벤트 발행
- 자원·상태 모델: HP/SP/DP/MP/UP 등 어트리뷰트, 코스트/쿨다운/드레인/번(지속딜) 등 GameplayEffect·MMC·ExecCalc
- 무기/투사체 히트 판정(`AWxWeaponBase`·`AWxProjectileBase`), AnimNotify 기반 공격/콤보 윈도우/무적/스냅 구간
- 락온 타게팅(`TargetingSystem` 필터 태스크 + `UWxLockOnManagerComponent`), 히트스톱·시간 감속 연출

**경계 (비담당)**
- 캐릭터 클래스·이동·입력 매핑 컨텍스트 소유는 게임 모듈([[WxGame]])에 위임 — 이 모듈은 ASC/컴포넌트/어빌리티만 제공
- 공용 태그·팀·상호작용 등 foundation 정의는 [[WxCore]]가 가진다(이 모듈은 Native Tag를 선언하지 않음)
- Cue/이펙트가 참조하는 실제 VFX·몽타주·DataTable 값 등 에셋 저작은 BP/GameFeature 콘텐츠 범위

## 의존성
- **주요 의존**: [[WxCore]] · GameplayAbilities(GAS) · ModularGameplay · EnhancedInput · TargetingSystem · MotionWarping · AIModule · Niagara(비공개) · LevelSequence/MovieScene(스킬 컷신, 비공개)
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (`.uplugin`·`Build.cs` 모두 Wx 중 `WxCore`만 참조)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 전투의 중앙 허브. 입력→어빌리티 라우팅, 히트스톱, GE 적용 훅에서 Cue·반응 이벤트 발행 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 전투 어빌리티의 Abstract 베이스. Row 기반 쿨다운(다중 충전)·코스트·입력 액션·후딜 캔슬 규약 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 캐릭터 BP가 지정하는 부여 묶음. 서버에서 어빌리티+이펙트+어트리뷰트 init을 ASC에 일괄 부여 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | 캐릭터 스탯 어트리뷰트(HP/SP/DP/MP/UP/ATK/DEF/Crit/SPD/ASPD + IncomingDamage 메타) | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터. 무기·투사체가 들고 다니며 Damage GE Spec으로 변환 | `Source/WxCombat/Public/Damage/WxDamageInfo.h` |
| `UWxExecCalc_Damage` | 대미지 최종 판정(무적/가드/퍼펙트가드/치명타)을 계산해 EffectContext에 결과 적재 | `Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |
| `UWxCombatLibrary` | `ApplyDamage` — 무기 스윙·피니셔가 공유하는 단일 대미지 적용 진입점(예측/히트스톱 포함) | `Source/WxCombat/Public/WxCombatLibrary.h` |
| `AWxWeaponBase` | AnimNotify가 여닫는 무기 히트 콜리전. 한 스윙당 액터 1회 피격, 히트 시 `ApplyDamage` 호출 | `Source/WxCombat/Public/Weapon/WxWeaponBase.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase`를 상속(C++) 또는 BP 파생. `ActivationInputAction`으로 입력 라우팅 키를 잡고, `AbilityDataRow`(`FWxAbilityTableRow`)로 쿨다운·충전·코스트·아이콘을 데이터 주도로 설정. 입력 미발동 어빌리티(AI 패턴·반응형·패시브)는 InputAction을 비우고 `ActivationPolicy`로 자동 발동 여부 결정.
- **쿨다운/코스트 모델**: 기본값은 공용 GE를 가리키는 "마커"라 Row에서 수치를 읽는다(`UWxMMC_*`가 Row 조회). 다른 GE로 바꾸면 엔진 순정 경로로 전환(Row와 상호배타). 다중 충전은 소모 충전 1개당 쿨다운 GE 1개.
- **부여 흐름**: 캐릭터가 `UWxAbilitySet` 에셋을 지정 → `InitAbilitySystem` 시점 서버에서 `GiveToAbilitySystem`이 어빌리티/이펙트/어트리뷰트 init Row를 일괄 부여.
- **대미지 흐름**: `FWxDamageInfo::MakeSpecs`가 Damage GE + AdditionalEffects Spec 생성 → `UWxExecCalc_Damage` 판정 → 결과를 `FWxCombatEffectContext`(커스텀, `UWxAbilitySystemGlobals`가 할당)에 적재 → ASC의 적용 훅이 읽어 Cue·HitReact 이벤트 발행. 클라 예측 경로는 ExecCalc를 건너뛰어 결과가 `None`으로 남고 연출을 스킵.
- **리플리케이션**: Damage GE는 Instant+Execution이라 서버 권위로 확정되고 지속형 AdditionalEffects만 예측된다. 락온 대상은 서버 권위로 전 머신 복제(SceneComponent 단위).

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력→어빌리티→대미지→연출로 이어지는 제어 흐름의 허브. 헤더 주석이 라우팅·히트스톱·Cue 발행을 설명한다.
2. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 모든 어빌리티가 공유하는 쿨다운·코스트·캔슬 규약. 개별 `WxAbility_*`는 이 위에서 읽는다.
3. `Source/WxCombat/Public/Damage/WxDamageInfo.h` + `Damage/WxCombatEffectContext.h` — 대미지가 데이터에서 판정 결과·연출까지 흐르는 두 끝. ExecCalc(`AbilitySystem/Effect/WxExecCalc_Damage.h`)가 그 사이를 잇는다.
4. `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` — 이 모든 게 캐릭터에 어떻게 부여되는지의 진입점.

## 관련
- 상위: 캐릭터·입력을 소유하고 이 ASC/AbilitySet을 장착하는 [[WxGame]]; foundation 정의(태그·팀 등)를 제공하는 [[WxCore]]
- 콘텐츠: 전투 어빌리티/이펙트 에셋을 켜는 GameFeature 플러그인(`Plugins/GameFeatures/`)이 Experience로 활성화

---
*문서 기준 커밋 `1ae8d2f` · 생성일 2026-08-13 · 소스 155파일 — `/readme-writer`로 갱신*
