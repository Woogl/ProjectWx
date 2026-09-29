# WxUI 리뷰 1·4번: 복제 빙의 시 플레이어 VM 갱신과 HUD 교체 시 메뉴 로드 취소

상태: 확인 대기 · 빌드·자동화 테스트 통과
다음 행동: 코드 리뷰를 확인한다.

- woogle 결정(2026-09-30): 1번(빙의 판정)은 WxUI에서 해결한다. "1번은 WxUI에서 해결하는게 낫죠?" → 같은 ASC 판정을 `UWxViewModel_Character::Initialize`가 하고, PC는 `SetPawn`마다 조건 없이 갱신한다.
- woogle(2026-09-30): 2번(원격 클라이언트 슬롯 재매칭)은 고치지 않는다. "지금은 런타임에서 어빌리티 부여하는 경우가 없으니 안고쳐도 되죠?" 플레이어 스킬은 빙의 프레임에 서버에서 부여되고 슬롯 어빌리티의 런타임 부여·회수가 없어서 지금은 드러나지 않는다. 스킬 교체·장비 부여를 넣거나 멀티를 정식 지원할 때 다시 본다(고친다면 부여·회수 모두, 권한과 무관하게 알려야 한다).
- 3번(이전 VM 트리 정리)은 요청 범위(1·4번) 밖이다.

## 요청

- 요청 · woogle 2026-09-30

> 1번, 4번 해결해주세요

근거 리뷰: [WxUI 리뷰](module_review_WxUI.md) 1번(=[WxGame 리뷰](module_review_WxGame.md) 1번)과 4번.

## 질문

추가 질문 없음

## 구현 계획

구현 승인: woogle 2026-09-30 (대화 지시 "1번, 4번 해결해주세요")

1. 복제 경로 빙의 교체 시 플레이어 Character VM 갱신
   - 원인: 클라이언트에서 엔진은 복제된 `Pawn`을 먼저 대입한 뒤 `SetPawn(Pawn)`을 부르므로(`Controller.cpp` `OnRep_Pawn`·`SetPawnFromRep`), `AWxPlayerController::SetPawn`의 전후 폰 비교가 항상 같다고 판정해 VM을 다시 채우지 않는다.
   - WxUI `UWxViewModel_AbilitySystem`: 관찰 중인 ASC를 읽는 `GetBoundASC()`를 추가한다.
   - WxUI `UWxViewModel_Character::Initialize`: 새 ASC가 있고 지금 AbilitySystem VM이 같은 ASC를 보고 있으면 AbilitySystem VM을 새로 만들지 않고 이름만 갱신한다. 새 ASC가 없으면 지금처럼 비운다.
   - WxGame `AWxPlayerController::SetPawn`: 전후 폰 비교를 없애고 매번 `RefreshPlayerCharacterViewModel`을 부른다. 빙의 교체 통지보다 먼저 불리는 순서는 그대로다.
2. HUD가 걷힐 때 진행 중인 메뉴 로드 취소
   - WxUI `UWxHUDLayout::NativeDestruct`에서 `PendingMenuPush`가 있으면 `Cancel()`한다. 취소는 완료 콜백을 거쳐 `PendingMenuPush`도 비운다.
   - 비활성화(`NativeOnDeactivated`)가 아닌 파괴 시점을 쓴다. 대화 화면이 같은 Game 레이어에 올라오면 HUD가 비활성화만 되므로, 그때 메뉴 요청을 끊는 것은 이번 범위 밖의 동작 변경이다. 프로젝트 레이어 스택은 전환 시간이 0이라 `ClearLayout`의 제거가 같은 프레임에 `NativeDestruct`까지 이어진다.
- 검증: WxEditor Development 빌드, 리슨 서버+클라이언트 PIE에서 서버가 클라이언트 PC를 새 캐릭터로 재빙의한 뒤 클라이언트 VM이 새 ASC를 보는지, 같은 폰 재호출에서 AbilitySystem VM이 유지되는지 값으로 확인한다.

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 빌드 | WxEditor Win64 Development 빌드(임시 테스트 삭제 후) | AI | 통과 | Result: Succeeded, BUILD_DOCTOR_RESULT=success (Saved/Logs/BuildDoctor/build_2026-09-30_041226_036_17092.log) |
| 같은 ASC 재초기화 유지 | 임시 자동화 테스트 Wx.Temp.CharacterVMSameASC: 같은 ASC 재호출·다른 ASC·null ASC | AI | 통과 | 같은 ASC는 AbilitySystem VM 유지와 이름 갱신, 다른 ASC는 새 VM, null은 비움. 수정을 끈 빌드에서는 실패 |
| 원격 클라이언트 재빙의 시 VM 갱신 | 헤드리스 PIE(UnrealEditor-Cmd -NullRHI, LV_DevCombat, 리슨 서버+클라이언트): 서버가 클라이언트 PC를 새로 스폰한 같은 클래스 캐릭터로 Possess하고 1초 뒤 클라이언트 VM_PlayerCharacter의 ASC를 비교 | AI | 통과 | 수정 후: 클라이언트 VM이 새 폰 BP_Template_C_2의 ASC를 봄(옛 폰은 살아 있음). 수정을 끈 빌드: VM이 빈 채로 남음(bound owner none) |
| 같은 폰 SetPawn 재호출 | 같은 PIE에서 클라이언트 PC의 SetPawn을 현재 폰으로 직접 호출 | AI | 통과 | AbilitySystem VM 객체가 그대로 유지됨 |
| 호스트·초기 접속 VM | 같은 PIE 시작 직후 호스트 VM과 클라이언트 VM이 각자 폰의 ASC를 보는지 | AI | 통과 | 호스트 BP_Template_C_0, 클라이언트 BP_Template_C_0 모두 일치 |
| HUD 걷힌 뒤 메뉴 로드 취소 | 같은 PIE 클라이언트: 로드되지 않은 WBP_MainMenu 요청을 HUD의 PendingMenuPush로 시작하고, 같은 프레임에 ClearLayout과 같은 순서(비활성화 후 Game 스택에서 제거)로 HUD를 걷은 뒤 5초 대기 | AI | 통과 | 수정 후: 요청이 대기 상태였고 클래스 로드가 끝났지만 메뉴가 뜨지 않음. 수정을 끈 빌드: 메뉴가 뜸. 실제 폰 교체로 HUD를 걷는 경로는 로드 완료 시점과 경쟁해 결정적으로 재현할 수 없어 제거 절차를 직접 호출했다 |
| 임시 테스트 정리 | Source/WxEditor/Tests 삭제, WxEditor.Build.cs의 임시 CommonUI 의존 되돌림, 지운 상태로 빌드 | AI | 통과 | git status에 Source/WxEditor 변경 없음, 빌드 성공 |
| 코드 리뷰 | 변경 파일: Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_AbilitySystem.h·Private/MVVM/WxViewModel_AbilitySystem.cpp(GetBoundASC), Public/MVVM/WxViewModel_Character.h·Private/MVVM/WxViewModel_Character.cpp(같은 ASC면 유지), Public/Widget/WxHUDLayout.h·Private/Widget/WxHUDLayout.cpp(NativeDestruct에서 메뉴 요청 취소), Source/WxGame/Controller/WxPlayerController.cpp(SetPawn마다 갱신). 볼 점: Initialize 계약 변경(보스 바·네임플레이트도 같은 규칙을 따름), SetPawn 무조건 갱신, 파괴 시점 취소 | 사람 | 대기 |  |
