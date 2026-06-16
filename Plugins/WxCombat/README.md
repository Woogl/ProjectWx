# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반 액션 RPG 전투의 런타임 전반을 책임진다. 어빌리티/이펙트/어트리뷰트, 무기·투사체 히트 판정, 대미지 계산, 락온, 타임 딜레이션까지 전투에 필요한 C++ 토대를 제공한다.

## 책임
**담당**
- GAS 토대: `UWxAbilitySystemComponent`, `UWxCombatAttributeSet`(HP/SP/DP/MP/UP/ATK/DEF 등), `UWxAbilitySet`(어빌리티·이펙트·어트리뷰트 일괄 부여)
- 어빌리티 베이스(`UWxAbilityBase`)와 구체 어빌리티들(공격/회피/가드/스킬/궁극기/락온/스프린트, AI 패턴/페이즈, 사망/그로기/히트리액트)
- 대미지 파이프라인: `FWxDamageInfo` → `UWxEffect_Damage` Spec → `UWxExecCalc_Damage`(크리/가드/퍼펙트가드/무적/반사 판정)
- 무기·투사체·이펙트존을 통한 히트 판정과 GE 적용 (`AWxWeaponBase`, `AWxProjectileBase`, `AWxEffectZone`)
- 전투용 AnimNotify(공격 구간/무적/퍼펙트가드/스냅/투사체 스폰 등)로 몽타주와 전투 로직 연결
- 락온/타게팅(`UWxLockOnManagerComponent`, TargetingSystem 필터 태스크들)과 타임 딜레이션(`UWxTimeDilationComponent`)
- GameplayEffect 라이브러리(버프/번/쿨다운/코스트/회복/반사 등)와 GameplayCue 노티파이

**경계 (비담당)**
- 어빌리티의 UI 표시 데이터(아이콘 등)는 도메인별 `UWxAbilityComponent` 파생으로 위임 — UI 메타데이터는 [[WxUI]]
- 입력 액션 자산/매핑 정의는 본 모듈 밖. 본 모듈은 InputTag → 어빌리티 라우팅만 담당
- 캐릭터 클래스/이동/AI 의사결정 자체는 비담당(어트리뷰트·이벤트 소비처). AI 패턴 발동만 어빌리티로 노출

## 의존성
- **주요 의존**: `WxCore`, GameplayAbilities, GameplayTags, GameplayTasks, EnhancedInput, TargetingSystem, MotionWarping(Private), Niagara(Private), LevelSequence/MovieScene(Private, 스킬 컷씬), AIModule/NavigationSystem, NetCore, UMG
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (uplugin·Build.cs 모두 Wx 중 `WxCore`만 참조)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 캐릭터에 붙는 ASC. InputTag 라우팅·AbilitySet 부여·래그돌 복제 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilitySet` | 어빌리티/이펙트/어트리뷰트 초기값을 한 데이터에셋으로 묶어 일괄 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 공용 쿨다운/코스트 GE, DataTable 수치 주입, 후딜 캔슬 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | 전투 어트리뷰트 + IncomingDamage 메타. PostGameplayEffectExecute에서 HP 차감 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터. AnimNotify에서 편집→무기/투사체로 전달→Spec 변환 | `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxExecCalc_Damage` | 대미지 최종 계산. 크리/가드/퍼펙트가드/무적/반사 판정의 중심 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |
| `AWxWeaponBase` | 근접 무기. ANS가 BeginAttack/EndAttack 호출, Overlap+Sweep으로 히트 처리 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent 단위) 서버 권위 복제·예측 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |
| `UWxCombatLibrary` | 무기 외 경로의 대미지 적용/적대 판정 BlueprintFunctionLibrary | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase` 상속(BP/C++). 쿨다운은 `CooldownTime`/`MaxRecharges`, 코스트는 `MPCost`/`UPCost` 프로퍼티로 설정 — 공용 `UWxEffect_Cooldown`/`UWxEffect_Cost`를 자동 사용. 활성화는 `EWxAbilityActivationPolicy`(OnInputTriggered/OnGranted).
- 데이터 주도 수치: `UWxAbilityBase::AbilityDataRow`(`WxAbilityTableRow`), 어트리뷰트 초기값은 `UWxAbilitySet::AttributeInitRow`(`WxCombatAttributeInitTableRow`), 대미지는 `FWxDamageInfo`/`WxDamageTableRow`. AbilitySet으로 캐릭터별 그랜트를 한 에셋에 집약.
- 대미지 추가 효과: `FWxDamageInfo::AdditionalEffects`에 GE 클래스를 더하면 Damage Spec과 함께 적용. 본체 GE는 `UWxEffect_Damage` + `UWxExecCalc_Damage`.
- AnimNotify로 전투 타이밍 노출: 공격 구간은 `UWxAnimNotifyState_WeaponAttack`(무기에 DamageInfo 전달), 무적/퍼펙트가드/스냅/후딜/투사체·광역은 대응 노티파이.
- 리플리케이션/권한: ASC·어트리뷰트·락온은 서버 권위. 락온/InputTag는 소유 클라 즉시 반영(예측) 후 서버 동기화. 대미지 적용은 권위 측 GE 실행.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티 모델·쿨다운/코스트 규약. 모든 전투 동작의 출발점
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` — 헤더 주석에 대미지 판정 6단계가 정리되어 있어 전투 룰을 가장 빨리 파악
3. `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` + `Public/Weapon/WxWeaponBase.h` — 애님 → 무기 → 대미지로 이어지는 히트 데이터 흐름
4. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 약어(HP/SP/DP/MP/UP …)와 복제/메타 어트리뷰트 정의

## 관련
- 상위: 캐릭터/플레이어·AI(게임 모듈 [[WxGame]], [[WxAI]])가 ASC·AbilitySet·락온을 장착해 사용. 어빌리티 UI 표시는 [[WxUI]], 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `8fb8b93` · 생성일 2026-06-16 · 소스 141파일 — `/readme-writer`로 갱신*
