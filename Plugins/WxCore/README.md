# WxCore — 공용 정의 (foundation)

> 모든 Wx 플러그인이 공유하는 최하단 정의 계층. Gameplay Tag의 유일한 출처이자, 도메인끼리 서로 직접 의존하지 않도록 계약 인터페이스(상호작용·세이브)와 프로젝트 공용 상수(콜리전 채널)를 제공한다.

## 책임
**담당**
- 프로젝트 전역 Gameplay Tag의 단일 선언처 (`WxGameplayTags`).
- 도메인 간 결합을 끊는 공용 계약 인터페이스: `IWxInteractable`(상호작용 대상), `IWxSavable`(세이브 참여).
- 프로젝트 커스텀 콜리전 채널 상수 (`ECC_WxAttack`).

**경계 (비담당)**
- 인터페이스의 *구현*은 소비 도메인이 한다 — 상호작용 대상은 [[WxWorld]]·[[WxInventory]] 등이, 세이브 라이프사이클 구동은 [[WxSave]]가 담당. WxCore는 계약만 든다.
- 태그를 실제로 부여·소비하는 로직(어빌리티·이펙트·UI)은 [[WxCombat]]·[[WxUI]] 등 각 도메인에 있다. 여기엔 이름만 있다.

## 의존성
- **주요 의존**: 엔진만 — `Core`, `CoreUObject`, `Engine`, `GameplayTags`. foundation답게 Wx 의존이 전무하다.
- 규칙: WxCore는 도메인 DAG 최하단이므로 다른 Wx 플러그인을 참조하면 안 된다. `WxCore.Build.cs` 검증 결과 Wx 의존 없음 — ✅ 준수.

## 핵심 타입 (진입점)
| 타입 | 역할 | 위치 |
| --- | --- | --- |
| `WxGameplayTags` (namespace) | 전 프로젝트 Native Tag 선언부. 태그 추가는 이 파일 + `.cpp`에만 | `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` |
| `IWxInteractable` | 상호작용 대상 계약. 소비 도메인이 WxWorld 의존 없이 자기 액터를 상호작용 대상으로 만드는 접점 | `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` |
| `IWxSavable` | 세이브 슬롯 라이프사이클 참여 마커+후크. WxSave와 소비 도메인을 분리 | `Plugins/WxCore/Source/WxCore/Public/WxSavable.h` |
| `ECC_WxAttack` | 무기·투사체 히트박스 Object Channel 상수. `DefaultEngine.ini` 등록 순서와 일치해야 함 | `Plugins/WxCore/Source/WxCore/Public/WxCollisionChannels.h` |
| `FWxCoreModule` | 모듈 부트스트랩 (Native Tag 등록 등) | `Plugins/WxCore/Source/WxCore/Public/WxCoreModule.h` |

## Gameplay Tags
공용 태그의 출처. 신규 태그는 반드시 여기(+`WxGameplayTags.cpp`)에만 추가한다. 변수명은 점(`.`)을 언더스코어(`_`)로 치환.
- 선언: `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h`
- 주요 네임스페이스:
  - `State.*` — 캐릭터 상태(사망·래그돌·그로기·락온·무적·회피·질주·탈진·피격·슈퍼아머·처형·대화 등). ASC에 부여되는 런타임 상태.
  - `Movement.*` — 이동 상태(`InAir` 체공). CMC가 각 머신에서 토글.
  - `Event.*` — 어빌리티 트리거용 GameplayEvent(피격 반응·회피/퍼펙트가드 성공·아이템 사용·상호작용·처형/백스탭). `Event.HitReact`는 부모 카테고리로, ExecCalc 필터링과 HitReact·Guard 어빌리티의 자식 전체 수신에 쓰인다.
  - `Gimmick.*` — StateTree 기믹 상태 라벨(문·엘리베이터·상자·체크포인트). 코드가 읽지 않고 세이브 슬롯에 담기는 상태 이름.
  - `StateTree.*` — 기믹 ST로 발행되는 상호작용 신호(`StateTree.Interact`).
  - `Quest.*` — 퀘스트 실패 이벤트.
  - `ANS.*` — 애님노티파이 구간 태그(`ComboWindow`).
  - `GameplayCue.*` — 연출 큐(데미지 플로터·타격 임팩트·퍼펙트가드·화상·텔레그래프 색상 등). 타격은 플로터(`Damage`, 서버 권위)와 임팩트(`Hit`, 대미지 GE가 들고 다녀 공격자 클라에서 예측)로 나뉜다.
  - `Damage.*` — 대미지 판정 플래그(치명타·가드불가·패리유발).
  - `Ability.*` — 어빌리티 식별. `Ability.Exclusive`가 차단/캔슬이 지목하는 유일 태그(액션 슬롯 상호배제).
  - `SetByCaller.*` — GameplayEffect SetByCaller 키(Duration·회복량·계수·이동배율 등).
  - `UI.*` — CommonUI 레이어(`Layer.*`)와 액션(`Action.*`) 태그.

## 확장 포인트 / 규약
- 새 태그: `WxGameplayTags.h` 선언 + `WxGameplayTags.cpp` 정의. 다른 곳에서 태그를 만들지 않는다.
- 상호작용 대상 추가: 액터가 직접 `IWxInteractable` 구현(액터 고유 동작), 또는 컴포넌트가 구현(호스트 액터를 순수 BP로 두는 기믹 갈래). 조회는 전부 `IWxInteractable::Find`를 거친다.
- 세이브 참여: `IWxSavable` 구현 + 보존 필드에 `UPROPERTY(SaveGame)` 표시. `GetSaveId()`가 유효한 `FGuid`를 반환해야 슬롯에 포함된다.
- 인터페이스는 `NotBlueprintable`(BP 구현 불가) — BP 액터는 C++ 컴포넌트 갈래로만 계약에 참여한다.

## 여기서부터 읽어라
1. `Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h` — 프로젝트의 상태·이벤트·어빌리티 어휘 전체가 doc-comment와 함께 여기 있다. 시스템 간 계약을 읽는 지도.
2. `Plugins/WxCore/Source/WxCore/Public/WxInteractable.h` — 도메인 분리가 어떻게 이뤄지는지 보여주는 대표 계약. 클라 스캐너/서버 어빌리티가 같은 답에 수렴하는 규약이 주석에 상세하다.

## 관련
- 상위: 모든 도메인 플러그인([[WxCombat]] · [[WxInventory]] · [[WxUI]] · [[WxWorld]] · [[WxAI]] · [[WxDialogue]] · [[WxQuest]] · [[WxSave]])과 게임 모듈 `WxGame`이 WxCore를 참조한다. 역참조는 없다.

---
*문서 기준 커밋 `de46ee7` · 생성일 2026-08-11 · 소스 9파일 — `/readme-writer`로 갱신*
