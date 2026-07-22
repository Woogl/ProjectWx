# WxCombat — GAS 기반 전투 시스템

> Gameplay Ability System(GAS) 위에 구축한 액션 RPG 전투 도메인. 어빌리티·어트리뷰트·대미지 파이프라인·락온·무기/투사체·히트 판정을 담당한다.

## 책임
**담당**
- GAS 통합: 커스텀 ASC(`UWxAbilitySystemComponent`), AttributeSet, AbilitySet(DataAsset) 기반 어빌리티/이펙트 일괄 부여
- 어빌리티 라이브러리: 공격·회피·가드·스킬·궁극기·락온·피격/사망/그로기·AI 패턴·피니셔 등 (`UWxAbilityBase` 파생)
- 대미지 파이프라인: `FWxDamageInfo` → `UWxEffect_Damage` Spec → `UWxExecCalc_Damage`(크리·가드·퍼펙트가드·반사·무적 판정) → AttributeSet HP/DP/SP 차감
- GameplayEffect / MMC / ExecCalc 저작(쿨다운·코스트·번·회복·드레인 등)과 GameplayCue 노티파이
- 전투 AnimNotify(무기 충돌 구간, 콤보 윈도우, 무적, 퍼펙트가드, 투사체 스폰, 광역 공격 등)
- 락온·타게팅: `UWxLockOnManagerComponent`, TargetingSystem 필터 태스크, MotionWarping SnapToTarget
- 무기/투사체 액터와 히트 콜리전 판정, 히트스톱(역경직), 시간 감속(`UWxTimeDilationComponent`)

**경계 (비담당)**
- Gameplay Tag 네이티브 선언(`WxGameplayTags`)은 [[WxCore]] 소유 — 여기선 소비만 한다
- 쿨다운/코스트/어트리뷰트 등의 UI 표시(ViewModel 바인딩)는 [[WxUI]]에 위임
- AI 의사결정(BT/BB)은 [[WxAI]] 몫 — WxCombat은 AI가 트리거하는 `UWxAbility_Pattern`만 제공

## 의존성
- **주요 의존**: [[WxCore]] · GameplayAbilities(GAS) · TargetingSystem · MotionWarping · EnhancedInput · ModularGameplay · Niagara/LevelSequence(연출)
- 규칙: WxCore 외 다른 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 프로젝트 ASC. AbilitySet 부여·입력 액션→어빌리티 라우팅의 허브 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilitySet` | 어빌리티·이펙트·어트리뷰트 초기값을 한 DataAsset으로 묶어 ASC에 일괄 부여 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 쿨다운/코스트를 `AbilityDataRow`에서 온디맨드 조회 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | HP/SP/DP/MP/UP·ATK/DEF/Crit/SPD/ASPD·IncomingDamage 스탯 정의 | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxExecCalc_Damage` | 대미지 계산·판정의 중심(크리/가드/퍼펙트가드/반사/무적) | `Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터. AnimNotify→무기/투사체→Damage Spec 변환 | `Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxCombatLibrary` | 무기 외 경로용 대미지 적용 BP 라이브러리(`ApplyDamage`/`ApplyRawDamage`) | `Source/WxCombat/Public/WxCombatLibrary.h` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent) 복제·관리. 부위 단위 추적 확장 | `Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase` 상속(BP 가능). 입력 발동이면 `ActivationInputAction` 지정, AI/반응형/패시브는 비움. `ActivationPolicy`로 OnTriggered/OnGranted 선택.
- **데이터 주도 수치**: 쿨다운·충전·코스트·아이콘은 `AbilityDataRow`(`FWxAbilityTableRow`), 어트리뷰트 초기값은 `WxCombatAttributeInitTableRow`, 대미지 프리셋은 `WxDamageTableRow` 데이터테이블에서 읽는다. 코드가 아닌 Row 편집으로 튜닝.
- **부여 단위**: 캐릭터 BP에 `UWxAbilitySet`을 지정 → `InitAbilityActorInfo` 시점에 어빌리티/이펙트/어트리뷰트 일괄 부여(`FWxAbilitySetGrantedHandles`로 일괄 제거).
- **새 이펙트/계산**: `WxEffect_*`(GameplayEffect), `WxMMC_*`(ModifierCalc), `WxExecCalc_*`(ExecutionCalc), `WxCueNotify_*`(GameplayCue) 네이밍 규약을 따른다.
- **쿨다운/코스트 규약**: 공용 `UWxEffect_Cooldown`/`UWxEffect_Cost`를 "마커"로 사용, MMC가 Row에서 값을 조회(엔진 순정 경로). 다른 GE로 바꾸면 순정 GE 방식으로 전환(Row와 상호배타).
- **리플리케이션**: AttributeSet은 ReplicatedUsing, 대미지·투사체 스폰은 서버 권위(클라 예측). 락온 대상은 서버 권위 복제 + 소유 클라 예측.

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티 공통 규약(쿨다운/코스트/히트스톱/후딜 캔슬)의 근간. 헤더 주석이 프로젝트 GAS 관례를 요약한다.
2. `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` — 무엇이 어떻게 캐릭터에 부여되는지의 진입점.
3. `Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` — 대미지가 최종 확정되는 판정 흐름(가드/퍼펙트가드/무적/크리).
4. `Source/WxCombat/Public/WxDamageInfo.h` + `Source/WxCombat/Public/WxCombatLibrary.h` — 대미지 데이터의 생성·적용 경로.

## 관련
- 상위: [[WxGame]]의 플레이어/적 캐릭터가 ASC·AbilitySet으로 이 모듈을 구동한다. 소비처로 [[WxUI]](스탯/쿨다운 표시), [[WxAI]](패턴 어빌리티 트리거).

---
*문서 기준 커밋 `b382b78` · 생성일 2026-07-22 · 소스 149파일 — `/readme-writer`로 갱신*
