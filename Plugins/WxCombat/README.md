# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 얹은 액션 RPG 전투의 몸통. 어빌리티 발동·배타 점유, 어트리뷰트와 데미지 파이프라인, 히트 반응(히트스톱·그로기·가드/패리), 락온 타게팅, 무기·투사체·소환물 스폰까지 실제 전투가 굴러가는 규칙을 담는다.

## 책임
**담당**
- 어빌리티 수명·발동 규칙: `UWxAbilityBase`의 활성화 그룹(Independent/Exclusive/Override)과 액션 페이즈(Blocking→ComboWindow→Recovery)로 배타 점유·캔슬 창을 판정.
- 입력 라우팅과 선입력 버퍼(`UWxAbilitySystemComponent` + `UWxInputBufferComponent`).
- 전투 어트리뷰트와 데미지/코스트/쿨다운 계산(`UWxCombatAttributeSet`, `UWxExecCalc_Damage`, MMC·EffectComponent).
- 데이터 주도 GE·어빌리티(`WxAbilityTableRow`/`WxEffectTableRow`/`WxDamageTableRow` + `UWxAbilitySet`)와 다수의 `WxEffect_*` GE.
- 히트 연출·반응(`UWxHitStopComponent`, GameplayCue, AnimNotify 계열).
- 락온·타게팅(`UWxLockOnComponent`, TargetingSystem 필터/소터 태스크, MotionWarping 스냅).
- 무기 히트박스·투사체·소환물 스폰(`AWxWeaponBase`, `UWxProjectileSubsystem`, `UWxMinionSubsystem`).

**경계 (비담당)**
- 캐릭터 클래스·소유 액터·컨트롤러 정의는 하지 않는다. ASC/AttributeSet를 붙이는 폰과 AI 컨트롤러는 [[WxAI]]와 게임 모듈([[WxGame]])이 소유한다.
- Gameplay Tag 네이티브 선언, `IWxUIData` 등 공용 인터페이스·정의는 [[WxCore]]에 둔다(이 모듈은 참조만).
- 표시(HUD·데미지 플로터 위젯·아이콘 렌더)는 [[WxUI]]가 담당하며, 여기서는 `IWxUIData`로 데이터만 노출한다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 전투용 ASC. 입력 라우팅·어빌리티셋 부여·몽타주 재생의 유일한 라이브 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 Wx 어빌리티의 베이스. 발동 그룹/액션 페이즈로 점유·캔슬을 정의 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxCombatAttributeSet` | HP/SP/GP/MP/UP·ATK/DEF·크리·속도 등 전투 수치와 Meta 데미지 채널 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxAbilitySet` | 어빌리티·GE·어트리뷰트 초기화 행을 한 데이터에셋으로 묶어 서버에서 일괄 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatLibrary` | 적대 판정·데미지 체크·데미지/구간 GE 적용의 공용 함수(BFL) | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |
| `UWxExecCalc_Damage` | 데미지 최종 산출(SP·GP·IncomingDamage 순). 반응은 DamageResponse 컴포넌트로 위임 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffect_Damage.h` |
| `UWxInputBufferComponent` | 발동 실패 입력을 잠시 기억했다 캔슬 창에서 재시도(선입력) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxInputBufferComponent.h` |
| `UWxLockOnComponent` | 겨누는 대상을 SceneComponent 단위로 서버 권위 복제. 플레이어·AI 공용 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnComponent.h` |

## 확장 포인트 / 규약
- 새 어빌리티는 `UWxAbilityBase`(또는 `WxAbility_*` 중 하나)를 상속하고, 쿨다운·코스트는 `AbilityDataRow`(`WxAbilityTableRow`)에서 읽는다. 코스트는 공용 `UWxEffect_Cost`, 쿨다운은 `UWxEffect_Cooldown` 파생 GE가 그 수치를 쓴다. 입력 트리거는 CDO의 `ActivationInputAction`이 쥔다.
- 데이터 주도 설정: `UWxAbilitySet`(에셋)이 부여 어빌리티·GE·`WxCombatAttributeInitTableRow`를 묶어 캐릭터 ASC의 `AbilitySets`에 들어가면 `GiveToAbilitySystem`이 서버에서 부여한다. GE 값은 스펙에 싣지 않고 `UWxEffectComponent_Table`이 지목한 `WxEffectTableRow`를 MMC가 계산 시점에 읽는다(표시 데이터는 `UGameplayEffectUIData` 앵커 경유로 [[WxUI]]가 조회). 공격 수치는 `WxDamageTableRow`.
- 리플리케이션/권한: 스폰(무기·투사체·소환물)과 데미지 적용은 서버 권위. 락온은 소유 클라 예측 후 서버가 재검증 없이 반영하고 전 머신에 복제한다. 히트스톱은 권위/소유 클라는 GE 인스턴스로, 시뮬 프록시는 복제 태그로 판정하며 예측 기각 시 롤백된다.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 활성화 그룹·액션 페이즈가 전투의 캔슬/콤보 규칙 전부를 정한다. 이걸 먼저 잡아야 나머지 어빌리티가 읽힌다.
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력이 어떻게 어빌리티로 흐르고 어빌리티셋이 언제 부여되는지의 허브.
3. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 수치·Meta 채널의 정의. 데미지가 어디로 흘러 HP가 되는지 파이프라인의 종착점.
4. `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` — 데미지/적대 판정 진입점. 무기·투사체·노티파이가 모두 여기로 모인다.

## 관련
- 상위: 전투 폰·AI가 이 모듈의 ASC/AttributeSet를 붙여 쓴다 — [[WxAI]], [[WxGame]].
- 함께: 공용 정의·태그·`IWxUIData`는 [[WxCore]], 전투 표시는 [[WxUI]].

---
*문서 기준 커밋 `81c04f5` · 생성일 2026-09-11 · 소스 178파일 — `/readme-writer`로 갱신*
