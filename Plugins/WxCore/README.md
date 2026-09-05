# WxCore — 공용 정의 (foundation)

> 모든 Wx 플러그인이 공유하는 최하단 정의 모듈. Native Gameplay Tag, 콜리전 채널, 상호작용·UI 데이터 계약 같은 도메인 간 공용 어휘를 한곳에 모아, 도메인 플러그인끼리 서로를 참조하지 않고도 같은 언어로 대화하게 한다.

## 책임
**담당**
- 프로젝트 전역 Native Gameplay Tag 선언 (`WxGameplayTags`)
- 커스텀 콜리전 채널 상수 (`ECC_WxAttack`)
- 도메인 횡단 인터페이스 계약: 상호작용(`IWxInteractable`), UI 표시 데이터(`IWxUIData`)
- 저작 도구용 로케이터 표시 헬퍼 (`FWxLocatorUtils`, 에디터 전용)

**경계 (비담당)**
- 태그를 소비하는 실제 시스템 로직은 각 도메인에 위임 — 전투는 [[WxCombat]], 상호작용 발동은 [[WxWorld]], UI 표시는 [[WxUI]], 인벤토리는 [[WxInventory]]
- WxCore는 정의만 두며 게임플레이 동작을 구현하지 않는다

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` | 전역 Native Tag 네임스페이스 — 태그 추가는 이 헤더와 짝 cpp에만 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상 계약 (액터가 구현) | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `IWxUIData` | UI가 그대로 표시하는 데이터 계약 (아이콘/이름/설명/충전) | `Plugins/WxCore/Source/WxCore/Public/WxUIData.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스 Object Channel 상수 | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxLocatorUtils` | 로케이터 표시명 헬퍼 (에디터 전용) | `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h` |

## Gameplay Tags
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` / 정의: `Plugins/WxCore/Source/WxCore/Private/WxGameplayTags.cpp`
- 주요 네임스페이스:
  - `State.*` — 락온/교전/대화/소환 등 ASC에 붙는 상태
  - `Effect.*` — GE가 부여하는 태그 (무적·가드감쇠·퍼펙트가드·탈진·슈퍼아머·히트스톱)
  - `Event.*` — 대미지 파이프라인·처형·아이템·장치 트리거가 ASC에 보내는 게임플레이 이벤트
  - `Ability.*` — 어빌리티 식별 태그 (플레이어 공격/스킬/궁극, 적 Pattern, 공용 HitReact/Groggy/Death)
  - `Cooldown.*` — 어빌리티별 쿨다운 GE가 부여
  - `Damage.*` / `HitReact.*` — 대미지 판정 결과·피격 반응 종류
  - `Device.*` — 장치 State Tree 상태값 (버튼/문/엘리베이터/상자/체크포인트/피스톤)
  - `GameplayCue.*` — 연출 큐 / `SetByCaller.*` — GE 매그니튜드 채널 / `UI.*` — CommonUI 레이어·액션

## 확장 포인트 / 규약
- 새 태그: `WxGameplayTags.h`에 `UE_DECLARE_..._EXTERN`, `.cpp`에 `UE_DEFINE_...` — 두 파일에만 손댄다
- 상호작용 대상 만들기: 액터가 `IWxInteractable` 구현 (컴포넌트 아님, 대상당 구현체 하나). 감지·사거리는 쿼리 콜리전 형상 위에서 돈다
- UI에 데이터 노출: 저작 데이터를 쥔 객체가 `IWxUIData` 구현
- 콜리전 채널 상수는 `DefaultEngine.ini`의 채널 등록 순서와 반드시 일치

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 프로젝트 전역 어휘 사전. 다른 모듈을 읽기 전 태그 구조부터 파악하면 시스템 간 계약이 보인다
2. `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` — 도메인이 WxWorld에 의존하지 않고 상호작용 대상이 되는 방식 (계약을 foundation에 둔 이유)
3. `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` — 피격 판정 콜리전 규약

## 관련
- 상위: 모든 Wx 플러그인이 WxCore에 의존한다 — [[WxCombat]], [[WxInventory]], [[WxUI]], [[WxWorld]], [[WxAI]], [[WxDialogue]], [[WxQuest]] 및 게임 모듈 WxGame

---
*문서 기준 커밋 `f826b21` · 생성일 2026-09-05 · 소스 11파일 — `/readme-writer`로 갱신*
