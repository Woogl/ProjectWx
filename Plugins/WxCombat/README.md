# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 얹은 액션 RPG 전투의 핵심. 어빌리티/이펙트/어트리뷰트, 무기·투사체 히트 판정, 대미지 계산, 락온 타게팅, 시간 감속을 담당한다.

## 책임
**담당**
- 어빌리티 스택: 공용 베이스(`UWxAbilityBase`)와 공격/회피/가드/스킬/궁극기/AI 패턴 등 파생 어빌리티, 쿨다운·코스트의 공용 GE 처리
- 어트리뷰트/데미지: `UWxCombatAttributeSet`(HP/SP/DP/MP/UP/ATK/DEF 등)과 `UWxExecCalc_Damage` 기반 데미지·가드·퍼펙트가드·크리 판정
- GameplayEffect / Cue / ExecCalc / MMC 묶음 (버프·번·회복·반사 등 상태 처리)
- 무기·투사체·이펙트존을 통한 히트 콜리전 → 대미지 Spec 적용 경로
- 락온 타게팅(`TargetingSystem` 필터 태스크 + 락온 매니저)과 전투용 AnimNotify(콤보 윈도우·무적·무기 공격 등)
- 서버 권위 Global TimeDilation 동기화(`UWxTimeDilationComponent`)

**경계 (비담당)**
- 네이티브 Gameplay Tag 선언(`WxGameplayTags`)과 팀/캐릭터 기반 등 공용 정의 → [[WxCore]]
- 캐릭터 클래스·입력 바인딩·게임모드 등 게임 조립 → [[WxGame]]
- 어빌리티 UI 표시 데이터(아이콘 등 `UWxAbilityComponent` 파생) → [[WxUI]]

## 의존성
- **주요 의존**: [[WxCore]], 엔진: GameplayAbilities, TargetingSystem, ModularGameplay, EnhancedInput, MotionWarping, Niagara, AIModule
- 규칙: 「WxCore 외 Wx 플러그인 참조」 — 없음 ✅ (`WxCombat.Build.cs`의 Wx 의존은 `WxCore`뿐)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 쿨다운·코스트·테이블Row·후딜 캔슬을 통합 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 캐릭터에 어빌리티/이펙트/어트리뷰트 초기값을 일괄 부여하는 데이터에셋 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxAbilitySystemComponent` | 입력 태그 라우팅·래그돌 복제를 얹은 프로젝트 ASC | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxCombatAttributeSet` | 전투 스탯 세트와 IncomingDamage 메타 어트리뷰트 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxExecCalc_Damage` | 데미지·가드·퍼펙트가드·크리 판정의 실계산 지점 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터 → Damage Spec 배열로 변환 | `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` |
| `AWxWeaponBase` / `AWxProjectileBase` | 히트 콜리전에서 DamageInfo를 Spec으로 적용하는 무기·투사체 | `Plugins/WxCombat/Source/WxCombat/Public/Weapon/` |
| `UWxCombatLibrary` | 무기·투사체 외 경로의 대미지 적용·적대 판정 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |

## 확장 포인트 / 규약
- 새 어빌리티는 `UWxAbilityBase`(또는 `UWxAbility_Attack` 등 파생)를 상속. 쿨다운/코스트는 프로퍼티로 지정하고 공용 `UWxEffect_Cooldown`/`UWxEffect_Cost` GE를 재사용한다(개별 GE 작성 불필요)
- 데이터 주도 설정: `UWxAbilitySet`(DataAsset)로 부여, `FWxAbilityTableRow`/`FWxCombatAttributeInitTableRow`/`FWxDamageTableRow`(DataTable)로 수치 주입
- 대미지 파이프라인: `FWxDamageInfo` → `MakeSpecs()` → `UWxEffect_Damage` Spec(SetByCaller/AssetTags) → `UWxExecCalc_Damage` → `IncomingDamage` 메타 → HP 차감(`PostGameplayEffectExecute`)
- 상태 태그는 여기서 선언하지 않고 [[WxCore]]의 `WxGameplayTags`를 참조한다(예: `State.Invincible`, `State.PerfectGuard`, `State.LockedOn`)
- 리플리케이션: 어트리뷰트·락온 대상·TimeDilation은 서버 권위 복제. 입력 태그는 클라→서버 RPC로 동기화

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 어빌리티 계약(쿨다운·코스트·후딜 캔슬)의 전제. 모든 파생 어빌리티의 출발점
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxExecCalc_Damage.h` — 피격 결과가 결정되는 곳. 헤더 주석의 판정 흐름이 전투 규칙 요약
3. `Plugins/WxCombat/Source/WxCombat/Public/WxDamageInfo.h` — 공격 데이터가 GE Spec으로 바뀌는 변환 지점. 무기·투사체·라이브러리가 공통으로 소비

## 관련
- 상위: 캐릭터/입력/게임모드 조립은 [[WxGame]], 공용 태그·정의는 [[WxCore]]. UI 표시 연동은 [[WxUI]]

---
*문서 기준 커밋 `9554c3c` · 생성일 2026-07-08 · 소스 149파일 — `/readme-writer`로 갱신*
