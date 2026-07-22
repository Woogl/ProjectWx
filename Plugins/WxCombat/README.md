# WxCombat — 전투 시스템

> Gameplay Ability System(GAS) 위에 구축한 액션 RPG 전투의 핵심 플러그인. 어빌리티·어트리뷰트·대미지·락온·무기/투사체·히트스톱까지 전투 런타임 전반을 담당한다.

## 책임
**담당**
- GAS 런타임 토대: ASC(`UWxAbilitySystemComponent`), 어트리뷰트 세트(`UWxCombatAttributeSet`), AbilitySet 기반 일괄 부여
- 전투 어빌리티 일습: Attack/Skill/Ultimate/Dodge/Guard/Finisher/HitReact/Groggy/Death/LockOn/Sprint/Pattern
- GameplayEffect·ExecCalc·MMC로 구성한 대미지·코스트·쿨다운·버프/디버프 파이프라인
- 대미지 적용 진입점(`UWxCombatLibrary`, `FWxDamageInfo`)과 무기/투사체 히트 판정
- 락온/타게팅(`UWxLockOnManagerComponent`, TargetingSystem 필터 태스크, MotionWarping 스냅)
- 전투용 AnimNotify(무기 활성 구간, 투사체 스폰, 광역 공격, 콤보 윈도우, 무적 구간 등)와 히트스톱/시간감속

**경계 (비담당)**
- 어빌리티 아이콘의 소프트 참조는 `AbilityDataRow`(`FWxAbilityTableRow::Icon`)가 보유하고 `UWxAbilityBase::GetIcon()`으로 노출 — 실제 표시/비동기 로드는 [[WxUI]] VM이 담당
- 적 AI 의사결정/행동 트리는 [[WxAI]] — WxCombat은 AI가 트리거하는 어빌리티(`WxAbility_Pattern` 등)만 제공

## 의존성
- **주요 의존**: `WxCore`(유일한 Wx 의존), GameplayAbilities, GameplayTags/Tasks, ModularGameplay, EnhancedInput, TargetingSystem, MotionWarping, AIModule, Niagara, LevelSequence/MovieScene
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 프로젝트 ASC. 입력 태그 → 어빌리티 라우팅, AbilitySet 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxCombatAttributeSet` | HP/SP/DP/MP/UP·ATK/DEF·크리티컬 등 전투 스탯, IncomingDamage 메타 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxAbilitySet` | 어빌리티·이펙트·어트리뷰트 초기값을 한 에셋으로 묶어 ASC에 일괄 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. DataRow 기반 쿨다운/코스트/충전, 후딜·히트스톱 규약 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatLibrary` | 무기/투사체 외 경로의 대미지 적용 진입점(`ApplyDamage`/`ApplyRawDamage`) | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터 → Damage GE Spec(SetByCaller·AssetTags)으로 변환 | `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` |
| `AWxWeaponBase` | 무기 액터. AnimNotify가 Begin/EndAttack으로 히트 구간 제어, 스윙당 1회 피격 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxLockOnManagerComponent` | 서버 권위 복제 락온 대상(SceneComponent 단위) 저장/조회 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase` 상속(Blueprintable). 쿨다운/코스트/충전 수치는 `AbilityDataRow`(`FWxAbilityTableRow`)에서 읽고, 공용 `UWxEffect_Cooldown`/`UWxEffect_Cost` GE + MMC가 Row 값을 온디맨드 해석한다(어빌리티별 GE 미작성).
- 데이터 주도: `UWxAbilitySet`(부여 세트), `WxAbilityTableRow`/`WxDamageTableRow`/`WxCombatAttributeInitTableRow`(DataTable Row)로 수치를 외부화.
- 대미지 계산: `WxExecCalc_Damage`가 ATK/DEF·크리티컬·가드/퍼펙트가드·HitStop을 처리하고 `IncomingDamage` 메타 어트리뷰트로 HP를 차감(`PostGameplayEffectExecute`).
- 새 타게팅 필터: `WxTargetingFilterTask_*`(TargetingSystem) 상속. 새 접촉 이펙트 존은 `AWxEffectZone` 상속.
- 리플리케이션: ASC/어트리뷰트는 서버 권위 복제, 락온 대상도 서버 권위(소유 클라는 로컬 예측 후 서버 확정). 투사체는 서버 스폰·복제.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 쿨다운/코스트/후딜/히트스톱 규약이 집약된 어빌리티 계약서
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 스탯 약어(HP/SP/DP/MP/UP…)와 대미지 흐름의 사전
3. `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` — 대미지 데이터가 GE Spec으로 변환되는 지점, 전투 흐름의 허리
4. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` — 캐릭터가 전투 능력을 획득하는 부팅 경로

## 관련
- 상위: 플레이어/적 캐릭터가 ASC·AbilitySet으로 이 모듈을 소비. UI 표시는 [[WxUI]], AI 트리거는 [[WxAI]]와 함께 본다.

---
*문서 기준 커밋 `b850e71` · 생성일 2026-07-21 · 소스 149파일 — `/readme-writer`로 갱신*
