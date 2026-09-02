# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 올린 액션 RPG 전투 코어. 어빌리티 발동·캔슬 창, 어트리뷰트/자원, 대미지 판정과 GE, 락온·무기·투사체·히트스톱을 담당한다.

## 책임
**담당**
- 어빌리티 골격: 베이스 클래스(`UWxAbilityBase`)와 발동 그룹/캔슬 창(Blocking → ComboWindow → Recovery), 입력 라우팅·선입력 버퍼링
- 전투 어트리뷰트(HP/SP/GP/MP/UP/ATK/DEF 등)와 클램프·그로기·사망 처리
- 대미지 파이프라인: 테이블 주도 대미지 GE + ExecCalc, 크리/가드/퍼펙트가드 판정, 히트스톱
- 락온 타깃팅, 무기 히트박스 스윕, 투사체, 미니언 스폰
- AnimNotify(콤보 창·무기 판정·스냅 등)와 GameplayCue 연출

**경계 (비담당)**
- UI 표시 데이터 규약(`IWxUIData` 등 공용 정의)은 [[WxCore]]에 위임 — 이 모듈은 그 인터페이스를 구현만 한다
- 어빌리티를 언제 트리거할지(AI 의사결정)는 [[WxAI]] 몫 — 이 모듈은 발동 진입점만 제공한다

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 발동 그룹·캔슬 창·쿨/코스트 테이블 참조가 여기 모인다 | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySystemComponent` | ASC. 라이브 입력 라우팅과 ASPD 반영 몽타주 재생의 진입점 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilitySet` | 어빌리티·이펙트·어트리뷰트 초기값을 캐릭터에 일괄 부여하는 DataAsset | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | 전투 자원/스탯 정의, 클램프·그로기·피격 메타 통로 | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxCombatLibrary` | 대미지 성립 판정(`CheckDamage`)과 적용(`ApplyDamage`)의 공용 진입점 | `Source/WxCombat/Public/WxCombatLibrary.h` |
| `UWxEffectComponent_Table` | GE에 붙여 `FWxEffectTableRow`를 지목하는 조회 앵커, 값은 MMC가 계산 시점에 읽음 | `Source/WxCombat/Public/AbilitySystem/Effect/WxEffectComponent_Table.h` |
| `UWxLockOnComponent` | 락온 대상을 SceneComponent 단위로 서버 권위 복제 | `Source/WxCombat/Public/Targeting/WxLockOnComponent.h` |
| `AWxWeaponBase` | 무기 히트박스(Overlap + 틱 Sweep) 판정, 한 스윙 1회 피격 | `Source/WxCombat/Public/Weapon/WxWeaponBase.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase`(또는 용도별 `WxAbility_*`) 파생. `ActivationPolicy`(OnTriggered/OnGiven)·`ActivationGroup`(Independent/Exclusive/Override)을 정하고, 쿨/코스트는 `AbilityDataRow`(`FWxAbilityTableRow`)에서 읽는다. 콤보/후딜 전이는 몽타주 노티파이(`WxAnimNotifyState_ComboWindow` 등)가 `OpenComboWindow`/`StartRecovery`로 몬다.
- 새 GE: `UGameplayEffect` 파생에 `UWxEffectComponent_Table`을 붙여 수치를 테이블에서 읽게 한다(스펙 주입 없이 MMC가 계산 시점에 조회). 대미지는 `UWxEffect_Damage` + `UWxExecCalc_Damage` 경로, 대미지 계수·반응 태그·추가 효과는 `FWxDamageTableRow`가 저작한다.
- 데이터 주도 부여: 캐릭터 BP는 `UWxAbilitySet`만 지정하면 서버 `InitAbilitySystem` 시점에 어빌리티·이펙트·어트리뷰트 초기값이 일괄 부여된다.
- 리플리케이션: 서버 권위 + ASC 예측 모델. 대미지 컨텍스트 `FWxCombatEffectContext`(크리 판정 운반)는 `UWxAbilitySystemGlobals`가 `MakeEffectContext`에서 생성하므로 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName` 등록이 전제다.

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 발동 그룹·캔슬 창(Blocking/ComboWindow/Recovery) 모델이 전투 흐름 전체를 규정한다
2. `Source/WxCombat/Public/WxCombatLibrary.h` — 히트 하나가 대미지로 성립하는 판정 경로의 진입점
3. `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 자원/스탯 약어와 그로기·메타 통로 정의

## 관련
- 상위: 캐릭터/[[WxAI]] 폰이 `UWxAbilitySet`으로 이 모듈을 소비하고, GameFeature 콘텐츠 플러그인이 어빌리티·GE 에셋을 저작한다. 공용 정의는 [[WxCore]]를 참조한다.

---
*문서 기준 커밋 `27fb65d` · 생성일 2026-09-02 · 소스 169파일 — `/readme-writer`로 갱신*
