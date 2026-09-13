# WxCore — 공용 기반 모듈

> 모든 Wx 플러그인이 공유하는 정의를 한곳에 모은 foundation 모듈이다. 네이티브 게임플레이 태그, 도메인 간 계약 인터페이스, 콜리전 채널 상수를 담아 도메인 플러그인끼리 서로를 참조하지 않고도 같은 언어로 대화하게 만든다.

## 책임
**담당**
- 프로젝트 전역에서 쓰는 C++ 네이티브 게임플레이 태그의 단일 선언처
- 도메인 경계를 가로지르는 계약 인터페이스(상호작용·UI 표시 데이터·소환물)의 정의
- 프로젝트 커스텀 콜리전 채널 상수의 정의(엔진 ini와 짝을 맞춤)

**경계 (비담당)**
- 위 인터페이스의 실제 구현·소비는 각 도메인이 담당한다 — 상호작용 대상은 [[WxWorld]]·[[WxInventory]], UI 표시는 [[WxUI]], 소환물 관리는 [[WxCombat]]·[[WxAI]]
- 태그를 발행·소비하는 로직은 전투([[WxCombat]])·장치([[WxWorld]]) 등 각 도메인에 있다. 여기엔 이름만 산다.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` | 전 프로젝트 네이티브 태그 네임스페이스. 태그 추가는 이 헤더와 짝 cpp에만 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상 계약. 액터가 구현하고 도메인이 소비 | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `IWxUIData` | UI가 그대로 표시하는 데이터(제목·설명·아이콘·충전 수) 계약 | `Plugins/WxCore/Source/WxCore/Public/WxUIData.h` |
| `IWxMinion` | 소환 가능한 액터 계약. 종류별 상한·어그로 무시 여부 선언 | `Plugins/WxCore/Source/WxCore/Public/Minion/WxMinion.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스가 쓰는 오브젝트 채널 상수 | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxLocatorUtils` | 저작 도구용 UOL 표시명 헬퍼(에디터 전용) | `Plugins/WxCore/Source/WxCore/Public/WxLocatorUtils.h` |

## Gameplay Tags
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` (정의는 짝 cpp `Private/WxGameplayTags.cpp`)
- `State.*` — 락온·교전·대화·소환물 보유·래그돌 등 ASC에 붙는 상태
- `Effect.*` — 무적·가드·퍼펙트가드·탈진·슈퍼아머·히트스톱 등 GE가 부여하는 태그
- `Event.*` — 피격·처형·아이템 사용·장치 트리거 등 어빌리티 트리거용 게임플레이 이벤트
- `Ability.*` / `Cooldown.*` — 어빌리티 식별 태그(활성 표식 겸용)와 짝 쿨다운 태그
- `Damage.*` / `HitReact.*` — 대미지 표식·판정 결과와 피격 반응 종류
- `Device.*` — 장치 State Tree 상태값(코드가 읽지 않고 태그만 여기서 정의)
- `GameplayCue.*` / `UI.*` / `SetByCaller.*` — 큐, HUD 레이어·CommonUI 액션, GE 매그니튜드 키

## 확장 포인트 / 규약
- 새 태그: `WxGameplayTags.h`에 `UE_DECLARE_...EXTERN`, `WxGameplayTags.cpp`에 `UE_DEFINE_...` 한 쌍만 추가. 다른 파일에서 선언하지 않는다.
- 계약 인터페이스는 액터/데이터 소유자가 구현하고, 소비 도메인은 WxCore만 참조해 조회한다 — 이 덕에 도메인 플러그인끼리 직접 의존이 생기지 않는다.
- `ECC_WxAttack`은 `DefaultEngine.ini`의 채널 등록 값과 반드시 일치해야 한다(값을 바꾸면 모든 히트 판정이 다른 채널을 가리킴).
- `FWxLocatorUtils`와 `UniversalObjectLocator` 의존은 `WITH_EDITOR`/`bBuildEditor`로만 컴파일된다.

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 태그 doc-comment가 전투·상호작용·UI의 제어 흐름을 요약한 사실상의 시스템 색인이다.
2. `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` — 계약이 왜 WxCore에 사는지(도메인 간 의존 회피)를 보여주는 대표 사례.

## 관련
- 상위: 모든 Wx 도메인 플러그인([[WxCombat]] · [[WxWorld]] · [[WxUI]] · [[WxInventory]] · [[WxAI]] · [[WxDialogue]] · [[WxQuest]])과 게임 모듈 WxGame이 이 모듈을 참조한다. WxCore는 어떤 Wx 플러그인도 참조하지 않는다.

---
*문서 기준 커밋 `eda01fd` · 생성일 2026-09-13 · 소스 13파일 — `/readme-writer`로 갱신*
