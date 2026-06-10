# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반의 액션 RPG 전투 도메인. 어빌리티·어트리뷰트·이펙트·큐로 데미지/스킬/상태이상을 처리하고, 무기·투사체·락온·히트스톱 등 액션 전투의 런타임을 담당한다.

## 책임
**담당**
- GAS 통합: 커스텀 ASC(`UWxAbilitySystemComponent`), 전투 AttributeSet, AbilitySet 일괄 부여
- 어빌리티: 공격/회피/가드/스킬/궁극기/질주/락온, 피격반응·그로기·사망, 보스 패턴 등 `UWxAbilityBase` 파생 (쿨다운·코스트·후딜 캔슬 공용 처리)
- GameplayEffect/ExecCalc/MMC/Cue: 데미지·번·버프·코스트·리소스 회복·치트성 이펙트
- 무기/투사체/영역 히트 판정과 데미지 적용(`AWxWeaponBase`, `AWxProjectileBase`, `AWxEffectZone`, `UWxCombatLibrary::ApplyDamage`)
- 락온/타겟팅(TargetingSystem 필터 태스크), 멀티 동기화되는 글로벌 타임딜레이션
- 전투용 AnimNotify/AnimNotifyState (콤보 윈도우, 무적, 퍼펙트가드, 무기 공격 구간, 투사체/광역 스폰, 후딜 시작)

**경계 (비담당)**
- Gameplay Tag 네이티브 선언 — [[WxCore]]의 태그를 소비만 한다 (이 모듈은 태그를 선언하지 않음)
- 캐릭터 클래스/입력 바인딩/카메라 등 폰 본체 — 게임 모듈([[WxGame]]) 및 [[WxCore]]
- UI 표시/뷰모델 — [[WxUI]] (이 모듈은 아이콘·쿨다운 GE 클래스 등 읽을 데이터만 노출)
- 적 행동 결정(BT/Perception) — [[WxAI]] (이 모듈은 Pattern 어빌리티 실행만)

## 의존성
- **주요 의존**: [[WxCore]](유일한 Wx 의존) / 엔진 서브시스템 `GameplayAbilities`, `GameplayTags`, `GameplayTasks`, `TargetingSystem`, `EnhancedInput`, `AIModule`/`NavigationSystem`, `MotionWarping`, `LevelSequence`/`MovieScene`(스킬 컷씬), `Niagara`(FX)
- 규칙: WxCore 외 Wx 플러그인 참조 없음 ✅

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 프로젝트 ASC. 입력 태그→어빌리티 활성화 라우팅, AbilitySet 부여, 래그돌 복제 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilitySet` | Ability/Effect/Attribute 초기 데이터를 묶은 DataAsset. 캐릭터에 무엇이 부여되는지의 출발점 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxAbilityBase` | 모든 어빌리티의 추상 베이스. 쿨다운/코스트/후딜 캔슬(`StartRecovery`)·ASPD PlayRate·테이블 Row 공용 처리 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | 단일 어트리뷰트 세트. `IncomingDamage` 메타로 데미지 수신 후 HP 차감 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 데미지 한 건의 설계 데이터. AnimNotify가 편집 → `MakeSpecs`로 GE Spec 배열 변환 | `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxExecCalc_Damage` | 데미지 판정의 단일 실행 지점(무적/가드/퍼펙트가드/크리/Raw 분기) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |
| `UWxCombatLibrary` | 무기/투사체 밖 경로의 데미지 적용·적대 판정 BP 라이브러리 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `AWxWeaponBase` | 무기 액터. ANS_WeaponAttack 구간 동안 히트 판정 후 `FWxDamageInfo` 적용 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase` 상속(`Public/AbilitySystem/Ability/`). `ActivationPolicy`(입력/부여) 지정, 쿨다운/코스트는 `CooldownTime`/`MaxRecharges`/`MPCost`/`UPCost` 프로퍼티로 데이터 주도. 후딜 캔슬 구간은 `AN_StartRecovery`가 `StartRecovery()`를 호출해 진입. 입력 연결은 어빌리티가 아니라 AbilitySet 항목의 `InputTag`로 한다(부여 Spec의 소스 태그가 되어 ASC 입력 매칭의 키).
- 새 GameplayEffect/Cue: `Public/AbilitySystem/Effect/`·`Cue/`의 기존 클래스를 본떠 추가, 수치는 SetByCaller로 주입.
- 데이터 주도: 캐릭터별 시작 능력은 `UWxAbilitySet` DataAsset. 어트리뷰트 초기값은 `FWxCombatAttributeInitTableRow`, 어빌리티 수치는 `FWxAbilityTableRow`, 데미지는 `FWxDamageTableRow` DataTable Row로 오버라이드.
- 데미지 경로: 무기/투사체는 `FWxDamageInfo`→`MakeSpecs`→Damage GE, 그 외 광역/환경 단발은 `UWxCombatLibrary::ApplyDamage`/`ApplyRawDamage`. 최종값은 `IncomingDamage` 메타 어트리뷰트로 모아 `PostGameplayEffectExecute`에서 HP 차감.
- 리플리케이션(최대 4인): 입력 태그/래그돌은 ASC에서 서버 RPC·복제, 데미지/판정은 서버 권한 ExecCalc·GE 적용, 타임딜레이션은 GameState 부착 `UWxTimeDilationComponent`가 서버 권위로 전 머신 동기화.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` — 한 캐릭터에 무엇이(어빌리티·이펙트·어트리뷰트) 부여되는지의 출발점
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` — 데미지 판정 전체 흐름이 클래스 doc에 단계별로 정리됨
3. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 모든 어빌리티가 공유하는 쿨다운/코스트/후딜 캔슬 규약

## 관련
- 상위: 게임 모듈 [[WxGame]]이 캐릭터에 ASC/AbilitySet을 부착해 사용. 공용 정의·Gameplay Tag는 [[WxCore]], 표시는 [[WxUI]], 적 행동은 [[WxAI]](Pattern 어빌리티)와 함께 본다.

---
*문서 기준 커밋 `eb4b2a2b` · 생성일 2026-06-11 · 소스 137파일 — `/readme-writer`로 갱신*
