---
type: source
title: "작업 - dialogue-presentation-vm"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "작업-기록"
  - "UI"
  - "MVVM"
  - "대화"
summary: "Dialogue VM을 WxUI의 순수 표시 데이터로 분리하고, 화면 클래스를 거쳐 최종적으로 WxGame 리졸버 세 층 구조로 정리한 완료 작업 기록"
source_type: task-record
source_id: src-7f1ecad66c49bd181697
sha256: da4546f1bd301d99cf7435c8834b83664048a0a457f77d76658888faff5b10d5
authority: primary
independence_key: ".agents/workflow/tasks/dialogue-presentation-vm.md"
review_state: active
refresh_due: 2027-03-25
original_paths:
  - ".agents/workflow/tasks/dialogue-presentation-vm.md"
raw_copy: ".raw/captured/da4546f1bd301d99cf7435c8834b83664048a0a457f77d76658888faff5b10d5.md"
claim_ids:
  - clm-350702917e-c1
  - clm-350702917e-c2
  - clm-350702917e-c3
  - clm-350702917e-c4
key_claims:
  - "최종 구조에서 대화 UI는 WxDialogue 세션, WxGame의 UWxViewModelResolver_Dialogue, WxUI의 UWxViewModel_Dialogue 세 층으로 나뉘며 VM은 세션을 모른다."
  - "UWxDialogueScreen 클래스는 사용자 요청으로 삭제되었고 WBP_DialogueScreen의 부모는 WxActivatableWidget으로 바뀌었다."
  - "리졸버 복귀 구조는 WxEditor Development 빌드와 4개 WBP의 경고-오류 컴파일을 통과했고, 사용자가 2026-09-23 인게임에서 문제없다고 확인했다."
  - "리졸버 구조에서는 대화 화면이 활성 상태가 아니어도 위젯이 생성되어 있는 동안 세션을 구독한다."
---

# 작업 - dialogue-presentation-vm

- 원본: `.agents/workflow/tasks/dialogue-presentation-vm.md`
- 원자료 사본: `.raw/captured/da4546f1bd301d99cf7435c8834b83664048a0a457f77d76658888faff5b10d5.md` (수집 2026-09-26, 재확인 기한 2027-03-25)
- 원본 갱신(2026-09-26): 옛 원자료 사본 `.raw/captured/6b19e4bae75efc3cbeddf6a3cbfb3f2a1a78bdf3f6d63724aa125c13bf711bf3.md`를 대체했다. 2026-09-26 기록 정리로 머리의 옛 `이전 상태:` 줄을 지운 것뿐이며, 작업 내용·상태·검증 범위는 그대로다.

## 개요

대화 화면의 뷰모델(`UWxViewModel_Dialogue`)을 세션을 모르는 순수 표시 데이터로 만들고, 대화 세션과의 연결 책임을 어디에 둘지 세 차례 바꾼 작업 기록이다. 상태는 완료이며, 최종 구조(리졸버 복귀)는 2026-09-23 사용자가 인게임에서 확인했다.

## 요청과 경계 (확정 결정)

- Dialogue VM 자체를 순수 표시 데이터로 만든다. 자식 VM을 추가하는 방식은 아니다.
- Wx 기능 모듈 간 신규 의존성은 WxCore를 제외하면 금지하고, WxCore에는 공용 정의를 넘어선 게임 로직이나 과도한 추상화를 넣지 않는다.
- 기존 WxGame 조립 계층이 WxUI와 WxDialogue를 연결한다.

## 구현 이력

### 1차: 순수 표시 VM 분리
- WxUI `UWxViewModel_Dialogue`는 Speaker, LineText, HasSpeaker와 `SetLine` 변경 알림만 가진다.
- WxGame `UWxViewModelResolver_Dialogue`가 세션 `OnLineChanged`를 VM `SetLine`에 연결했고, `UWxDialogueScreen`이 `RequestAdvance`를 세션에 전달했다.
- 이동한 VM 클래스만 CoreRedirect로 WxGame → WxUI 경로를 유지했다. 이 리다이렉트는 2026-09-24 사용자 지시로 `DefaultEngine.ini`의 `[CoreRedirects]`를 모두 비울 때 제거됐다(참조 WBP 6개가 누락 클래스 경고 없이 로드, 인게임 표시는 미확인).
- 에디터 도구에 MVVM 이벤트 목적지를 위젯 함수로 바꾸는 기능을 추가했다(MVVMEditorSubsystem 사용).

### 2차: Resolver 제거·화면 클래스에 연결 집중 (사용자 요청)
- WBP VM 생성 방식을 Create Instance로 바꾸고 Resolver 소스를 삭제했다. `UWxDialogueScreen`이 활성화 때 구독, 비활성화·파괴 때 해제했다.
- 자동화 `Wx.UI.Dialogue.ScreenLifecycle`로 조기/일반 활성화, 두 화면 독립 구독, 재활성 복구, 재생성 시 구형 VM 해제를 검사해 통과했다. 이후 사용자 요청으로 테스트 파일(`WxDialogueScreenTest.cpp`)과 테스트용 friend 선언을 제거했다.

### 최종: 화면 클래스 제거·리졸버 복귀 (2026-09-23)
> 사용자 2026-09-23: "MVVM을 쓰기 때문에 굳이 Widget 클래스를 늘릴 필요가 없다."

- 구조는 보스 바와 같은 세 층이다: 모델(WxDialogue 세션) / 연결(WxGame 리졸버) / 표시(WxUI VM).
- `UWxViewModel_Dialogue`에 진행 명령 `RequestAdvance()`(BlueprintCallable)와 네이티브 단일 델리게이트 `OnAdvanceRequested`를 추가했다. VM은 세션을 모른다.
- `UWxViewModelResolver_Dialogue`(신규): CreateInstance에서 위젯을 Outer로 VM을 만들고 세션 `OnLineChanged`→`SetLine`, `OnAdvanceRequested`→세션 `Advance`(BindUObject)를 잇는다. DestroyInstance는 `RemoveAll(VM)`로 그 VM 구독만 끊는다. 리졸버는 상태가 없다.
- `WBP_DialogueScreen`: 부모 `WxDialogueScreen` → `WxActivatableWidget`, 진행 이벤트 목적지 `Self.RequestAdvance` → `WxViewModel_Dialogue.RequestAdvance`. `WxDialogueScreen.h/.cpp`는 삭제했다.
- WxToolset의 이벤트 목적지 설정 함수를 `SetEventDestination(WidgetBlueprint, EventIndex, "Self.함수" | "뷰모델이름.함수")`로 일반화했다.
- 수명 근거(엔진 코드 관찰): CommonUI 스택은 창을 닫을 때 `GeneratedWidgetsPool.Release(Widget, true)`로 해제하므로 `NativeDestruct` → MVVM `Destruct` → `DestroyInstance`가 불리고, 다시 띄우면 `CreateInstance`로 새 VM이 현재 대사를 채운다.
- 동작 차이: 이제는 활성화 여부가 아니라 위젯이 생성되어 있는 동안 구독한다. 비활성 화면은 보이지 않아 클릭될 수 없고 `Advance`는 활성 대화가 없으면 무시한다.

## 검증 범위

- AI 빌드: WxEditor Win64 Development 빌드 성공(클래스 삭제 후 최종, 종료 코드 0).
- AI 에셋 검증: 이관 로그와 새 프로세스 재검증에서 부모·리졸버·VM 클래스·이벤트 목적지를 확인했고, 4개 WBP가 경고를 오류로 취급한 컴파일을 통과했다. Content에서 삭제 클래스 이름의 바이너리 참조 0건.
- 런타임 구독·해제는 코드와 엔진 수명으로만 확인했다(정적).
- 사람 인게임 확인: 대화 표시·클릭 진행·종료 후 재대화를 사용자가 확인했다.

> 사용자 2026-09-23: "테스트 문제 없습니다."

- 인간 코드 리뷰의 별도 승인 기록은 없다.

## 관련 주제

- [[UI 표시 구조]]
- [[퀘스트와 대화]]
- [[모듈 구조와 코드 정리]]
- [[결정 노트 - 2026-09-23-dialogue-presentation-vm]]
- [[결정 노트 - 2026-09-23-screen-classes-to-resolvers]]

## 핵심 주장

- 최종 구조에서 대화 UI는 WxDialogue 세션, WxGame의 UWxViewModelResolver_Dialogue, WxUI의 UWxViewModel_Dialogue 세 층으로 나뉘며 VM은 세션을 모른다. ^c1
- UWxDialogueScreen 클래스는 사용자 요청으로 삭제되었고 WBP_DialogueScreen의 부모는 WxActivatableWidget으로 바뀌었다. ^c2
- 리졸버 복귀 구조는 WxEditor Development 빌드와 4개 WBP의 경고-오류 컴파일을 통과했고, 사용자가 2026-09-23 인게임에서 문제없다고 확인했다. ^c3
- 리졸버 구조에서는 대화 화면이 활성 상태가 아니어도 위젯이 생성되어 있는 동안 세션을 구독한다. ^c4
