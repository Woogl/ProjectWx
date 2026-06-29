# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반의 액션 RPG 전투 도메인. 어빌리티·이펙트·어트리뷰트로 캐릭터의 공격/방어/피격/사망과 대미지 파이프라인을 구동하고, 락온·무기·투사체·시간 감속 등 전투 주변 시스템을 함께 제공한다.

## 책임
**담당**
- ASC/AttributeSet/AbilitySet — 캐릭터에 어빌리티·이펙트·어트리뷰트를 부여하는 GAS 런타임 골격
- 어빌리티 카탈로그 — Attack/Dodge/Guard/Skill/Ultimate/HitReact/Death/LockOn 등 플레이어·AI 공용 어빌리티 베이스 및 구현
- 대미지 파이프라인 — `FWxDamageInfo` → Damage GE Spec → `UWxExecCalc_Damage`(크리·가드·퍼펙트가드·무적 판정) → AttributeSet HP/DP/SP 차감
- GameplayEffect/Cue/MMC/ExecCalc — 버프·도트·코스트·쿨다운 GE와 피격/히트스탑 등 연출 큐
- AnimNotify(State) — 몽타주 구동 히트박스 활성, 무적/퍼펙트가드 윈도우, 콤보 윈도우, 투사체/광역 발생, 게임플레이 이벤트 송출
- 타게팅/락온 — `TargetingSystem` 기반 후보 필터 + `UWxLockOnManagerComponent` 복제 대상 관리
- 무기/투사체/이펙트존 — 히트 콜리전 액터(`AWxWeaponBase`/`AWxProjectileBase`/`AWxEffectZone`)
- 전투용 시간 제어 — 서버 권위 글로벌 TimeDilation, 어빌리티 단위 슬로우/히트스탑

**경계 (비담당)**
- 입력 바인딩·플레이어 컨트롤러·캐릭터 클래스 정의는 [[WxGame]]/캐릭터 측 (이 모듈은 `InputTag` 라우팅 진입점만 제공)
- 어빌리티 UI 표시 데이터(아이콘 등)는 `UWxAbilityComponent` 파생으로 [[WxUI]]에 위임 (이 모듈은 `Components` 슬롯과 베이스만 정의)
- AI 의사결정/패턴 선택 로직은 [[WxAI]] (이 모듈은 `WxAbility_Pattern` 실행 골격만)
- 공용 팀/태그/베이스 정의 등 foundation은 [[WxCore]]

## 의존성
- **주요 의존**: [[WxCore]], `GameplayAbilities`, `GameplayTags`, `GameplayTasks`, `TargetingSystem`, `EnhancedInput`, `ModularGameplay`, `MotionWarping`, `AIModule`/`NavigationSystem`, `Niagara`, `LevelSequence`/`MovieScene`(스킬 컷신)
- 규칙: 플러그인이므로 「WxCore 외 Wx 플러그인 참조」를 검증 — 없음 ✅ (`.uplugin`·`Build.cs` 모두 Wx 중 `WxCore`만 참조)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 캐릭터 ASC. `InputTag`→어빌리티 활성화 라우팅, AbilitySet 부여, 래그돌 복제 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilitySet` | 캐릭터 BP가 지정하는 부여 데이터 에셋(어빌리티+이펙트+어트리뷰트 초기값). 모듈의 데이터 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 공용 쿨다운/코스트 GE 위임, 후딜=캔슬 구간, 테이블 Row 수치 주입 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | HP/SP/DP/MP/UP·ATK/DEF/Crit 등 전투 스탯과 `IncomingDamage` 메타 어트리뷰트 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터. AnimNotify에서 편집→무기/투사체로 전달→GE Spec 변환 | `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` |
| `UWxExecCalc_Damage` | 대미지 계산 핵심. ATK·DEF·크리·가드·퍼펙트가드·무적 판정의 단일 지점 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |
| `UWxCombatLibrary` | 무기/투사체 외 경로의 대미지 적용 진입점(`ApplyDamage`/`ApplyRawDamage`) 및 적대 판정 | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `AWxWeaponBase` | 몽타주 ANS가 구동하는 히트 콜리전 무기 액터. 스윙당 1히트 보장 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent 단위) 복제 관리. 발사체 방향/몽타주 스냅이 소비 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase`(또는 `WxAbility_*`) 상속. 쿨다운/코스트는 `CooldownTime`/`MaxRecharges`/`MPCost`/`UPCost` 프로퍼티나 `AbilityDataRow`(`FWxAbilityTableRow`)로 설정 — 별도 쿨다운/코스트 GE 작성 불필요(공용 `UWxEffect_Cooldown`/`UWxEffect_Cost` 위임).
- 데이터 주도: `UWxAbilitySet`(어빌리티+`GrantedEffects`+`AttributeInitRow`)이 캐릭터 1대를 구동. 어빌리티 밸런스는 `WxAbilityTableRow` DataTable, 대미지 설계는 `WxDamageTableRow`(`FWxDamageInfo::ApplyTableRow`). `InputTag`가 빈 어빌리티는 입력이 아닌 부여/이벤트로 발동(AI 패턴 등).
- 대미지 추가 경로: 무기/투사체 외에는 `UWxCombatLibrary::ApplyDamage`/`ApplyRawDamage`를 쓴다. 추가 상태이상은 `FWxDamageInfo::AdditionalEffects`로 Damage GE와 함께 적용.
- 리플리케이션/권한(최대 4인 멀티): 어트리뷰트·락온 대상·래그돌·글로벌 TimeDilation은 서버 권위 복제. 락온은 소유 클라가 로컬 예측 후 서버 정합. TimeDilation은 CMC 동기화를 위해 `UWxTimeDilationComponent`로 전 머신에 강제 동기화.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` — 캐릭터가 무엇을 부여받는지(어빌리티·이펙트·어트리뷰트)부터 보면 모듈 전체 데이터 흐름이 잡힌다
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 모든 어빌리티의 공용 규약(쿨다운/코스트/후딜/테이블)
3. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` — 대미지가 가드/크리/무적을 거쳐 HP까지 가는 판정 흐름(헤더 주석에 단계별 명시)
4. `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` — AnimNotify→무기→GE Spec로 이어지는 대미지 데이터의 출발점

## 관련
- 상위: 캐릭터 클래스가 ASC/AttributeSet/AbilitySet을 호스팅하고 이 모듈의 어빌리티를 사용 — [[WxGame]]. AI 패턴 구동은 [[WxAI]], 어빌리티 UI 표시는 [[WxUI]], 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `97577fb` · 생성일 2026-06-29 · 소스 149파일 — `/readme-writer`로 갱신*
