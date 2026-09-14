# WxCombat — 전투 시스템

> GAS(Gameplay Ability System) 위에 올린 액션 RPG 전투의 뼈대. 어빌리티 발동·입력 라우팅·어트리뷰트·대미지 파이프라인·락온/타겟팅을 책임진다.

## 책임
**담당**
- 어빌리티 발동 모델: 활성화 정책(OnTriggered/OnGiven), 배타 점유 그룹, 발동 중 캔슬 창(Blocking → ComboWindow → Recovery)
- 라이브 입력 라우팅과 선입력 버퍼링
- 전투 어트리뷰트(HP/SP/GP/MP/UP 등)와 대미지·사망·그로기 반영
- 데이터 주도 GameplayEffect(값·표시·쿨다운·코스트)와 대미지 실행 계산
- 락온/타겟팅, 웨폰·투사체 스폰, 미니언, 히트스톱·큐 등 전투 연출 접점

**경계 (비담당)**
- 팀 판정·공용 UI 데이터 계약(IWxUIData) 등 공용 정의 → [[WxCore]]
- 어빌리티를 발동시키는 AI 판단·퍼셉션 타겟 주입 → [[WxAI]]
- 어트리뷰트·이펙트의 화면 표시 → [[WxUI]]

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `UWxAbilityBase` | 모든 어빌리티의 베이스. 활성화 정책·배타 그룹·캔슬 창을 정의 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` |
| `UWxAbilitySystemComponent` | 입력 라우팅·몽타주 재생·배타 점유 조정의 중심 ASC | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` |
| `UWxCombatAttributeSet` | 전투 자원 어트리뷰트와 대미지/사망/그로기 반영 규칙 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` |
| `UWxAbilitySet` | 어빌리티·이펙트·어트리뷰트 초기화를 데이터 주도로 일괄 부여 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySet.h` |
| `UWxInputBufferComponent` | 발동 실패 입력을 기억했다 캔슬 창에서 재시도하는 선입력 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxInputBufferComponent.h` |
| `UWxEffectComponent_Table` | GE에 붙어 값·표시 데이터를 담는 조회 앵커. MMC/ExecCalc가 계산 시점에 읽음 | `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Effect/WxEffectComponent_Table.h` |
| `UWxLockOnComponent` | 캐릭터가 겨누는 대상을 SceneComponent 단위로 복제 보관 | `Plugins/WxCombat/Source/WxCombat/Public/Targeting/WxLockOnComponent.h` |
| `UWxCombatLibrary` | 적대 관계 조회와 타격 Wrapper GE 적용의 공용 진입점 | `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` |

## 확장 포인트 / 규약
- **새 어빌리티**: `UWxAbilityBase`를 상속하고 `EWxAbilityActivationPolicy`/`EWxAbilityActivationGroup`을 선언한다. 입력 키는 CDO의 `ActivationInputAction`이 쥐며(ASC가 아님), `FWxAbilityTableRow`로 쿨다운·코스트·표시를 데이터로 설정한다.
- **어빌리티 부여**: 캐릭터 BP가 `UWxAbilitySet`을 ASC의 AbilitySets에 넣으면 InitAbilitySystem 시점에 서버에서 어빌리티/이펙트/어트리뷰트 초기화가 일괄 적용된다.
- **새 이펙트**: `UGameplayEffect`에 `UWxEffectComponent_Table`을 붙이고 `FWxEffectTableRow`로 값·표시를 저작한다. 값은 스펙에 실리지 않고 MMC/ExecCalc가 계산 시점에 GE 정의에서 컴포넌트를 찾아 행을 읽는다.
- **대미지**: `FWxDamageTableRow`를 `UWxCombatLibrary::ApplyDamage`에 넘기면 `WxEffect_Hit`의 Component가 아군·시체를 거르고 무적·가드를 선판정한다. 회피는 성공 이벤트만 내고 종료한다. 자식 `WxEffect_Damage`는 수치 계산·SP→IncomingDamage→GP 반영·DamageFloater를 담당하고, Wrapper는 실제 자식 실행 결과를 받아 HitReact·가드·반사·추가 GE를 후처리한다. 퍼펙트 가드는 자식에서 반사량만 계산하며 HP·SP·대상 GP를 변경하지 않는다. 반환값은 자식 GE 적용 성공 여부이므로 회피·자식 거부에는 히트스톱이 발생하지 않는다.
- **리플리케이션**: 서버 권위 모델. 락온 대상은 전 머신에 복제되고 소유 클라이언트는 응답성을 위해 로컬 선반영 후 서버에 요청한다.
- **등록**: `UWxAbilitySystemGlobals`를 `DefaultGame.ini`의 `AbilitySystemGlobalsClassName`으로 등록해야 GE 큐가 히트 위치에서 터진다.

## 여기서부터 읽어라
1. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Ability/WxAbilityBase.h` — 활성화 정책/배타 그룹/캔슬 창이 전투 흐름 전체의 상태 모델이라 여기가 출발점이다.
2. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/WxAbilitySystemComponent.h` — 입력이 어떻게 어빌리티로 라우팅되고 배타 점유가 조정되는지.
3. `Plugins/WxCombat/Source/WxCombat/Public/AbilitySystem/Attribute/WxCombatAttributeSet.h` — 자원 정의와 대미지·사망·그로기 반영 규칙.
4. `Plugins/WxCombat/Source/WxCombat/Public/WxCombatLibrary.h` — 대미지/이펙트 적용의 공용 진입점.

## 관련
- 상위: 캐릭터에 ASC/AbilitySet을 장착하는 [[WxGame]], 어빌리티를 발동·타겟을 주입하는 [[WxAI]], 어트리뷰트·이펙트를 표시하는 [[WxUI]]. 공용 정의는 [[WxCore]].

---
*문서 기준 커밋 `eda01fd` · 생성일 2026-09-13 · 소스 181파일 — `/readme-writer`로 갱신*
