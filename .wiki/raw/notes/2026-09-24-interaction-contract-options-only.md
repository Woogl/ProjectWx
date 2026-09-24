---
title: "상호작용 계약을 선택지 하나로 통합"
source: "MANUAL"
type: notes
ingested: 2026-09-24
tags: [wx, foundation, world, finisher, architecture]
summary: "IWxInteractable의 CanInteract·GetInteractionPrompt를 없애고 순수 가상 GetInteractionOptions 하나로 자격과 문구를 답한다. 선택지가 비면 지금은 상호작용할 수 없다. 적은 처형 문구를 찾을 때 GetPlayerPawn(0) 대신 넘겨받은 상호작용자를 쓴다."
---

# 상호작용 계약 통합

2026-09-24, HEAD `36fbb4371` 위에서 작업했다. WxCore 모듈 리뷰(`378c0decf`에서 추가, 후속 조치 완료로 삭제)의 `GetInteractionPrompt` 지적에 대한 후속이다. WxEditor Win64 Development 빌드 성공(`Saved/Logs/BuildDoctor/build_2026-09-24_154842_086_18212.log`). 인게임 동작은 확인하지 않았다.

## 사용자 결정

- "전체적으로 점검해서 가장 적절한 해결책을 찾아주세요" 요청에 자격까지 선택지로 합치는 안을 제시했고, 사용자가 "네 진행해주세요"로 승인했다.

## 이전 구조와 문제

- 계약에 `CanInteract`(기본 true), `GetInteractionOptions`(기본은 `GetInteractionPrompt()` 하나), 순수 가상 `GetInteractionPrompt()`가 있었다. `GetInteractionPrompt`를 부르는 곳은 기본 `GetInteractionOptions`뿐이었다.
- `AWxDevice`는 `CanInteract`를 `!Options.IsEmpty()`로 구현해 스캔마다 선택지를 두 번 계산했고, 호출되지 않는 `GetInteractionPrompt`를 순수 가상이라 들고 있었다.
- `AWxEnemyCharacter::GetInteractionPrompt`는 인자가 없어 상호작용자를 `GetPlayerPawn(0)`으로 짐작했다. 서버 검증도 이 함수를 거쳐 데디케이티드 서버에서 다른 플레이어의 처형 어빌리티를 훑었다. 검증은 `Value`만 대조해 오동작은 없었다.

## 변경

- `IWxInteractable`: 순수 가상 `GetInteractionOptions`와 `OnInteracted`만 남았다. 선택지가 비면 지금은 상호작용할 수 없다. `WxInteractable.cpp`는 지웠다.
- 스캐너: 반경 안의 `IWxInteractable` 액터를 모두 후보로 모으고, `UpdateInRange`가 대상마다 선택지를 한 번 모은다. 선택지가 빈 대상은 행이 생기지 않는다. 자격 판정 반복을 막던 `Examined` 목록은 이유가 사라져 `AddUnique`로 바꿨다.
- `UWxAbility_Interact`: `CanInteract` 검사를 지웠다. 선택지 값 대조가 빈 목록을 거절하므로 서버 자격 검증은 유지된다. 거리 검사가 선택지 대조보다 먼저 온다.
- `AWxNpc`: `IsAwaited`일 때만 `AWxDialogueActor`의 선택지를 낸다.
- `AWxDialogueActor`·`AWxItemPickup`: 문구 하나짜리 선택지를 낸다(`Value` 기본값 `INDEX_NONE`). 픽업은 `ItemDef`가 비어도 빈 문구 선택지를 낸다. 누르면 `OnInteracted`가 픽업을 치우는 경로를 보존하기 위해서다.
- `AWxEnemyCharacter`: 자격 조건(적대·생존·몽타주 1회 재생 중 아님·그로기 또는 교전 밖 등 뒤)과 처형 문구 조회를 한 함수로 합쳤다. 상호작용자 ASC는 `UAbilitySystemGlobals::GetAbilitySystemComponentFromActor`로 찾는다. 처형 어빌리티가 없는 상호작용자에게는 선택지를 내지 않는다. 이전에는 빈 문구 행이 뜨고 눌러도 아무 일이 없었다.
- `AWxDevice`: `CanInteract`·`GetInteractionPrompt`를 지웠다. `GetInteractionOptions`는 그대로다.

## 참고

- Lyra `IInteractableTarget`(Lyra 5.7 `Source/LyraGame/Interaction/IInteractableTarget.h`)도 순수 가상 `GatherInteractionOptions` 하나만 두고, 문구·보조 문구·부여 어빌리티·위젯 클래스를 선택지(`FInteractionOption`) 단위로 둔다.
- 비활성(보이지만 누를 수 없는) 선택지가 필요해지면 `FWxInteractionOption`에 필드를 추가하고 `UWxAbility_Interact`의 선택지 대조 조건을 함께 바꿔야 한다. 지금 빈 선택지 목록은 "숨김"이다.
- NPC 자격이 권위 측 등록부(`FWxStateTreeTask_WaitForInteraction::IsAwaited`)에서 파생한다는 기존 전제는 바뀌지 않았다.

근거: [계약](../../../Plugins/WxCore/Source/WxCore/Public/WxInteractable.h), [스캐너](../../../Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp), [상호작용 어빌리티](../../../Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp), [적](../../../Source/WxGame/Character/WxEnemyCharacter.cpp), [NPC](../../../Source/WxGame/Character/WxNpc.cpp), [픽업](../../../Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp), [대화 대상](../../../Plugins/WxDialogue/Source/WxDialogue/Private/WxDialogueActor.cpp), [장치](../../../Plugins/WxWorld/Source/WxWorld/Private/Device/WxDevice.cpp).
