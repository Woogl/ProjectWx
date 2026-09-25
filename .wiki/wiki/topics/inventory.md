---
title: "WxInventory — 아이템 소유와 사용"
category: topic
sources:
  - "raw/notes/2026-09-22-current-inventory.md"
  - "raw/notes/2026-09-22-current-foundation.md"
  - "raw/notes/2026-09-23-item-viewmodel-unification.md"
  - "raw/notes/2026-09-23-interaction-list-vm.md"
  - "raw/notes/2026-09-25-ability-montage-section-model.md"
created: 2026-09-22
updated: 2026-09-25
tags: [wx, inventory]
aliases: ["WxInventory"]
confidence: medium
volatility: warm
verified: 2026-09-26
summary: "WxInventory는 서버에서 아이템 소유·소비·충전을 변경하고 인벤토리와 인스턴스를 복제한다."
---

# WxInventory — 아이템 소유와 사용

WxInventory는 서버에서 아이템 소유·소비·충전을 변경하고 인벤토리와 인스턴스를 복제한다.

## 정의·인스턴스·소유

`UWxItemDefinition`은 정적 데이터, `UWxItemInstance`는 런타임 식별·충전 상태, `UWxInventoryComponent`는 소유와 변경의 중심이다. WxGame의 PlayerController가 인벤토리를 소유한다. 목록은 FastArray이며 아이템 인스턴스는 복제 서브오브젝트로 등록한다. 시작 아이템은 서버 BeginPlay에서 지급한 뒤 준비 통지를 발행한다.

Fragment는 정의에 붙은 공유 객체다. Usable은 사용 효과, Charges는 개별 인스턴스 충전, Stackable은 슬롯당 스택 한도, Pickup은 월드 픽업 데이터, Grade는 등급 표시를 제공한다. 개별 아이템의 가변 상태를 공유 Fragment에 저장하지 않는다.

## 사용 흐름

```mermaid
sequenceDiagram
  participant GA as 사용 어빌리티
  participant Use as ItemUseComponent
  participant Anim as 몽타주 노티파이
  participant Inv as InventoryComponent
  GA->>Use: CanUseItem
  Use->>Inv: CanUseConsumable
  GA->>Use: BeginUseItem(사용 대기)
  Anim->>Use: Event.UseItem
  Use->>Use: 서버·노티파이·메시 오너·대기 검사, 대기 해제
  Use->>Inv: UseConsumable
  Inv->>Inv: 소비 아이템 선택
  Inv->>Inv: 필요한 GE Spec 준비
  Inv->>Inv: 충전 또는 스택 1 차감
  Inv->>Inv: 사용자 Pawn ASC에 효과 적용
```

어떤 소비 아이템을 쓸지는 인벤토리가 고른다(커밋 `1000ea32b`). `UWxAbility_UseItem`의 `ConsumableDef`는 지웠고 `CanUseItemByDef`/`UseItemByDef`는 `CanUseConsumable`/`UseConsumable`로 바뀌었다. 두 함수는 같은 선택 규칙(`FindConsumableInstance`)을 쓴다. 규칙은 목록 순서상 Usable이 있는 첫 인스턴스이며, Charges가 있으면 충전이 남은 것만 고른다. 코드 주석은 소비 아이템이 에스트병 하나뿐이라고 전제한다. Usable 아이템이 둘 이상이면 목록에서 앞선 것이 쓰인다. `UWxAbility_UseItem`은 몽타주 재생 전에 `CanUseItem`으로 보유를 묻고, 노티파이 시점의 소비도 다시 인벤토리가 고른다.

HP 부족량은 `CanUseConsumable`의 조건이 아니다. Charges가 있으면 충전만 감소하고 아이템은 남는다. Charges가 없으면 정의 기준으로 수량 1을 소비한다. 효과가 지정되지 않은 Usable도 소비 경로를 지날 수 있다. GE가 필요하면 ASC·Spec 준비가 먼저지만, 차감 후 효과 적용의 실제 성공을 소비 취소와 연결한 트랜잭션으로 해석하지 않는다.

## 보상과 네트워크 제약

Add/Consume/Use/Refill은 서버 호출 계약이다. 클라이언트 표시와 사용 요청 전달은 별도 경로를 사용한다. StateTree의 GiveRewards/RefillItemCharges는 초기 진입과 비권위 실행을 건너뛰며 0번 PlayerController를 대상으로 한다. 이 구현을 플레이어별 보상 귀속 정책 완성으로 해석하지 않는다.

외부 기능이 이미 아이템을 소비했다면 `UseConsumable`을 다시 호출해 효과만 적용할 수는 없다. 소비가 다시 발생하므로 외부 섭취 결과에 연결하는 설계는 경계를 별도로 정해야 한다.

## 화면 표시와 픽업 문구

WxInventory는 표시 VM을 모른다. WxGame의 PC당 공유 인벤토리 VM이 컴포넌트의 스택·슬롯·충전·목록 변경 통지를 구독해 WxUI 아이템 VM에 값을 공급한다. 클라에서 인벤토리가 늦게 복제되면 등장·제거 통지로 연결만 바꾼다. 퀵슬롯의 사용 요청은 `Ability.UseItem` 어빌리티 발동으로 보낸다. WxGame `UWxAbility_UseItem`이 ItemUseComponent에 사용 대기를 걸므로 위 사용 흐름을 그대로 탄다. 인벤토리의 `RequestUseConsumable`도 같은 태그로 어빌리티를 발동하지만, HEAD C++에 호출처가 없고 UFUNCTION도 아니다. VM 구조와 검증 범위는 [UI](ui.md)의 표시 VM 절에 있다.

픽업의 상호작용 문구는 아이템 데이터의 표시 이름(수량이 2 이상이면 `{0} x{1}`)으로 만든다. 키 표기는 문구에 넣지 않고 상호작용 위젯이 표시한다([월드](world.md)의 문구 출처 원칙).

진입점: [InventoryComponent](../../../Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxInventoryComponent.cpp), [Fragment](../../../Plugins/WxInventory/Source/WxInventory/Public/Items/WxItemFragment.h), [ItemUseComponent](../../../Plugins/WxInventory/Source/WxInventory/Private/Inventory/WxItemUseComponent.cpp). 실제 아이템 에셋·픽업 배선·복제 타이밍은 미검증이다.

## 관련 문서

- [[combat-resources|전투 자원과 현광의 예외]] ([전투 자원과 현광의 예외](../concepts/combat-resources.md))
- [[foundation|WxCore — 공용 계약과 설정]] ([WxCore — 공용 계약과 설정](../topics/foundation.md))
- [[game|WxGame — 게임 조립과 실행 흐름]] ([WxGame — 게임 조립과 실행 흐름](../topics/game.md))
- [[quests|WxQuest — 퀘스트 실행과 저널]] ([WxQuest — 퀘스트 실행과 저널](../topics/quests.md))
- [[ui|WxUI — 화면 레이어와 표시 수명]] ([WxUI — 화면 레이어와 표시 수명](../topics/ui.md))
- [[world|WxWorld — 장치와 상호작용]] ([WxWorld — 장치와 상호작용](../topics/world.md))

## Sources

- [근거 1](../../raw/notes/2026-09-22-current-inventory.md)
- [근거 2](../../raw/notes/2026-09-22-current-foundation.md)
- [아이템 VM 단일화](../../raw/notes/2026-09-23-item-viewmodel-unification.md)
- [상호작용 문구 출처](../../raw/notes/2026-09-23-interaction-list-vm.md)
- [어빌리티 규칙 변경과 몽타주 섹션 모델](../../raw/notes/2026-09-25-ability-montage-section-model.md) — 소비 아이템 선택을 어빌리티에서 인벤토리로 옮김

<details id="document-notes">
<summary>출처·검증 및 참고 정보</summary>

2026-09-22 현재 작업 트리 정적 조사·재편찬. 기준 HEAD `fe8c943f49401326e1007fedd78a937c9e66db47`에 미커밋 문서·도구 변경을 포함하며, 정확한 입력은 출처의 파일별 SHA-256과 발췌 범위로 식별한다. 문서의 `confidence: medium`은 제한된 정적 근거에 대한 표시다. `verified`는 순정 규칙에 따른 편찬일이다.

빌드·게임 실행·멀티플레이·BP/WBP·DataTable·BT/StateTree 바이너리 내부는 이번에 검증하지 않았다. 기획·회의 보고·확정 판단·코드 관찰을 서로 대체하지 않는다.

2026-09-23 refresh: 아이템 VM 단일화(`ba396fc16`)와 픽업 문구의 키 표기 제거(`9556bfc78`)에 따른 표시 연결·픽업 문구 절을 HEAD `7d2a20408` 기준으로 추가했다. 퀵슬롯·획득 표시의 런타임 동작은 확인하지 않았다.

2026-09-25 refresh: 소비 아이템 선택의 인벤토리 이관(`1000ea32b`)에 따른 사용 흐름·함수 이름·선택 규칙을 HEAD `d63ce0630` 코드와 대조해 고쳤고, `GA_Shared_UseItem` 에셋 내부와 아이템 사용의 인게임 동작은 확인하지 않았다.

</details>
