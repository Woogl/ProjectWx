---
title: "모듈 경계와 배치 원칙"
category: topic
sources:
  - "raw/notes/2026-09-25-ui-data-interface-removal.md"
  - "raw/notes/2026-09-24-module-principles.md"
  - "raw/notes/2026-09-24-nameplate-manager-wxgame.md"
  - "raw/notes/2026-09-24-device-statetree-cleanup.md"
created: 2026-09-24
updated: 2026-09-25
tags: [wx, architecture, foundation]
aliases: ["모듈화 원칙", "플러그인 경계"]
confidence: medium
volatility: warm
verified: 2026-09-26
summary: "Wx 플러그인은 다른 프로젝트 재사용이 아니라 이 게임 안의 도메인 경계를 지키기 위해 둔다. 코드는 책임이 속한 도메인에 두고, 여러 도메인을 엮는 책임과 도메인 없는 게임 고유 코드는 WxGame에 둔다. 경계를 넘는 정보는 성격에 맞는 통로로 받는다."
---

# 모듈 경계와 배치 원칙

Wx 플러그인은 다른 프로젝트 재사용이 아니라 이 게임 안의 도메인 경계를 지키기 위해 둔다. 코드는 책임이 속한 도메인에 두고, 여러 도메인을 엮는 책임과 도메인 없는 게임 고유 코드는 WxGame에 둔다. 경계를 넘는 정보는 성격에 맞는 통로로 받는다.

2026-09-24 사용자 확정. 다른 도메인의 베이스 클래스가 필요한 코드의 배치는 아직 미결정이다(아래 "미결정" 절).

## 1. 목적

- **목적:** Wx 플러그인은 **이 게임 안의 도메인 경계를 컴파일 단계에서 지키기 위해** 둔다.
  - 도메인끼리 서로 모르고, 의존 방향이 한쪽으로 고정되며, 한 도메인을 고쳐도 다른 도메인은 다시 빌드되지 않는다.
- **재사용은 목적이 아니다.** 다른 프로젝트 재사용은 설계 근거로 쓰지 않는다.
  - WxCore 태그 117개 중 상당수가 이 게임의 규칙이다(`Master.Doppelganger`, `Device.Elevator.*`, `Effect.PerfectGuard` 등). 그래서 도메인 플러그인만 떼어 가도 태그 체계 전체가 따라간다.
  - 실제로 범용인 코드는 Wx 접두사 없는 플러그인으로 분리한다(7절).
- **플러그인 구조는 유지한다.** 플러그인마다 콘텐츠가 0~1개라 사실상 코드 모듈이다. 프로젝트 모듈로 바꿔도 경계는 같고 이동 비용만 생긴다.

## 2. 계층과 의존 방향

```
조립 계층: WxGame(런타임) · WxEditor(에디터) · 기능 단위 플러그인(GameFeature, 도입 시)
  │ 필요한 도메인 모두
  ▼
도메인: WxCombat · WxAI · WxWorld · WxDialogue · WxInventory · WxQuest · WxUI
  │ WxCore와 엔진·외부 플러그인만
  ▼
공용 정의: WxCore
  │ 엔진만

범용 플러그인(DataTableRowFixup·BoxComponentVisualizer)과 WxToolset은 Wx 모듈을 의존하지 않는다.
```

| 계층 | 담는 것 | 담지 않는 것 |
|---|---|---|
| 조립 | 여러 도메인을 엮는 판단·조립, 도메인 없는 게임 고유 코드(캐릭터 이동, 입력, 팀, PlayerState, 치트 등), 도메인 데이터를 VM에 싣는 리졸버 | 한 도메인의 규칙 |
| 도메인 | 그 도메인의 규칙과 구현(컴포넌트·어빌리티·서브시스템·StateTree 노드) | 다른 도메인이나 조립 계층 참조 |
| 표시 도메인(WxUI) | 순수 표시 VM, 화면 레이어·수명, 표시 부품(`AWxIndicator`), MVVM 변환 함수 | 도메인 상태 판단, "누구에게 언제 띄울지" |
| 공용 정의(WxCore) | 게임플레이 태그(한 곳), 인터페이스·구조체·열거형 정의, 여러 도메인이 쓰는 상태 없는 헬퍼(`FWxLocatorUtils`) | 게임플레이 로직, 컴포넌트, 서브시스템, 모듈 클래스 |
| 범용(이름에 Wx 없음) | 프로젝트와 무관한 도구 | Wx 모듈 참조, 게임 규칙 |

금지하는 의존은 셋이다. 도메인 → 다른 도메인, 도메인 → 조립 계층, WxCore → 다른 Wx 모듈. 조립 계층 사이에서도 도메인이 참조하는 중간 계층은 두지 않는다. 도메인 이벤트를 구독하던 통합 플러그인 `WxSave`는 2026-09-01에 삭제됐다.

## 3. 배치: 책임으로 정한다

시험 질문: **"이 코드를 고치게 만드는 기획 변경은 어느 도메인의 것인가?"**

1. **답이 한 도메인이면** 그 도메인에 둔다. 다른 쪽 정보가 필요하면 4절의 통로로 받는다.
2. **답이 여러 도메인이면** WxGame에 둔다. 판단 자체가 여러 도메인의 상태를 엮는 경우다.
3. **답이 없으면** WxGame에 둔다. 어느 도메인에도 속하지 않는 게임 고유 코드다.
4. **참조 모듈 수는 판단 기준이 아니라 점검용이다.** 부모 클래스가 어느 모듈에 있는지도 근거가 아니다. WxGame 클래스가 도메인 베이스를 상속하는 것은 의존 방향에 맞다.

| 사례 | 책임 | 위치 |
|---|---|---|
| `UWxNameplateManagerComponent` | 전투 상태(교전·락온)로 적 위 표시를 정하는 판단. 여러 도메인이다. | WxGame(2026-09-24 이동). WxUI에 두느라 생겼던 질의 델리게이트·마커 컴포넌트·태그 비대칭 조건이 사라졌다. |
| `AWxSpawner` | 스폰과 처치 추적. WxWorld 한 도메인이다. | WxWorld. 적의 처치 통지는 WxCore 계약 `IWxSpawnable`로 받는다. |
| `AWxItemPickup` | 줍기. WxInventory 한 도메인이다. | WxInventory. 상호작용 대상 계약 `IWxInteractable`을 구현한다. |
| `FWxStateTreeTask_PlayMontageOnce` | 장치 트리에서 쓰지만 효과는 캐릭터에게 전투 어빌리티를 발동하는 것이다. WxCombat 한 도메인이다. | WxCombat(2026-09-24 WxWorld `PlayInteractorMontage`를 대체). 장치 당사자는 `Actor.InteractingCharacter` 바인딩으로 받는다. 선례는 WxInventory `GiveRewards`다. |
| 캐릭터 이동·MetaHuman·팀·입력 설정·PlayerState·치트 | 도메인 없음 | WxGame |

## 4. 경계를 넘는 통로: 성격으로 고른다

통로는 한 줄로 선 우선순위가 아니다. 넘기려는 정보의 성격에 맞춰 고른다.

| 넘기려는 것 | 통로 |
|---|---|
| GAS 액터의 상태·효과를 GAS 조건(요구 태그, GE)으로 읽는다. 모든 머신에서 같은 월드 상태다(예: `State.Engaged`). | ASC 게임플레이 태그, GE, Gameplay Event |
| 한 도메인 책임의 코드가 도메인 중립 개념이나 이벤트를 필요로 한다(예: "스폰된 것이 처치됐다"). | WxCore 계약(5절 조건) |
| 판단 자체가 여러 도메인에 걸친다. | 통로 대신 코드를 WxGame에 둔다. 도메인은 자기 델리게이트로 발신만 하고, WxGame이 구독해 잇는다. |
| 위 셋이 모두 맞지 않는다. | 질의 주입(도메인이 질의 델리게이트를 선언하고 WxGame이 바인딩). 최후 수단이다. 읽는 코드에서 값의 출처가 보이지 않고 바인딩 시점에 기댄다. |

보는 사람마다 다른 로컬 상태(예: 내 락온 대상)는 태그로 싣지 않는다. 리슨 호스트에서 권위 ASC와 섞이기 때문이다. 태그 구독이 소비자를 GAS 액터로 묶어 버린다면 계약 통지를 택한다(`IWxSpawnable` 결정).

## 5. WxCore 계약 조건

새 인터페이스·구조체를 WxCore에 둘 때는 셋을 모두 만족해야 한다.

1. **소비자의 책임이 한 도메인 안에 있다.** 그 도메인 규칙을 수행하는 데 도메인 밖의 개념 하나가 필요한 경우다. 소비자의 판단 자체가 여러 도메인에 걸치면 계약이 아니라 WxGame 배치다.
2. **도메인 중립 개념이다.** 제공자 내부 상태를 한 소비자에게 나르는 중계가 아니다. "스폰된 것", "상호작용 대상", "표시 데이터"처럼 게임 공통 개념이어야 한다.
3. **판정은 구현체가 한다.** 소비자가 구현 세부(특정 컴포넌트·태그)를 들여다보지 않는다.

| 계약 | 소비자(책임) | 구현 |
|---|---|---|
| `IWxSpawnable` | WxWorld 스포너(스폰·처치 추적) | WxGame 적 |
| `IWxInteractable` | WxWorld 스캐너(상호작용 탐색·선택), WxGame 상호작용 어빌리티(서버 실행) | WxWorld·WxDialogue·WxInventory·WxGame 액터 |

**표시 연결(2026-09-25):** `IWxUIData`를 제거했다. 어빌리티·GE의 표시 데이터는 WxCombat, 표시 값과 GAS 공통 구독·갱신은 WxUI, 구체 도메인 타입을 읽어 값을 전달하는 리졸버는 WxGame에 둔다. 전역 제공자나 중계 서브시스템을 추가하지 않는다. WxEditor 썸네일은 에디터 조립 계층에서 해당 타입의 아이콘을 직접 읽는다.

**기각 사례:** 락온 대상 질의 계약(2026-09-24). 소비자인 NameplateManager의 판단이 전투와 UI에 걸쳐 있어 조건 1을 채우지 못했고, WxGame 배치로 풀었다. 상호작용 선택(WxWorld 스캐너)이 락온 대상을 읽게 되면, 그때는 상호작용이라는 한 도메인 책임의 소비자가 생기므로 다시 검토한다.

태그는 도메인 전용이어도 `WxGameplayTags` 한 곳에 둔다([WxCore](foundation.md)).

## 6. 조립 계층 추가

- **새 Wx 도메인 플러그인:** 기획상 독립 도메인이고 WxCore만으로 구현할 수 있을 때만 만든다. 기존 도메인과 계약이 계속 필요하면 합치거나 WxGame에 둔다.
- **기능 단위 플러그인(미니게임·사이드미션 등 GameFeature):** 조립 계층으로 둘 수 있다. 여러 도메인을 의존해도 되지만, 도메인이나 WxGame이 이를 참조하지는 않는다.

## 7. 범용 플러그인

프로젝트 규칙과 Wx 모듈을 모르는 코드만 둔다. 이름·식별자에 Wx를 붙이지 않는다. 실제 재사용 수요가 생긴 코드만 이렇게 뽑는다.

## 8. 모듈 이동 절차

1. **리다이렉트 추가:** 클래스를 다른 모듈로 옮기면 `/Script/<모듈>.<클래스>` 경로가 바뀐다. `DefaultEngine.ini`의 `[CoreRedirects]`에 ClassRedirect를 추가한다.
2. **참조 에셋 재저장:**
   - 실행 중인 에디터가 DebugGame이면, DLL 빌드 시각이 소스 변경보다 뒤인지 먼저 본다.
   - 네이티브 기본값이 달라진 속성이 재저장으로 BP 오버라이드가 되어 굳지 않는지 확인한다.
3. **제거 전 확인:** `Content` 문자열 검색을 하고, 리다이렉트 없이 로드·컴파일해 경고가 없는지 본다.
4. **같은 작업 안에서 리다이렉트를 제거한다.** 리다이렉트를 쌓아 두지 않는다.

**예외:** 서브오브젝트의 클래스 자체를 다른 클래스로 바꾸면, 리다이렉트가 한 액터에 같은 역할의 컴포넌트를 둘 만들 수 있다. 이때는 리다이렉트 없이 재저장해 옛 데이터를 버린다.

## 9. 재검토 신호

- **같은 두 도메인 사이에 계약·델리게이트가 반복해서 생긴다.** 경계를 잘못 그었거나 두 도메인이 사실상 하나다.
- **WxGame 코드의 책임이 한 도메인으로 좁아졌다.** 그 도메인으로 내린다. 단, 아래 미결정 사례는 제외한다.
- **다른 프로젝트에서 실제로 쓸 일이 생긴다.** 그 코드만 범용 플러그인으로 뽑는다.

## 미결정: 다른 도메인의 베이스 클래스가 필요한 코드

- **사례:** `UWxAbility_UseItem`과 `UWxAbility_Interact`가 해당한다.
  - 책임은 각각 WxInventory(아이템 사용)와 WxWorld(상호작용)다.
  - 하지만 WxCombat의 `UWxAbilityBase`(ActivationGroup·ActionPhase 등)를 상속해야 해서 WxGame에 있다.
  - 상속은 태그나 계약 같은 통로로 대신할 수 없다.
- **긴장:** 어빌리티 공통 기반이 전투 도메인에 있어서, 전투가 아닌 어빌리티가 WxGame으로 밀려나는 구조다.
- **현재 판단:** 사용자가 "예전부터 고민이 많이 되는 부분"이라며 결정을 미뤘다(2026-09-24). 그동안 현재 배치를 유지하고, 이 유형은 원칙 경계 사례로 표시해 사용자 판단을 받는다.

## 현재 상태

2026-09-24 정적 확인 결과다.
- `*.Build.cs` 기준으로 도메인 플러그인 7개는 모두 Wx 모듈 중 WxCore만 의존한다.
- WxGame은 도메인 7개 모두를 의존하고, WxEditor는 당시 WxCore·WxInventory·WxUI·WxWorld를 의존했다. 2026-09-25 썸네일 직접 조회로 WxCombat·WxGame 의존성을 추가했다. WxToolset과 범용 플러그인은 Wx 모듈을 의존하지 않는다.
- WxGame에서 도메인을 하나 이하로 참조하는 클래스는 8개다. 모두 도메인 없는 게임 고유 코드이거나(이동, MetaHuman, 팀, 입력 설정, PlayerState, 프런트엔드 설정, 모듈) 여러 도메인으로 넓어질 치트다.
- `[CoreRedirects]`는 비어 있다.

실행 검증과는 무관하다.

## 관련 문서

- [[foundation|WxCore — 공용 계약과 설정]] ([WxCore — 공용 계약과 설정](../topics/foundation.md))
- [[game|WxGame — 게임 조립과 실행 흐름]] ([WxGame — 게임 조립과 실행 흐름](../topics/game.md))
- [[ui|WxUI — 화면 레이어와 표시 수명]] ([WxUI — 화면 레이어와 표시 수명](../topics/ui.md))
- [[world|WxWorld — 장치와 상호작용]] ([WxWorld — 장치와 상호작용](../topics/world.md))
- [[editor-tools|편집기 도구 — WxEditor·WxToolset·DataTableRowFixup·BoxComponentVisualizer]] ([편집기 도구 — WxEditor·WxToolset·DataTableRowFixup·BoxComponentVisualizer](../references/editor-tools.md))

## Sources

- [UI 데이터 인터페이스 제거와 리졸버 연결](../../raw/notes/2026-09-25-ui-data-interface-removal.md) — 2026-09-25 사용자 합의와 구현

- [모듈화 목적 재정의와 배치 원칙 정립](../../raw/notes/2026-09-24-module-principles.md) — 사용자 발언, 초안 반증 검토와 확정, 의존 그래프·태그·콘텐츠 현황
- [NameplateManager를 WxGame으로 옮기고 마커·락온 질의를 제거](../../raw/notes/2026-09-24-nameplate-manager-wxgame.md) — 배치와 WxCore 계약 기각 사례
- [장치 StateTree 정리](../../raw/notes/2026-09-24-device-statetree-cleanup.md) — 장치 트리의 다른 도메인 효과 태스크를 효과 도메인에 두는 사례

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-24 HEAD `4c9ee67d3` 기준 정적 확인. 의존 관계는 `*.Build.cs`의 Wx 모듈 이름으로, WxGame 클래스별 참조는 include 헤더가 속한 플러그인으로 대조했다. 에이전트가 정리한 초안을 사용자와 두 차례 검토했다. 초안의 "참조 수 배치", "도메인 둘 이상 참여 조건", "통로 우선순위", "중간 통합 플러그인 전면 금지"는 기존 결정·코드와 어긋나 고쳤다. 사용자는 WxCore 헬퍼 문구를 반영하고 베이스 클래스 사례는 보류했다.

2026-09-24 refresh: 배치 사례 표에 몽타주 1회 재생 태스크(커밋 `f6b4af9d4`, 사용자 제안)를 HEAD `142fab5d6` 코드와 대조해 추가했다. WxCombat이 StateTree를 새로 의존하지만 Wx 모듈 의존은 여전히 WxCore뿐이다.

</details>
