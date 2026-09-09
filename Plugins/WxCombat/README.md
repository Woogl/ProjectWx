# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 액션 RPG 전투를 구축한다. 어빌리티·속성·이펙트·큐부터 락온, 히트스톱, 콤보 선입력, 무기 히트박스, 투사체, 소환물, 마무리 일격까지 실제 전투 루프 전반을 담당한다.

## 책임
**담당**
- GAS 래핑: ASC(`UWxAbilitySystemComponent`), 속성(`UWxCombatAttributeSet`), 어빌리티/이펙트/큐 계층, 어빌리티 부여용 DataAsset(`UWxAbilitySet`).
- 입력 → 발동 라우팅과 선입력 버퍼(`UWxInputBufferComponent`), 콤보 창·후딜 창 재시도.
- 락온/타게팅(TargetingSystem 필터·소터 태스크), 히트스톱·슬로타임 등 타격감 연출.
- 무기 히트박스 판정(`AWxWeaponBase`), 투사체·소환물의 서버 권위 스폰, 마무리 일격 피해.
- 전투 구동 AnimNotify 계열(피해 적용, 몽타주 이벤트 송출, 스폰 명령 등).

**경계 (비담당)**
- 공용 타입·베이스 정의는 [[WxCore]]에 둔다(예: `IWxUIData`, `WxUIData`).
- AI 의사결정·퍼셉션은 [[WxAI]]가 담당한다. 락온 컴포넌트는 AI가 채운 타겟을 받기만 한다.
- HUD·데미지 플로터 등 위젯의 시각 표현은 [[WxUI]] 쪽. 이 모듈은 큐/이벤트 발생까지만 책임진다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilitySystemComponent` | 라이브 입력 라우팅의 유일한 진입점, 몽타주/애니메이션 정책 승격 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxAbilityBase` | 모든 전투 어빌리티의 베이스. 발동 정책·그룹·페이즈·비용 규약을 정의 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySet` | 캐릭터 BP가 ASC에 넣어 어빌리티·이펙트·속성 초기화를 일괄 부여하는 DataAsset | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxCombatAttributeSet` | HP/SP/GP/MP/UP·ATK/DEF 등 전투 속성의 단일 정의처 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxInputBufferComponent` | 발동 실패 입력을 기억했다 캔슬 창에서 재시도(선입력) | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxInputBufferComponent.h` |
| `UWxLockOnComponent` | 겨누는 대상을 SceneComponent 단위로 보유, 타게팅 계약의 축 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnComponent.h` |
| `UWxMinionSubsystem` | 소환물을 서버 권위로 생성·주인별 관리 | `Plugins/WxCombat/Source/WxCombat/Public/Minion/WxMinionSubsystem.h` |
| `UWxCombatLibrary` | BP/코드 공용 전투 헬퍼 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |

## 확장 포인트 / 규약
- 새 어빌리티: `UWxAbilityBase`를 상속(또는 `WxAbility_Attack/Skill/Dodge/Guard/...` 등 기존 특화 클래스 활용)하고, `FWxAbilityTableRow`(`Public/AbilitySystem/Ability/WxAbilityTableRow.h`)로 데이터화한 뒤 `UWxAbilitySet`의 `GrantedAbilities`에 등록한다. 입력 라우팅 키는 각 어빌리티 CDO의 ActivationInputAction이 쥔다.
- 새 이펙트: `Public/AbilitySystem/Effect/`의 `WxEffect_*`(GE) 계열을 추가한다. 값 주도 설정은 `UWxEffectComponent_Table`이 `FWxEffectTableRow`를 지목하는 방식으로, 피해 반영 후 처리는 `UWxEffectComponent_DamageResponse`가 맡는다.
- 데이터 주도: `UWxAbilitySet`(DataAsset)이 부여를 구동하고, 속성 초기화는 `FWxCombatAttributeInitTableRow`, 피해 수치는 `FWxDamageTableRow`, 이펙트/어빌리티는 `FWxEffectTableRow`/`FWxAbilityTableRow`가 DataTable Row로 공급한다.
- 리플리케이션/권한: 어빌리티·이펙트 부여(`GiveToAbilitySystem`), 소환물·투사체 스폰(`UWxMinionSubsystem`/`UWxProjectileSubsystem`)은 서버 권위다.
- 등록 필수: `UWxAbilitySystemGlobals`를 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`으로 등록해야 큐 발생 위치가 히트 결과로 채워진다(누락 시 큐가 원점에서 터진다).

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` — 캐릭터가 어떤 어빌리티/이펙트/속성을 갖는지 부여 흐름의 시작점.
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 발동 정책·그룹·페이즈 규약. 개별 어빌리티를 읽기 전 공통 계약을 잡는다.
3. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` + `WxInputBufferComponent.h` — 입력이 어떻게 발동/버퍼로 이어지는지 제어 흐름.
4. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 전투 수치 모델(약어 정의 포함).

## 관련
- 상위: 캐릭터/컨트롤러가 `UWxAbilitySet`으로 이 모듈을 구동한다. AI 연동은 [[WxAI]], UI 표현은 [[WxUI]], 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `e0e3ecc` · 생성일 2026-09-09 · 소스 176파일 — `/readme-writer`로 갱신*
