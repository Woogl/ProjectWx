# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 액션 RPG 전투를 구현하는 도메인 플러그인. 어빌리티·어트리뷰트·대미지 판정·락온·무기/투사체·히트스톱과 타임딜레이션까지, 한 번의 적중이 성립하고 연출까지 이어지는 전 경로를 소유한다.

## 책임
**담당**
- 어빌리티 프레임워크: 입력/이벤트/부여로 발동하는 어빌리티(공격·회피·가드·스킬·궁극기·피니셔·히트리액트·그로기·사망·AI 패턴)와 이를 캐릭터에 일괄 부여하는 AbilitySet.
- 전투 어트리뷰트(HP/SP/DP/MP/UP, ATK/DEF/Crit/SPD 등)와 이를 조작하는 GameplayEffect·MMC·ExecCalc 묶음.
- 대미지 판정 파이프라인: `FWxDamageInfo` → Spec 생성 → `UWxExecCalc_Damage`가 무적/가드/퍼펙트가드/Unblockable을 갈라 어트리뷰트 차감 → 결과를 EffectContext에 실어 ASC가 Cue·반응 이벤트 발행.
- 락온(대상 선정·복제·추적), 무기/투사체 액터, 히트 콜리전, AnimNotify 기반 공격 구간·콤보 윈도우·무적·스냅.
- 히트스톱(역경직)과 전역 TimeDilation의 서버 권위 관리.

**경계 (비담당)**
- 체력바·스킬 아이콘·대미지 플로터 위젯 등 실제 UI 위젯 → [[WxUI]] (여기서는 Cue/이벤트만 발행).
- 어빌리티를 언제 쓸지 결정하는 AI(행동트리·인지) → [[WxAI]] (`UWxAbility_Pattern`은 발동 틀만 제공).
- 공용 정의·팀 판정 등 foundation 타입 → [[WxCore]].

## 의존성
- **주요 의존**: `WxCore` · GameplayAbilities · GameplayTags/Tasks · TargetingSystem · MotionWarping · ModularGameplay · EnhancedInput · AIModule
- 규칙: WxCore 외 Wx 플러그인 참조 — 없음 ✅ (`WxCombat.Build.cs`의 Wx 의존은 `WxCore` 단 하나)

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxCombatLibrary` | `ApplyDamage` — 무기·피니셔·투사체가 공유하는 대미지 적용 단일 진입점 | [`Source/WxCombat/Public/WxCombatLibrary.h`](Source/WxCombat/Public/WxCombatLibrary.h) |
| `UWxExecCalc_Damage` | 대미지 판정의 두뇌. 상태별 차감 규칙 전부가 여기 있고 결과를 EffectContext에 남긴다 | [`Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h`](Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h) |
| `FWxDamageInfo` | 대미지 한 건의 설계 데이터. Damage 테이블 행에서 만들어져 GE Spec으로 변환 | [`Source/WxCombat/Public/Damage/WxDamageInfo.h`](Source/WxCombat/Public/Damage/WxDamageInfo.h) |
| `UWxAbilitySystemComponent` | 입력→어빌리티 라우팅, 히트스톱, GE 적용 후 Cue/반응 이벤트 발행 허브 | [`Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h`](Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h) |
| `UWxAbilitySet` | 캐릭터 BP가 지정하면 어트리뷰트·어빌리티·이펙트를 ASC에 일괄 부여하는 DataAsset | [`Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h`](Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h) |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. Row 기반 쿨다운/코스트, ActivationPolicy, 활성 구간 오너 이펙트 | [`Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h`](Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h) |
| `UWxCombatAttributeSet` | HP/SP/DP/MP/UP 등 전투 스탯 정의와 클램프·사망·그로기 트리거 | [`Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h`](Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h) |
| `UWxLockOnManagerComponent` | 락온 대상(SceneComponent 단위)을 서버 권위로 관리·복제, 변경 델리게이트 방송 | [`Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h`](Source/WxCombat/Public/Targeting/WxLockOnManagerComponent.h) |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase`(Abstract, Blueprintable)를 상속. 입력 발동이면 `ActivationInputAction`을, 수치는 `AbilityDataRow`(`WxAbilityTableRow`)를 채우고 AbilitySet의 `GrantedAbilities`에 등록한다.
- **쿨다운/코스트**: 기본은 Row 주도 — 공용 `UWxEffect_Cooldown`/`UWxEffect_Cost` 마커를 그대로 두면 Row 값을 SetByCaller로 싣고, 다른 GE로 바꾸면 엔진 순정 경로(Row와 상호배타). 공용 GE 상속 금지(지속시간이 조용히 1초가 된다).
- **새 대미지 효과/상태이상**: GE를 만들어 `FWxDamageInfo::AdditionalEffects`에 추가하면 Damage GE와 같은 컨텍스트로 함께 적용된다.
- **데이터 주도 설정**: 대미지=`WxDamageTableRow`, 어빌리티=`WxAbilityTableRow`, 어트리뷰트 초기값=`WxCombatAttributeInitTableRow` DataTable로 저작하고 AbilitySet이 참조한다.
- **락온 필터**: TargetingSystem의 `UWxTargetingFilterTask_*`(Team/LineTrace/ScreenBounds/InputDirection/GameplayTag)로 후보를 거른다.
- **리플리케이션/권한**: 판정·상태변화는 서버 권위. 대미지 GE는 Instant+Execution이라 클라 예측은 ExecCalc를 건너뛰며(EffectContext의 `DamageResult`가 None으로 남아 연출 생략), 히트스톱만 `UWxExecCalc_Damage::CheckDamage`를 로컬에서 직접 돌려 서버와 결론을 맞춘다. EffectContext 타입 주입은 `DefaultGame.ini`의 `UWxAbilitySystemGlobals` 등록에 의존한다.

## 여기서부터 읽어라
1. [`Source/WxCombat/Public/WxCombatLibrary.h`](Source/WxCombat/Public/WxCombatLibrary.h) — `ApplyDamage` 한 함수가 대미지 전 경로의 입구다. 예측/권위 모델 주석이 흐름의 지도.
2. [`Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h`](Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h) — `UWxExecCalc_Damage`. 상태별 판정 규칙이 모두 여기 있다.
3. [`Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h`](Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h) — 입력이 어떻게 어빌리티로 가고, GE 적용 뒤 어떻게 Cue/이벤트가 나가는지.
4. [`Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h`](Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h) — 한 캐릭터가 무엇을 부여받는지(어트리뷰트·어빌리티·이펙트)의 조립 지점.

## 관련
- 상위: 이 ASC/AbilitySet을 캐릭터에 붙이고 입력을 바인딩하는 곳은 게임 모듈([[WxGame]]) 및 Experience/GameFeature 주입. 연출 위젯은 [[WxUI]], 어빌리티 사용 판단은 [[WxAI]]와 함께 본다.

---
*문서 기준 커밋 `36dc0e1` · 생성일 2026-08-18 · 소스 153파일 — `/readme-writer`로 갱신*
