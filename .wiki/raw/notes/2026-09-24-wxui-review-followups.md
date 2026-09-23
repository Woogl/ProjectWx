---
title: "WxUI 리뷰 후속: 일시정지 해제 규칙 정정·Effect VM 월드 타이머·인디케이터 여백 상수"
source: "MANUAL"
type: notes
ingested: 2026-09-24
tags: [wx, ui, architecture]
summary: "게임모드는 대리자를 건 정지만 해제 때 되묻고 대리자 없는 정지는 그냥 지운다는 엔진 동작에 맞춰 UIManager 주석을 고쳤다. Effect VM 남은 시간은 코어 티커 대신 월드 타이머로 갱신하고, 인디케이터 화면 여백은 C++ 상수로 바꿨다."
---

# WxUI 리뷰 후속 네 건

2026-09-24 커밋 `eb92e99a7`·`08a1ef928`·`1b15a61ff`·`a4c28a949`. WxUI 모듈 리뷰(`bf9c872ca`, [작업 자료](../../../.agents/workflow/tasks/module_review_WxUI.md)) 중에 고친 항목이다. 커밋 메시지에 이유가 없어 HEAD `d76e48717` 코드와 UE 5.8 엔진 소스로 대조했다. 리뷰에 남은 미해결 항목(원격 클라이언트 슬롯 재매칭)은 Workflow 작업 자료라 여기로 옮기지 않았다.

## 일시정지 해제 규칙 (`1b15a61ff`, 주석 정정)

- `UWxUIManagerSubsystem::RefreshGamePause`는 Standalone에서 정지를 걸 때 `FCanUnpause` 대리자(`HandleCanUnpause` = `!WantsGamePause()`)를 함께 건다. 해제는 `SetPause(false)`다.
- 엔진 `AGameModeBase::ClearPause`(UE 5.8 `GameModeBase.cpp:237`~`274`)는 정지 목록(`Pausers`)을 돈다. 대리자를 건 정지는 대리자가 참일 때만 지우고, 대리자가 없는 정지는 묻지 않고 지운다. 목록이 비면 정지가 풀린다.
- 그래서 UI가 정지를 원하는 동안 다른 주체의 해제는 UIManager의 정지를 지우지 못한다. 반대로 대리자 없이 건 다른 정지는 UIManager가 해제할 때 함께 지워진다. 옛 주석은 두 방향이 모두 막힌다고 적었다.
- 현재 C++에서 `SetPause`를 부르는 곳은 UIManager뿐이다. `Content`·`Plugins/*/Content` 에셋에서도 `SetGamePaused`·`SetPause` 문자열은 나오지 않았다(2026-09-24 grep). 다른 정지 주체를 추가하면 대리자를 함께 걸어야 한다.

## Effect VM 남은 시간 갱신 (`eb92e99a7`)

- `UWxViewModel_Effect`는 유한 지속 효과의 남은 시간을 코어 티커(`FTSTicker`) 대신 월드 타이머로 갱신한다. `SetTimerForNextTick`으로 다음 틱을 예약하고, 콜백에서 갱신에 성공하면 다시 예약한다. 효과가 사라졌거나 ASC·월드가 없으면 멈추고, Deinitialize에서 타이머를 지운다.
- 남은 시간 계산(`StartWorldTime + Duration − World->GetTimeSeconds()`)은 바뀌지 않았다. 월드 타이머는 월드가 정지된 동안 돌지 않는다(UE 5.8 `LevelTick.cpp:1812`의 `!bIsPaused` 조건). 계산 기준인 월드 시간도 정지 중에는 멈추므로 표시 값은 같고, 정지 중의 헛갱신만 사라진다.
- 무한 지속 효과는 링을 가득 채우고 갱신을 걸지 않는다(변경 없음).

## 인디케이터 화면 여백 (`08a1ef928`)

- `AWxIndicator::ScreenMargin`을 `EditAnywhere` 속성에서 `static constexpr float` 48로 바꿨다. 아이콘 크기를 바꾸면 C++ 값도 함께 바꾼다.
- `Content`·`Plugins/*/Content` 에셋에서 `ScreenMargin` 문자열이 나오지 않았다(2026-09-24 grep). 그래서 사라진 에셋 오버라이드는 없는 것으로 본다.

## 확인 팝업 헬퍼 인라인 (`a4c28a949`)

`UWxUILibrary::ShowConfirmationPopup`의 익명 namespace 헬퍼(`MakeNativeResultDelegate`)를 호출부로 옮겼다. 동작은 바뀌지 않는다.

## 검증 범위

코드·엔진 소스 정적 대조와 에셋 문자열 검색만 했다. 이 기록에서 빌드와 인게임 동작(일시정지 메뉴, 효과 게이지, 화면 밖 인디케이터)은 확인하지 않았다.

근거: [UIManager](../../../Plugins/WxUI/Source/WxUI/Private/System/WxUIManagerSubsystem.cpp), [Effect VM](../../../Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Effect.cpp), [인디케이터](../../../Plugins/WxUI/Source/WxUI/Public/Indicator/WxIndicator.h), [UI 라이브러리](../../../Plugins/WxUI/Source/WxUI/Private/WxUILibrary.cpp).
