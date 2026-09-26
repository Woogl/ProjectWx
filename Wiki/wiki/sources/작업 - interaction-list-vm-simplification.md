---
type: source
title: "작업 - interaction-list-vm-simplification"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "작업-기록"
  - "UI"
  - "상호작용"
  - "MVVM"
summary: "상호작용 목록 VM을 유지하되 스캐너 신호를 OnRowsChanged 하나로 합쳐 단순화하고, 문구 출처 기준과 엘리베이터 탑승칸 버튼 잠금까지 정리한 완료 작업 기록"
source_type: task-record
source_id: src-1730cb541df1643b7273
sha256: 3b605e70920f90d97e6b75d9a3fabbb5517c9490689a1b42759890de9d2873f7
authority: primary
independence_key: ".agents/workflow/tasks/interaction-list-vm-simplification.md"
review_state: active
refresh_due: 2026-10-26
original_paths:
  - ".agents/workflow/tasks/interaction-list-vm-simplification.md"
raw_copy: ".raw/captured/3b605e70920f90d97e6b75d9a3fabbb5517c9490689a1b42759890de9d2873f7.md"
claim_ids:
  - clm-80c9cf979d-c1
  - clm-80c9cf979d-c2
  - clm-80c9cf979d-c3
  - clm-80c9cf979d-c4
key_claims:
  - "상호작용 스캐너는 목록 변경과 선택 변경 신호를 OnRowsChanged 하나로 합쳤고, 목록 VM은 신호마다 행 전체를 다시 만든다."
  - "상호작용 문구는 실제로 일어날 행동의 주인이 가진다는 기준에 따라 Device는 StateTree, 픽업은 아이템 데이터, 피니시는 피니시 어빌리티가 문구를 가진다."
  - "StateTree 인스턴스 구조체의 FText에 C++ 기본값을 두면 값이 기본값과 같을 때 에셋 저장이 FortniteMain 커스텀 버전 불일치로 실패했다."
  - "상호작용 목록 VM 단순화와 엘리베이터 버튼 잠금은 2026-09-25 이우성이 인게임 5개 항목과 코드 리뷰를 모두 통과로 확인했다."
---

# 작업 - interaction-list-vm-simplification

- 원본: `.agents/workflow/tasks/interaction-list-vm-simplification.md`
- 원자료 사본: `.raw/captured/3b605e70920f90d97e6b75d9a3fabbb5517c9490689a1b42759890de9d2873f7.md` (수집 2026-09-26, 재확인 기한 2026-10-26)

## 개요

상호작용 목록 VM(`UWxViewModel_InteractionList`)과 행 VM(`UWxViewModel_Interaction`)의 역할 중복 문제를 검토해 구조는 유지하고 선택 동기화 코드를 없앤 작업이다. 이어서 상호작용 문구의 출처 기준을 정하고, 엘리베이터 탑승칸 버튼 잠금 문제를 고쳤다. 상태는 완료이며 테스트 체크리스트 6/6이 사람 확인으로 통과했다.

## 확정 결정 (2026-09-23 사용자 확정)

- 목록 VM 클래스는 유지한다. 목록 VM은 행 VM을 담는 쪽이라 역할이 겹치지 않는다.
- 겹친 대상을 목록으로 보여 주고 휠로 선택하는 동작은 유지한다(사용자).
- Resolver는 유지한다. 엔진 CreateInstance는 `NewObject`만 부르고 `ReleaseInstance`는 Resolver에만 `DestroyInstance`를 부르므로(UE 5.8 `MVVMViewClass.cpp`), Resolver 없이는 스캐너 전달·VM 정리가 불가능하다.
- 스캐너 신호를 `OnRowsChanged` 하나로 합치고, VM은 신호마다 스캐너에서 목록과 선택을 읽어 행 전체를 다시 만든다. 대가: 선택을 바꿀 때마다 ListView 엔트리가 새로 붙는다(현재 `WBP_Interaction`은 이미지 가시성만 바꾸므로 화면 차이 없음).
- 스캐너 늦은 도착 관찰 코드(`OnAnyScannerReady`)를 제거한다. 스캐너가 `AWxPlayerController` 생성자 컴포넌트로 돌아왔으므로 Resolver가 직접 찾는다.
- 두 클래스 구조를 유지하고 `UWxViewModel_Interaction` 하나로 합치는 안은 기각했다(한 클래스 두 역할, 모듈 이동과 CoreRedirects 3개 필요).
- 범위 밖: 엔진 MVVM ListView 확장으로 `WBP_Interaction` BP 연결 코드 제거, 공용 Resolver 통합.

## 구현

- `WxInteractionScannerComponent.h/.cpp`: `OnListChanged`·`OnSelectionChanged` → `OnRowsChanged`, `OnAnyScannerReady`·`FWxOnScannerReady` 제거.
- `WxViewModel_InteractionList.h/.cpp`: `SelectedIndex`, `ApplySelection`, `RebuildEntries`, 신호 핸들러 두 개 제거. `Initialize` 첫 줄의 `Deinitialize()` 호출과 `Deinitialize`의 표시 비우기도 제거했다(새 VM에 한 번만 불리고 정리된 VM은 재사용 안 함).
- Resolver: `FindComponentByClass`로 찾은 스캐너를 `Initialize`에 넘긴다.
- 행 VM: `SetPrompt`/`SetSelected`와 `WxViewModel_Interaction.cpp` 삭제. 행은 만들어진 뒤 바뀌지 않으므로 목록 VM이 필드에 직접 값을 넣는다. 문구 변경은 스캐너가 0.1초마다 비교해 `OnRowsChanged`로 새 행을 만든다. `FieldNotify` 지정자는 WBP OneWay 바인딩 때문에 유지했다.
- WBP 에셋은 변경하지 않았다.

## 상호작용 문구 출처 (확정 결정)

- 모든 문구를 StateTree로 옮기는 안은 기각. 기준: "문구는 상호작용했을 때 실제로 일어날 행동의 주인이 갖는다." Device는 StateTree, 대화 액터는 대화 컴포넌트, 픽업은 아이템 데이터, 피니시는 피니시 어빌리티.
- 엘리베이터 층 문구 C++ `"{0}층"`을 `FWxDeviceTriggerRule_SplineStops::StopPrompt`로 옮겼다(StateTree에서 편집).
- 픽업 문구의 `[F]`를 제거했다. 키 표시는 `WBP_Interaction`의 `CommonActionWidget`이 맡는다.

## 엘리베이터 탑승칸 버튼 잠금 (사용자 확정 B안)

- 증상: 비활성 상태에서 탑승칸 목록에 `Floor 1`과 `Floor 2`가 모두 떴다. 원인은 비활성 대기 노드의 `bAcceptCurrentStop=true`가 탑승칸 목록에도 현재 층을 넣은 것.
- 판단: 기획서(`Object_Design.md` §4.4)의 활성화 레버 방식(A안)은 레버가 없는 기존 엘리베이터 3대를 영구히 잠그므로 보류. B안은 비활성 상태에서 탑승칸 버튼을 잠그고 탑승칸 목록에서 항상 현재 층을 뺀다. 활성화 레버는 필요할 때 기획 확인 후 별도 작업.
- 구현: `bAcceptCurrentStop` → `bWakeOnCall`로 이름 변경. 탑승칸 목록은 플래그와 관계없이 현재 층을 뺀다.
- 발견한 함정(구현 관찰): 인스턴스 구조체 안 FText에 C++ 기본값(`NSLOCTEXT "Floor {0}"`)을 두면 값이 기본값과 같을 때 `ST_Elevator` 저장이 `FortniteMain` 커스텀 버전 불일치로 실패했다. `StopPrompt`의 C++ 기본값을 제거해 해결했다. 최종 `ST_Elevator`는 Inactive `bWakeOnCall=true`, Idle `false`, 둘 다 `StopPrompt="Floor {0}"`.

## 검증 범위

- AI 빌드: build-doctor(WxEditor Win64 Development) 단계마다 성공.
- AI BP 컴파일: `CompileAllBlueprints`로 `WBP_InteractionList`, `WBP_Interaction`, `WBP_GameLayout` 오류 0·경고 0. 제거 심볼 소스 검색 0건.
- 사람(이우성, 2026-09-25) 체크리스트 6개 통과: 목록 표시, 휠 선택, 범위 이탈·리스폰, 행 표시(`Floor N`·키 아이콘 한 번), 탑승칸 버튼 잠금, 코드 리뷰.

## 관련 주제

- [[상호작용과 장치]]
- [[UI 표시 구조]]
- [[결정 노트 - 2026-09-23-interaction-list-vm]]
- <!--wl-->결정 노트 - 2026-09-26-interaction-list-play-acceptance
- [[기획서 - Object_Design]]

## 핵심 주장

- 상호작용 스캐너는 목록 변경과 선택 변경 신호를 OnRowsChanged 하나로 합쳤고, 목록 VM은 신호마다 행 전체를 다시 만든다. ^c1
- 상호작용 문구는 실제로 일어날 행동의 주인이 가진다는 기준에 따라 Device는 StateTree, 픽업은 아이템 데이터, 피니시는 피니시 어빌리티가 문구를 가진다. ^c2
- StateTree 인스턴스 구조체의 FText에 C++ 기본값을 두면 값이 기본값과 같을 때 에셋 저장이 FortniteMain 커스텀 버전 불일치로 실패했다. ^c3
- 상호작용 목록 VM 단순화와 엘리베이터 버튼 잠금은 2026-09-25 이우성이 인게임 5개 항목과 코드 리뷰를 모두 통과로 확인했다. ^c4
