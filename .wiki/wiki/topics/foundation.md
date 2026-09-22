---
title: "WxCore — 공용 계약과 설정"
category: topic
sources:
  - "raw/notes/2026-09-22-current-foundation.md"
created: 2026-09-22
updated: 2026-09-22
tags: [wx, foundation]
aliases: ["WxCore"]
confidence: medium
volatility: warm
summary: "WxCore는 도메인들이 함께 사용하는 태그·상호작용·스폰·표시 계약을 제공한다."
---

# WxCore — 공용 계약과 설정

WxCore는 도메인들이 함께 사용하는 태그·상호작용·스폰·표시 계약을 제공한다.

## 책임과 의존 방향

WxGame은 여러 도메인 플러그인을 조립한다. WxCore는 그 아래에서 GameplayTag와 인터페이스를 제공하며 전투·인벤토리·월드 구현에 의존하지 않는다. 이 분리 덕분에 아이템 픽업은 WxWorld를 참조하지 않고도 상호작용 대상이 된다. 모듈 의존성은 각 `Build.cs`, 활성화·유형은 `Wx.uproject`와 `.uplugin`을 함께 확인한다.

## 공용 계약

| 계약 | 의미와 주의점 |
|---|---|
| `WxGameplayTags` | 네이티브 태그 선언·정의의 단일 위치. 도메인 내부용 태그도 이곳에 둔다. |
| `IWxInteractable` | 액터가 자격·문구·선택지를 제공하고 `OnInteracted`로 실행한다. 선택지의 `Value` 의미는 대상이 정한다. |
| `IWxSpawnable` | 스폰된 액터가 서버에서 처치를 통지한다. 네이티브 델리게이트 계약이므로 BP 단독 구현은 허용하지 않는다. |
| `IWxUIData` | 게임 도메인의 제목·설명·아이콘 등 표시 정보를 UI가 읽는 접점. |
| `FWxLocatorUtils` | 에디터에서 UniversalObjectLocator를 읽기 쉬운 이름으로 표시한다. 런타임 액터 수명을 보장하는 기능은 아니다. |

태그를 `WxGameplayTags.h/.cpp`에 모으는 규칙은 기존 Wiki에 2026-09-21 사용자 확정으로 기록되어 있었으며, 현재 헤더의 명시 규칙과도 일치한다. 이번 재편찬으로 새 결정을 만든 것은 아니다.

## 설정과 실제 적용의 구분

`ECC_WxAttack`은 `ECC_GameTraceChannel1`과 일치해야 한다. `DefaultEngine.ini`의 WxAttack은 Object Channel이며 기본 응답은 Block이다. 캐릭터 생성자는 메시를 Overlap, 캡슐을 Ignore로 지정하지만 `WxCharacterMesh` 프리셋에는 WxAttack Block이 있다. 따라서 모든 BP의 최종 메시 응답을 C++ 생성자만으로 단정할 수 없다.

`DefaultGame.ini`는 `UWxAbilitySystemGlobals`와 GameplayCue 탐색 경로를 등록한다. 아이템은 `WxItemDefinition` PrimaryAssetType으로 `/Game/Item`을 탐색한다. 경로 설정 존재와 개별 에셋의 유효성·쿠킹 성공은 별개다.

## 변경 진입점

- [태그 선언](../../../Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h): 새 태그는 짝 cpp 정의까지 변경한다.
- [상호작용 계약](../../../Plugins/WxCore/Source/WxCore/Public/WxInteractable.h): 선택지 변경은 서버 재검증 경로까지 대조한다.
- [충돌 설정](../../../Config/DefaultEngine.ini): 채널 번호와 프리셋·캐릭터 BP를 함께 확인한다.

## 관련 문서

- [[ai|WxAI — AI 인지와 행동]] ([WxAI — AI 인지와 행동](../topics/ai.md))
- [[combat|WxCombat — 전투 시스템]] ([WxCombat — 전투 시스템](../topics/combat.md))
- [[editor-tools|편집기 도구 — WxEditor·WxToolset·BoxComponentVisualizer]] ([편집기 도구 — WxEditor·WxToolset·BoxComponentVisualizer](../references/editor-tools.md))
- [[game|WxGame — 게임 조립과 실행 흐름]] ([WxGame — 게임 조립과 실행 흐름](../topics/game.md))
- [[ui|WxUI — 화면 레이어와 표시 수명]] ([WxUI — 화면 레이어와 표시 수명](../topics/ui.md))
- [[wiki-operation|WX Wiki 운영과 재생성]] ([WX Wiki 운영과 재생성](../references/wiki-operation.md))
- [[world|WxWorld — 장치와 상호작용]] ([WxWorld — 장치와 상호작용](../topics/world.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-foundation.md)

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. 인간 검증일 `verified`는 새로 부여하지 않았다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>
