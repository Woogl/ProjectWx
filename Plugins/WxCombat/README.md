# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 기반 액션 RPG 전투 플러그인. 어빌리티·어트리뷰트·데이터 주도 GameplayEffect, 입력 버퍼·히트스톱, 락온·무기 히트박스·투사체·처형까지 런타임 전투 전반을 담당한다.

## 책임
**담당**
- 어빌리티 파이프라인: `UWxAbilityBase` 파생(공격/회피/가드/스킬/궁극/패시브/피격·그로기·사망 등)과 발동 그룹(Exclusive/Override)·캔슬 창(Blocking→ComboWindow→Recovery) 배타 제어
- 어트리뷰트·데미지 파이프라인: `UWxCombatAttributeSet`(HP/SP/GP/MP/UP/ATK/DEF/Crit/SPD/ASPD), ExecCalc 기반 데미지 산출과 크리·그로기·퍼펙트가드 반사 처리
- 데이터 주도 GameplayEffect: DataTable 행을 MMC/컴포넌트로 읽어 코스트·쿨다운·데미지·상태 GE의 수치를 채움
- 입력 라우팅과 버퍼링, 몽타주 재생 속도(ASPD) 관리 (`UWxAbilitySystemComponent`), 히트스톱 반응 (`UWxHitStopComponent`)
- 락온(SceneComponent 단위·서버 복제), 무기 히트박스 스윕, 투사체, 처형 데미지, AnimNotify 기반 전투 이벤트
- GameplayCue 연출(피격/데미지 플로터/공격 텔레그래프/퍼펙트가드 등)과 TargetingSystem 필터 태스크

**경계 (비담당)**
- UI 표시(체력바·데미지 수치 위젯 등)는 [[WxUI]]로 위임 — 전투는 `IWxUIData`로 표시 데이터만 노출한다
- AI 의사결정·패턴 선택은 [[WxAI]]로 위임 — 전투는 발동 진입점만 제공한다
- 공용 정의(`IWxUIData`, GameplayTag 등)는 [[WxCore]]에 둔다

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | ASC. 입력 라우팅·버퍼·몽타주 속도의 단일 진입점 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxHitStopComponent` | `Effect.HitStop`을 부여하는 GE의 추가·제거를 받아 몽타주를 얼리고 되돌린다 | `Source/WxCombat/Public/AbilitySystem/WxHitStopComponent.h` |
| `UWxAbilityBase` | 모든 어빌리티 베이스. 발동 그룹·캔슬 창·데이터 행·활성 GE | `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 캐릭터에 어빌리티·이펙트·어트리뷰트 초기값을 일괄 부여하는 데이터 에셋 | `Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | 전투 어트리뷰트와 데미지·그로기·퍼펙트가드 처리 파이프라인 | `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxCombatLibrary` | 데미지·이펙트 적용의 공용 진입점(`ApplyDamage`·`CheckDamage`) | `Source/WxCombat/Public/WxCombatLibrary.h` |
| `UWxExecCalc_Damage` | ExecCalc. `FWxCombatEffectContext`로 크리 판정을 실어 IncomingDamage 산출 | `Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h` |
| `AWxWeaponBase` | 무기 액터. ShapeComponent 히트박스 스윕/오버랩(한 스윙 1회 피격) | `Source/WxCombat/Public/Weapon/WxWeaponBase.h` |
| `UWxLockOnComponent` | 락온 대상(SceneComponent 단위)을 서버 권위로 복제 | `Source/WxCombat/Public/Targeting/WxLockOnComponent.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase`를 상속(대개 `WxAbility_*` 계열 중 가까운 것). `ActivationPolicy`(OnTriggered/OnGiven)·`ActivationGroup`(Independent/Exclusive/Override)·`ActivationInputAction`·`AbilityDataRow`를 CDO에서 설정한다. 캔슬 창은 몽타주 노티파이가 `OpenComboWindow`/`StartRecovery`로 전이시킨다.
- **데이터 주도 GE**: 코스트·쿨다운·데미지 등 수치는 GE 서브클래스가 아니라 DataTable 행(`WxAbilityTableRow`/`WxEffectTableRow`/`WxDamageTableRow`)에서 읽는다. GE에 `UWxEffectComponent_Table`을 붙이면 `UWxMMC_EffectMagnitude`/`_EffectDuration`이 계산 시점에 행을 조회한다.
- **히트스톱**: 무기·투사체 BP의 `HitStopDuration`(초)이 전부다. `UWxEffect_HitStop`은 `Effect.HitStop` 태그를 `SetByCaller.Duration`만큼 부여할 뿐이고, 무기는 공격자·피격자 양쪽에, 투사체는 피격자에게만 `UWxEffect_HitStop::Apply`로 적중마다 건다. 캐릭터의 `UWxHitStopComponent`가 인스턴스의 추가·제거를 받아 몽타주를 얼리고, 마지막 인스턴스가 빠질 때 되돌린다.
- **데미지**: `UWxCombatLibrary::ApplyDamage`(또는 `AWxWeaponBase`/`UWxFinisherDamageComponent`)로 진입한다. 크리 등 어트리뷰트로 못 싣는 정보는 `FWxCombatEffectContext`를 통해 전달되며, 이는 `UWxAbilitySystemGlobals`가 `MakeEffectContext`에서 할당한다(`DefaultGame.ini`의 `AbilitySystemGlobalsClassName` 등록 필수).
- **부여**: 캐릭터 BP가 `UWxAbilitySet`을 지정하면 서버가 `GiveToAbilitySystem`으로 어빌리티·이펙트·어트리뷰트 초기값을 일괄 부여한다. 입력 라우팅 키는 각 어빌리티 CDO의 `ActivationInputAction`이 쥔다.
- **리플리케이션**: 어트리뷰트는 서버 권위·RepNotify, 락온은 서버 권위 복제(대상 선택은 클라 신뢰). ExecCalc·데미지 판정은 서버에서 확정한다.

## 여기서부터 읽어라
1. `Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력·버퍼·몽타주 속도가 만나는 전투의 심장. 어빌리티 라우팅 진입점
2. `Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 발동 그룹·캔슬 창 개념이 모두 여기 정의됨. 모든 어빌리티의 골격
3. `Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 어트리뷰트 목록과 데미지가 HP로 흘러가는 파이프라인
4. `Source/WxCombat/Public/WxCombatLibrary.h` — 외부(무기·투사체·AI)가 전투에 데미지를 넣는 공용 문

## 관련
- 상위: <[[WxGame]]> (캐릭터에 ASC·AbilitySet 부착), <[[WxCore]]> (공용 정의·표시 데이터 계약)

---
*문서 기준 커밋 `ee3c177` · 생성일 2026-09-01 · 소스 161파일 — `/readme-writer`로 갱신*
