---
title: "WxCore — 공용 계약과 설정"
category: topic
sources:
  - "raw/notes/2026-09-22-current-foundation.md"
  - "raw/notes/2026-09-22-current-core-support.md"
  - "raw/notes/2026-09-22-current-game.md"
  - "raw/notes/2026-09-22-current-ui.md"
  - "raw/notes/2026-09-24-wxcore-cleanup.md"
  - "raw/notes/2026-09-24-interaction-contract-options-only.md"
created: 2026-09-22
updated: 2026-09-24
tags: [wx, foundation]
aliases: ["WxCore"]
confidence: medium
volatility: warm
verified: 2026-09-24
summary: "WxCore는 도메인들이 함께 사용하는 태그·상호작용·스폰·표시 계약을 제공한다."
---

# WxCore — 공용 계약과 설정

WxCore는 도메인들이 함께 사용하는 태그·상호작용·스폰·표시 계약을 제공한다.

## 책임과 의존 방향

WxGame은 여러 도메인 플러그인을 조립한다. WxCore는 그 아래에서 GameplayTag와 인터페이스를 제공하며 전투·인벤토리·월드 구현에 의존하지 않는다. WxCore는 순수 정의만 두고 게임플레이 로직은 구현하지 않는다(2026-09-24 사용자 확정). 그래서 모듈 클래스도 없이 엔진 기본 `FDefaultModuleImpl`로 등록한다. 이 분리 덕분에 `AWxItemPickup`은 WxCore의 `IWxInteractable`을 직접 구현해 WxWorld를 참조하지 않고도 상호작용 대상이 된다. 모듈 의존성은 각 `Build.cs`, 활성화·유형은 `Wx.uproject`와 `.uplugin`을 함께 확인한다. WxCore에 새 계약을 둘 조건과 모듈 배치 규칙은 [모듈 경계와 배치 원칙](modules.md)에 있다.

## 공용 계약

| 계약 | 의미와 주의점 |
|---|---|
| `WxGameplayTags` | 네이티브 태그의 단일 위치. 선언은 `WxGameplayTags.h`, 정의는 `WxGameplayTags.cpp`의 `UE_DEFINE_GAMEPLAY_TAG`에 둔다. 도메인 내부용 태그도 이곳에 둔다. |
| `IWxInteractable` | 액터가 선택지(문구·값)를 내고 `OnInteracted`로 실행한다. 선택지가 비면 지금은 상호작용할 수 없다는 뜻이며, 스캐너 표시와 서버 검증이 같은 답을 받는다. 선택지의 `Value` 의미는 대상이 정한다. |
| `IWxSpawnable` | 스폰된 액터가 서버에서 처치를 통지한다. 네이티브 델리게이트 계약이므로 BP 단독 구현은 허용하지 않는다. |
| `IWxUIData` | 게임 도메인의 제목·설명·아이콘 등 표시 정보를 UI가 읽는 접점. |
| `FWxLocatorUtils` | 에디터에서 UniversalObjectLocator를 읽기 쉬운 이름으로 표시한다. 해석되면 액터 라벨, 미해석이면 경로 끝 이름, 비어 있으면 unset이다. 함수는 `WITH_EDITOR`에서만 존재하며 런타임 액터 수명을 보장하는 기능이 아니다. |

태그를 `WxGameplayTags.h/.cpp`에 모으는 규칙은 기존 Wiki에 2026-09-21 사용자 확정으로 기록되어 있었으며, 현재 헤더의 명시 규칙과도 일치한다. 이번 재편찬으로 새 결정을 만든 것은 아니다.

## 설정과 실제 적용의 구분

`ECC_WxAttack`은 `ECC_GameTraceChannel1`과 일치해야 한다. `DefaultEngine.ini`의 WxAttack은 Object Channel이며 기본 응답은 Block이다. 캐릭터 생성자는 메시를 Overlap, 캡슐을 Ignore로 지정하지만 `WxCharacterMesh` 프리셋에는 WxAttack Block이 있다. 따라서 모든 BP의 최종 메시 응답을 C++ 생성자만으로 단정할 수 없다.

`DefaultGame.ini`는 `UWxAbilitySystemGlobals`와 GameplayCue 탐색 경로를 등록한다. 아이템은 `WxItemDefinition` PrimaryAssetType으로 `/Game/Item`을 탐색한다. 경로 설정 존재와 개별 에셋의 유효성·쿠킹 성공은 별개다.

## 변경 진입점

- [태그 선언](../../../Plugins/WxCore/Source/WxCore/Public/WxGameplayTags.h): 새 태그는 짝 cpp 정의까지 변경한다.
- [상호작용 계약](../../../Plugins/WxCore/Source/WxCore/Public/WxInteractable.h): 선택지 변경은 서버 재검증 경로까지 대조한다.
- [충돌 설정](../../../Config/DefaultEngine.ini): 채널 번호와 프리셋·캐릭터 BP를 함께 확인한다.

## 관련 문서

- [[modules|모듈 경계와 배치 원칙]] ([모듈 경계와 배치 원칙](../topics/modules.md))
- [[ai|WxAI — AI 인지와 행동]] ([WxAI — AI 인지와 행동](../topics/ai.md))
- [[combat|WxCombat — 전투 시스템]] ([WxCombat — 전투 시스템](../topics/combat.md))
- [[dialogue|WxDialogue — 대화 세션]] ([WxDialogue — 대화 세션](../topics/dialogue.md))
- [[editor-tools|편집기 도구 — WxEditor·WxToolset·DataTableRowFixup·BoxComponentVisualizer]] ([편집기 도구 — WxEditor·WxToolset·DataTableRowFixup·BoxComponentVisualizer](../references/editor-tools.md))
- [[game|WxGame — 게임 조립과 실행 흐름]] ([WxGame — 게임 조립과 실행 흐름](../topics/game.md))
- [[inventory|WxInventory — 아이템 소유와 사용]] ([WxInventory — 아이템 소유와 사용](../topics/inventory.md))
- [[ui|WxUI — 화면 레이어와 표시 수명]] ([WxUI — 화면 레이어와 표시 수명](../topics/ui.md))
- [[wiki-operation|WX Wiki 운영과 재생성]] ([WX Wiki 운영과 재생성](../references/wiki-operation.md))
- [[world|WxWorld — 장치와 상호작용]] ([WxWorld — 장치와 상호작용](../topics/world.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-foundation.md)
- [WxCore 태그 정의·로케이터 표시·픽업 상호작용 조사](../../raw/notes/2026-09-22-current-core-support.md) — 태그 정의부·로케이터 헬퍼·픽업 구현
- [게임 조립·새 게임·부활 정적 조사](../../raw/notes/2026-09-22-current-game.md) — 캐릭터 충돌 응답
- [레이어·대화 화면·HUD·속성 표시 수명 조사](../../raw/notes/2026-09-22-current-ui.md) — `IWxUIData`
- [WxCore 정리: 쓰지 않는 태그·모듈 클래스 제거](../../raw/notes/2026-09-24-wxcore-cleanup.md) — 순수 정의 원칙·`FDefaultModuleImpl`

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 태그 정의부·로케이터·픽업 설명은 HEAD `60c324c714b1dab10cd48d36cabad63ace232716` 기준으로 보강했다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다. 2026-09-24 refresh에서 충돌 채널·프리셋·`DefaultGame.ini` 등록·공용 계약 헤더를 HEAD `d76e48717`의 설정·코드와 다시 대조했다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

</details>
