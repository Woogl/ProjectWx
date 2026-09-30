# WxGame — 코드 리뷰

> 플레이어 Character VM 갱신은 이제 서버 빙의·RPC·프로퍼티 복제 어느 경로로 들어와도 같은 판정을 거치고, 빙의 교체 통지보다 먼저 채워지는 순서도 엔진 경로와 맞는다. 남은 미해결 지적은 이미 알려진 모듈 경계 예외 하나다.
> 이번 리뷰는 `0260099b1` 위의 미커밋 변경(`Source/WxGame/Controller/WxPlayerController.cpp`)과 그 호출 경로(`WxGameViewModel::InitializeCharacter`의 모든 호출자, 연동 WxUI VM, UE 5.8 빙의·복제 경로)를 깊게 봤다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 1 |
| 🟢 사소 | 0 |

## 결과

### 1. 🟡 InteractionList 뷰모델이 아직 WxGame에 있고 도메인 컴포넌트를 직접 호출한다
- **위치**: `Source/WxGame/MVVM/WxViewModel_InteractionList.h:21`
- **범주**: 설계/구조
- **문제**: 2026-09-30 채택한 UI 설계 원칙 2번(모든 뷰모델은 WxUI에 두고 도메인 타입을 쓰지 않음)과 9번(도메인 명령은 뷰모델이 델리게이트로 내보내고 조립 층이 모델을 호출)에 어긋난다. `UWxViewModel_InteractionList`는 WxWorld의 `UWxInteractionScannerComponent`를 약참조로 들고 `OnRowsChanged`를 직접 구독하며(`Source/WxGame/MVVM/WxViewModel_InteractionList.cpp:17`), `RequestInteract`·`RequestCycle`이 스캐너를 바로 호출한다(같은 파일 55·63행). [MVVM 원칙에 맞춘 뷰모델 재설계](viewmodel-mvvm-redesign.md)는 이를 "알려진 예외(후속 일감)"로, [뷰모델 품질 정리](viewmodel-quality-cleanup.md)는 7번 "나중"으로 남겼다. [상호작용 목록 뷰모델을 UI 설계 원칙에 맞추기](interaction-list-vm-principles.md)가 맡는다.
- **제안**: 뷰모델은 표시 값(`Entries`)과 요청 델리게이트만 가진 WxUI 클래스로 옮기고, 스캐너 구독·값 넣기·요청 전달은 WxGame의 `UWxViewModelResolver_InteractionList`가 맡게 한다(원칙 3번: 위젯마다 만드는 뷰모델은 리졸버가 만든다). WBP 참조는 ClassRedirects로 잇고 리세이브 뒤 지운다.
- **확신도**: 높음

## 검토 범위
- **깊게 본 파일**: `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Controller/WxPlayerController.h`, `Source/WxGame/MVVM/WxGameViewModelUtils.h`, `Source/WxGame/MVVM/WxGameViewModelUtils.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp`, `Source/WxGame/Controller/WxNameplateManagerComponent.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.h`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`
- **훑은 파일**: `Source/WxGame/Battle/WxBattleSubsystem.h`, `Source/WxGame/Battle/WxBattleSubsystem.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/Character/WxCharacterBase.cpp`(ASC 소유 위치). 교차 근거로 `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModel_Character.cpp`·`WxViewModel_AbilitySystem.cpp`와 헤더, `Plugins/WxUI/Source/WxUI/Private/Component/WxPlayerLayoutComponent.cpp`, UE 5.8 `Controller.cpp`(`Possess`·`UnPossess`·`SetPawnFromRep`·`OnRep_Pawn`, `Pawn`의 `REPNOTIFY_Always`)·`PlayerController.cpp`(`ClientRetryClientRestart`·`ClientRestart`·`OnPossess`·`OnUnPossess`·`SetPawn`)·`Pawn.cpp`(`OnRep_Controller`)를 대조했다. 전 59파일에서 첫 줄 저작권 문구와 인라인 정의를 검색했고 위반은 없었다.
- **미검토 / 한계**: 이번 리뷰는 커밋 `0260099b1` 위의 미커밋 작업 트리 변경(`Source/WxGame/Controller/WxPlayerController.cpp`)을 포함해 봤다. 검토 중 다른 세션의 임시 검증(`Source/WxEditor/Tests/WxTempPossessionMenuTest.cpp`)이 `SetPawn`에 전후 폰 비교를 잠시 되넣었다가 되돌렸고, 되돌린 뒤의 상태(위 diff와 같음)를 기준으로 판정했다. 정적 리뷰이며 빌드·자동화 테스트·리슨 서버 실행은 직접 하지 않았다. 이전 AbilitySystem VM 트리의 구독 해제는 WxUI 리뷰 3번으로 보류된 WxUI 문제라 다루지 않았다. 변경이 없는 나머지 영역(캐릭터·AI 컨트롤러·프런트엔드·치트 등)은 직전 리뷰에서 봤고 다시 통독하지 않았다. BP/WBP·DataTable·BT·StateTree 내부는 범위 밖이다.

---
*문서 기준 커밋 `0260099b1` · 리뷰일 2026-09-30 · 소스 59파일 — `/module-review`로 갱신*
