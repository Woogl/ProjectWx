---
title: "상호작용 목록 VM의 행 전체 재생성과 문구 출처 원칙"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, world, ui, static-review]
summary: "스캐너 신호를 OnRowsChanged 하나로 합치고 목록 VM이 신호마다 행 VM을 다시 만드는 구조, 리졸버의 스캐너 연결, 상호작용 문구 출처 원칙에 대한 사용자 결정과 코드 근거. 인게임 동작은 미검증."
revision: 7d2a20408
---

# 상호작용 목록 VM의 행 전체 재생성과 문구 출처 원칙

2026-09-23 HEAD `7d2a20408` 기준 정적 조사와 사용자 결정 기록이다. 구현 커밋은 `f98eef471`(목록 VM), `9556bfc78`(픽업 문구)이다. 작업 상태·빌드·BP 컴파일 근거는 [Workflow Task](../../../.agents/workflow/tasks/interaction-list-vm-simplification.md)에 있다.

## 결정 (사용자 확정)

- 목록 VM(`UWxViewModel_InteractionList`)과 행 VM(`UWxViewModel_Interaction`)의 두 클래스 구조를 유지한다. 목록 VM은 행 VM을 담는 쪽이라 역할이 겹치지 않는다. 행 VM 하나로 합치는 안은 한 클래스가 두 역할을 맡고 모듈 이동과 CoreRedirects 3개가 필요해 기각했다.
- 겹친 대상을 목록으로 보여 주고 휠로 선택하는 동작은 유지한다.
- 리졸버는 유지한다. 엔진 Create Instance는 `NewObject`만 호출하고, 해제 시 `DestroyInstance`는 리졸버에만 불린다(UE 5.8 `MVVMViewClass.cpp`). 리졸버 없이는 스캐너를 넘기지도, VM을 정리하지도 못한다.
- 스캐너 신호를 `OnRowsChanged` 하나로 합친다. 목록 VM은 신호마다 스캐너에서 목록과 선택을 읽어 행 전체를 다시 만든다. 선택 상태를 목록 VM `SelectedIndex`와 행 `bSelected`에 동기화하던 코드가 사라졌다.
- 대가: 선택을 바꿀 때마다 ListView 엔트리가 새로 붙는다. `WBP_Interaction`은 `bSelected`로 이미지 가시성만 바꾸므로 화면상 차이는 없다. 선택 전환 애니메이션을 넣으려면 선택 신호를 다시 나눠야 한다.
- 스캐너 늦은 도착 관찰(`OnAnyScannerReady`)을 제거했다. 이 코드는 Experience 주입(`7e709a764`) 때문에 추가됐는데(`6d3cc56de`), `97eb35f97`에서 스캐너가 `AWxPlayerController` 생성자 컴포넌트로 돌아왔다.
- 문구는 상호작용했을 때 실제로 일어날 행동의 주인이 갖는다. 모든 문구를 StateTree로 옮기는 안은 기각했다.
  - 장치: StateTree
  - 대화 액터: 대화 컴포넌트
  - 픽업: 아이템 데이터(실행 중 스폰되는 드랍이라 StateTree가 맞지 않는다)
  - 피니시: 피니시 어빌리티
- 픽업 문구의 `[F]` 키 표기를 제거했다. 키 표시는 `WBP_Interaction`의 `CommonActionWidget`이 맡는다(`DT_InputActions`의 `Interact` 행).

## 확인한 계약

- 스캐너(`UWxInteractionScannerComponent`)는 기존 대상의 순서를 보존하고 새 후보를 거리순으로 뒤에 붙인다. 행은 대상의 선택지 하나당 하나이며, 선택지 문구·값은 스캔마다 대상에서 다시 읽는다.
- 대상·선택지 값·문구 중 하나라도 달라진 경우에만 행을 교체하고 `OnRowsChanged`를 발행한다. 같은 선택지가 남아 있으면 선택을 잇고, 선택지만 바뀌었으면 같은 대상의 첫 행을 잇는다.
- 선택 변경(`CycleSelection`)도 같은 `OnRowsChanged`로 발행한다. 외곽선은 선택을 소유한 스캐너만 건다.
- 목록 VM은 `Initialize`에서 구독한 뒤 현재 상태로 한 번 채운다. `Deinitialize`에서 구독을 끊는다. `Entries`만 FieldNotify로 노출하고 입력은 `RequestInteract`·`RequestCycle`로 스캐너에 넘긴다.
- 리졸버는 위젯 소유 PC를 Outer로 목록 VM을 만들고, `FindComponentByClass`로 찾은 스캐너를 넘긴다. 스캐너가 없는 PC면 빈 목록으로 남는다. 헤더 주석은 "나중에 주입하는 구조로 바꾸면 늦은 도착 처리가 다시 필요하다"고 적는다.
- 행 VM은 WxUI에 있고 `Prompt`·`bSelected` 필드만 가진다. 만들어진 뒤 바뀌지 않으므로 setter와 cpp가 없다. 엔진은 VM을 붙이는 순간 해당 소스의 바인딩을 모두 실행한다(UE 5.8 `MVVMView.cpp:1074-1076`).
- 목록 VM이 WxGame에 있는 이유: 스캐너(WxWorld)를 직접 들고 구독하므로 WxUI가 아니라 양쪽에 의존할 수 있는 모듈에 둔다.

## 검증 범위

- Task 기록 기준 Development 빌드와 `WBP_InteractionList`·`WBP_Interaction`·`WBP_GameLayout` 컴파일(오류 0, 경고 0)이 성공했다.
- 인게임은 실행하지 않았다. 목록 표시·휠 선택과 외곽선·선택지 실행·다중 선택지 장치·범위 이탈·리스폰 후 동작은 인간 확인 대상이다.

## 코드 근거

| 파일 | 확인 범위 | SHA-256 |
|---|---|---|
| [WxInteractionScannerComponent.h](../../../Plugins/WxWorld/Source/WxWorld/Public/Interaction/WxInteractionScannerComponent.h) | 단일 신호·행 구조·선택 조회 | `23a2d040b76ab834ff165aaca848f0aa4f85a7aaaacf2fb287f01f89566c8750` |
| [WxInteractionScannerComponent.cpp](../../../Plugins/WxWorld/Source/WxWorld/Private/Interaction/WxInteractionScannerComponent.cpp) | `UpdateInRange`·`UpdateSelection`의 변경 판정·선택 복원 | `d82ca7c3691d4fa60766d2026389fc4ab3713d8860a727c42aba3ee20bfd8f6b` |
| [WxViewModel_InteractionList.h](../../../Source/WxGame/MVVM/WxViewModel_InteractionList.h) | 목록 VM·리졸버 계약, 주입 전환 주의 | `e3cba60b5741a833afae1ef9177093b5ee17f6c74c55915f03bcc91bbe545c6e` |
| [WxViewModel_InteractionList.cpp](../../../Source/WxGame/MVVM/WxViewModel_InteractionList.cpp) | 구독·행 재생성·리졸버 연결 | `5363055736470128ea3f1b4813bbfedd8a61ce42ca212c6de02963688a153b84` |
| [WxViewModel_Interaction.h](../../../Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Interaction.h) | 불변 행 VM | `4a01838799cb879c3b60a292fa6188943850b724aeac12252de55a766aadb114` |
| [WxItemPickup.cpp](../../../Plugins/WxInventory/Source/WxInventory/Private/Items/WxItemPickup.cpp) | 픽업 문구 형식 | `1c592c31d6bbc248d4c3fbf3d2d347cfeee9579ca6bd00a7a0eb33bc80ddad51` |
