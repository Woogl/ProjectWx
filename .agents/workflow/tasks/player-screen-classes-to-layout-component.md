# 사망·대화 화면 클래스를 컨트롤러 컴포넌트로 이동

상태: 완료 · 커밋 대기 · 2026-09-23

## 확정 설계 · 2026-09-23 사용자 확정

- 요청: `UWxUIDeveloperSettings`의 `DeathScreenClass`·`DialogueScreenClass` 위치를 검토했다.
- 판단: 설정에는 UI의 틀(`LayoutClass`, `ConfirmationPopupClass`)만 두고, 게임 화면은 컨트롤러 BP 쪽에 둔다.
  - 근거 1: HUD는 `UWxPlayerLayoutComponent::LayoutClass`에, 메뉴는 `UWxHUDLayout`에 이미 있다.
  - 근거 2: 사망·대화 화면은 `a8cffba6c`(07-28)에서 HUD와 함께 설정으로 옮겨졌다. 이후 HUD만 `f8718e3a6`(08-23)에서 다시 컴포넌트로 나갔고, 이 둘은 그대로 남아 있었다.
- 설계: 태그 관찰(`Ability.Death`·`State.Dialogue`)과 대화 창 수명을 `UWxUIManagerSubsystem`에서 `UWxPlayerLayoutComponent`로 옮긴다. 서브시스템은 레이아웃·팝업·일시정지만 맡는다. `TrackedPlayerController`는 일시정지용으로 남긴다.
- 주의: 폰이 바뀔 때 대화 창만 닫고 사망 화면은 닫지 않는다. 부활이 폰을 교체하며, 사망 화면은 부활 요청이 완료될 때 스스로 비활성화된다.
- 대가: 값이 ini에서 BP 에셋으로 옮겨져 git diff로 값 변경을 리뷰할 수 없다. 모드마다 다른 화면을 쓰려면 컨트롤러 BP 자식이 필요하다.

## 구현 · 2026-09-23

- 커밋되지 않은 변경이며, 기준은 HEAD `47b7f8bd7`다.
- `WxPlayerLayoutComponent.h/.cpp`: `DeathScreenClass`·`DialogueScreenClass`(EditDefaultsOnly)를 추가했다. `WatchPawnTags`, 태그 핸들러 두 개, 대화 창 push 완료·닫기 처리를 서브시스템에서 옮겼다.
  - 폰 교체(`OldPawn != NewPawn`) 때 관찰 대상을 새 폰으로 바꾼다.
  - `EndPlay`에서 관찰을 끊는다.
- `WxUIManagerSubsystem.h/.cpp`: 빙의 구독, 태그 관찰, 대화 관련 멤버를 제거했다.
- `WxUIDeveloperSettings.h`, `DefaultGame.ini`: 두 필드와 두 값을 제거했다.
- BP: 사용자가 `BP_PlayerController` → `PlayerLayoutComponent`에 `WBP_DeathScreen`·`WBP_DialogueScreen`을 입력했다(2026-09-23). 에셋에 두 참조가 들어간 것을 확인했다. `BP_FrontEndPlayerController`는 비워 두었다.

## 검증 · 2026-09-23

- 첫 build-doctor 실행은 UHT 단계에서 실패했다. 원인은 다른 세션(wx-09)이 진행 중이던 `WxViewModel_Item`·`WxViewModel_InventoryItem` 변경이며, 이번 변경과는 무관하다.
- 재실행(WxEditor Win64 Development): 성공, 경고 0. 로그는 `Saved/Logs/BuildDoctor/build_2026-09-23_105827_436_15860.log`이다. 이 빌드에는 wx-09의 미커밋 변경도 함께 들어 있다.
- 인게임(사용자 확인, 2026-09-23): 통과. "잘 됩니다"
  - 사망하면 사망 화면이 뜨고, 부활하면 닫히는지
  - 대화를 시작하면 대화 창이 뜨고, 끝나면 닫히며 HUD가 돌아오는지
  - 부활한 뒤에도 사망·대화 화면이 다시 뜨는지(새 폰 관찰)
- 인간 코드 리뷰: 별도 승인 기록 없음. 사용자는 인게임 결과만 확인했다.

## 완료 · 2026-09-23

- Wiki: 원자료 `raw/notes/2026-09-23-player-screen-owner.md`를 추가하고 `ui.md`·`dialogue.md`를 갱신했다. 옛 서술 "대화 창은 UI 매니저 몫"을 고쳤다. `log.md`와 `raw/notes/_index.md`에도 반영했다.
- 저장소 링크 검사(`CheckWikiLinks.ps1`) 결과: 0 errors, 41 documents.
- 뷰어 재생성은 하지 않았다(PS 7 미설치).
- 커밋: 미실행. 작업 트리에 wx-09 세션의 미커밋 변경(`WxViewModel_*`, `DefaultEngine.ini` 등)이 섞여 있어, 커밋할 때 이번 변경 파일만 골라야 한다.
