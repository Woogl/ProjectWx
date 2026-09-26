---
type: source
title: "결정 노트 - 2026-09-24-interaction-contract-options-only"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "결정-노트"
  - "상호작용"
  - "구조"
summary: "IWxInteractable의 CanInteract·GetInteractionPrompt를 없애고 GetInteractionOptions 하나로 자격과 문구를 답하게 한 상호작용 계약 통합 기록."
source_type: decision-note
source_id: src-3337ab1368bb988640ac
sha256: 048571ff8d3f592c29b21891c6a77480d8bea2c743950adcbb8cbd727931f871
authority: primary
independence_key: ".wiki/raw/notes/2026-09-24-interaction-contract-options-only.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".wiki/raw/notes/2026-09-24-interaction-contract-options-only.md"
raw_copy: ".raw/captured/048571ff8d3f592c29b21891c6a77480d8bea2c743950adcbb8cbd727931f871.md"
claim_ids:
  - clm-8b25963986-c1
  - clm-8b25963986-c2
  - clm-8b25963986-c3
  - clm-8b25963986-c4
key_claims:
  - "2026-09-24 이후 IWxInteractable 계약에는 순수 가상 GetInteractionOptions와 OnInteracted만 남고 빈 선택지 목록은 상호작용 불가를 뜻한다."
  - "AWxEnemyCharacter는 처형 문구를 찾을 때 GetPlayerPawn(0) 대신 넘겨받은 상호작용자의 ASC를 쓰고 처형 어빌리티가 없는 상호작용자에게 선택지를 내지 않는다."
  - "UWxAbility_Interact는 CanInteract 검사 없이 선택지 값 대조로 서버 자격 검증을 유지한다."
  - "상호작용 계약 통합은 WxEditor 빌드만 통과했고 인게임 동작은 확인하지 않았다."
---

# 결정 노트 - 2026-09-24-interaction-contract-options-only

- 원본: `.wiki/raw/notes/2026-09-24-interaction-contract-options-only.md`
- 원자료 사본: `.raw/captured/048571ff8d3f592c29b21891c6a77480d8bea2c743950adcbb8cbd727931f871.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

- 원자료: 옛 LLM Wiki `.wiki/raw/notes/2026-09-24-interaction-contract-options-only.md` (source: MANUAL, ingested: 2026-09-24).
- 2026-09-24 HEAD `36fbb4371` 위의 작업 기록으로, WxCore 모듈 리뷰의 `GetInteractionPrompt` 지적에 대한 후속이다. 노트 날짜 기준이라 현재 코드와 다를 수 있다.
- WxEditor Win64 Development 빌드 성공(`Saved/Logs/BuildDoctor/build_2026-09-24_154842_086_18212.log`). 인게임 동작은 확인하지 않았다.

## 사람의 판단 원문

> 사용자 2026-09-24: "전체적으로 점검해서 가장 적절한 해결책을 찾아주세요"

- 에이전트가 자격까지 선택지로 합치는 안을 제시했다.

> 사용자 2026-09-24: "네 진행해주세요"

## 이전 구조와 문제(구현 관찰)

- 계약에 `CanInteract`(기본 true), `GetInteractionOptions`(기본은 `GetInteractionPrompt()` 하나), 순수 가상 `GetInteractionPrompt()`가 있었고, `GetInteractionPrompt`를 부르는 곳은 기본 `GetInteractionOptions`뿐이었다.
- `AWxDevice`는 `CanInteract`를 `!Options.IsEmpty()`로 구현해 스캔마다 선택지를 두 번 계산했다.
- `AWxEnemyCharacter::GetInteractionPrompt`는 인자가 없어 상호작용자를 `GetPlayerPawn(0)`으로 짐작했다. 서버 검증은 `Value`만 대조해 오동작은 없었다.

## 변경(구현 관찰)

- `IWxInteractable`: 순수 가상 `GetInteractionOptions`와 `OnInteracted`만 남았다. 선택지가 비면 지금은 상호작용할 수 없다. `WxInteractable.cpp` 삭제.
- 스캐너: 반경 안 `IWxInteractable` 액터를 모두 후보로 모으고 `UpdateInRange`가 대상마다 선택지를 한 번 모은다. 빈 대상은 행이 생기지 않는다.
- `UWxAbility_Interact`: `CanInteract` 검사를 지웠다. 선택지 값 대조가 빈 목록을 거절하므로 서버 자격 검증은 유지되며, 거리 검사가 먼저 온다.
- `AWxNpc`: `IsAwaited`일 때만 `AWxDialogueActor`의 선택지를 낸다.
- `AWxDialogueActor`·`AWxItemPickup`: 문구 하나짜리 선택지(`Value` 기본값 `INDEX_NONE`). 픽업은 `ItemDef`가 비어도 빈 문구 선택지를 내 `OnInteracted`로 치우는 경로를 보존한다.
- `AWxEnemyCharacter`: 자격 조건(적대·생존·몽타주 1회 재생 중 아님·그로기 또는 교전 밖 등 뒤)과 처형 문구 조회를 한 함수로 합쳤고, 상호작용자 ASC는 `UAbilitySystemGlobals::GetAbilitySystemComponentFromActor`로 찾는다. 처형 어빌리티가 없는 상호작용자에게는 선택지를 내지 않는다(이전에는 빈 문구 행이 떴다).

## 참고와 남은 전제

- Lyra `IInteractableTarget`도 순수 가상 `GatherInteractionOptions` 하나만 두고 선택지(`FInteractionOption`) 단위로 문구 등을 둔다.
- 비활성(보이지만 누를 수 없는) 선택지가 필요해지면 `FWxInteractionOption`에 필드를 추가하고 `UWxAbility_Interact`의 대조 조건을 함께 바꿔야 한다. 지금 빈 선택지 목록은 "숨김"이다.
- NPC 자격이 권위 측 등록부(`FWxStateTreeTask_WaitForInteraction::IsAwaited`)에서 파생한다는 전제는 바뀌지 않았다.

## 검증 범위

- 빌드 성공만 확인했고 인게임 상호작용 동작은 확인하지 않았다.

## 관련 주제

- [[상호작용과 장치]]
- [[그로기·경직·피니시]]
- [[모듈 구조와 코드 정리]]

## 핵심 주장

- 2026-09-24 이후 IWxInteractable 계약에는 순수 가상 GetInteractionOptions와 OnInteracted만 남고 빈 선택지 목록은 상호작용 불가를 뜻한다. ^c1
- AWxEnemyCharacter는 처형 문구를 찾을 때 GetPlayerPawn(0) 대신 넘겨받은 상호작용자의 ASC를 쓰고 처형 어빌리티가 없는 상호작용자에게 선택지를 내지 않는다. ^c2
- UWxAbility_Interact는 CanInteract 검사 없이 선택지 값 대조로 서버 자격 검증을 유지한다. ^c3
- 상호작용 계약 통합은 WxEditor 빌드만 통과했고 인게임 동작은 확인하지 않았다. ^c4
