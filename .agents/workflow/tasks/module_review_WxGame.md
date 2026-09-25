# WxGame — 코드 리뷰

상태: 확인 대기 · 남은 UI 테스트 소스까지 제거하고 최종 빌드 통과
다음 행동: 코드 리뷰와 실제 레벨 재표시·새 게임 진입을 확인한다.

## 현재 작업

- **추가 삭제 승인**: 사용자(2026-09-25) — 기존 UI 테스트 `Source/WxGame/Tests/WxUIPresentationTests.cpp`의 회귀 검사 3개도 삭제할지 확인한 뒤 “네, 제거하고 제출합시다”라고 승인했다. 해당 파일을 제거하고 재제출한다.
- **요청**: 사용자(2026-09-25) — “WxGame — 코드 리뷰 의 지적사항을 모두 올바른 방법으로 해결해주세요.”
- **후속 지시**: 사용자(2026-09-25) — “테스트 코드 제거 후 제출해주세요.”, “EWxAbilitySetGrantState  이라는 State가 추가된 것도 마음에 안들어요”. 추가로 “OnRegister, Unregister 자체를 아예 override하지 않았으면 해요. EndPlay도요”, “단순하고 직관적인 코드가 좋거든요”를 지시했다. 상태 enum과 이번 작업에서 추가한 수명 훅을 제거하고 실제 스펙 보유 여부로 판단한다. 검증용 테스트 코드를 제거해 제출한다.
- **범위**: 아래 세 지적을 현재 작업 트리에 반영한다. 같은 재진입 테스트에서 드러난 WxAI 행동 컴포넌트의 컨트롤러 변경·피격 이벤트 중복 구독 방지도 포함한다. 다른 작업의 UI 리졸버·표시 계약과 Exclusive 정책 변경은 보존한다.
- **수정 방향**: ASC의 최초 속성·효과 초기화와 등록 해제 후 기본 어빌리티 재부여를 구분한다. 태그 구독은 중복을 막고 처치 통지를 객체 수명당 한 번으로 제한한다. 선택 Pawn 클래스를 저장 삭제 전에 검증하고 목적지까지 유지한다.
- **구현**: 최초 속성·GE 초기화와 어빌리티 부여 수명을 분리했다. 캐릭터 태그와 ASC SP 구독 중복을 막았고, 사망 처리 및 적의 BeginPlay 보상 재진입을 객체 수명당 한 번으로 제한했다. 선택 Pawn은 삭제 전에 검증하고 강한 참조로 유지한다.
- **자체 검증**: 수명 훅 제거 후 어빌리티 재등록·사망 및 AI 구독 회귀 2개 통과(오류·경고·미실행 0). 새 게임 저장 보존 검사는 변경 없는 구현의 이전 통과 결과를 유지한다. 테스트 코드를 제거한 최종 Editor Development 빌드도 통과했다.

## 수정 내용

1. **ASC 재등록** — `GiveAbilitySets`는 실제 보유한 스펙을 확인해 없는 어빌리티만 채운다. `bAbilitySetsInitialized` 하나로 속성·GE를 최초 한 번만 적용하며 HP/SP 초기화와 효과 중복을 막는다. 미등록·비권위 ASC는 부여하지 않는다. SP 변화 구독은 객체당 한 번이다. 부여 상태 enum·`bAbilitySetsGranted`·`OnRegister`·`OnUnregister`·`DestroyActiveState` 오버라이드를 추가하지 않는다.
2. **사망·이벤트 구독** — 캐릭터 사망·래그돌 태그 구독을 객체당 한 번으로 제한한다. `bDeathHandled`와 `bDeathNotified`로 사망 처리 및 시체 BeginPlay 재진입의 처치·보상 중복을 막는다. 부활은 새 Pawn을 생성하는 기존 계약을 따른다. WxAI의 컨트롤러 이벤트는 `AddUniqueDynamic`, 피격 이벤트는 유효한 구독 핸들이 없을 때만 등록한다. 콜백은 플레이 중에만 처리하므로 새 `EndPlay` 오버라이드가 필요 없다. 기존 적 캐릭터의 교전 정리용 EndPlay는 이번 변경 대상이 아니다.
3. **새 게임 검증 순서** — 체크포인트 삭제 전에 선택 클래스를 동기 로드하고 Pawn 상속 및 Abstract·Deprecated·NewerVersionExists 플래그를 확인한다. 실패하면 상태 문구만 갱신하고 요청을 거절한다. 유효한 클래스는 `UPROPERTY TSubclassOf<APawn>`으로 유지하여 목적지에서 다시 로드하지 않는다.

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| Editor Development 빌드 | build-doctor 실행기 | AI | 통과 | `Saved/Logs/BuildDoctor/build_2026-09-25_232031_628_31828.log`: Succeeded, 종료 코드 0 |
| WxGame 테스트 코드 제거 | 자동화 테스트 선언·조건부 코드·테스트 헤더 참조 검색 | AI | 통과 | WxGame 소스 검색 결과 0건, UI 표시 테스트 파일 삭제 확인 |
| 어빌리티 재등록 | `Wx.Game.Review.AbilityReregister`: 두 차례 등록 왕복, 스펙·인스턴스 복원과 HP·SP·효과 핸들 보존 | AI | 통과 | 최종 회귀 보고서: Success, 오류·경고 0 |
| 태그 구독·처치 일회성 | `Wx.Game.Review.DeathNotification`: 반복 초기화 후 사망·래그돌 구독 각각 1개, 사망 태그 재적용과 시체 BeginPlay 재진입의 처치 통지 1회 | AI | 통과 | 최종 회귀 보고서: Success, 오류·경고 0; 재진입 두 차례 뒤 AI 피격 구독도 1개 |
| 새 게임 실패 시 저장 보존 | `Wx.Game.Review.NewGameValidation`: 없는 경로·추상 클래스·잘못된 부모 거절, 유효 선택만 저장 삭제 | AI | 통과 | 이전 회귀 보고서(20260925_225721): Success, 오류·경고 0; 이후 해당 구현 변경 없음, 전용 UserDir에서 실행 |
| 코드 리뷰 | WxAbilitySystemComponent·WxAbilitySet의 누락 스펙 보충, WxAIBehaviorComponent의 단일 구독, 캐릭터 사망 일회성, WxGameFlowSubsystem의 저장 삭제 전 검증을 확인한다. | 사람 | 대기 | 사용자 단순화 지시를 반영한 최종 구현 확인 |
| 레벨 재표시 전투·연출 | 게임: 적이 있는 로드 유지 서브레벨을 두 번 숨겼다 표시한다. 공격·피격·사망이 동작하고 래그돌·보상이 중복되지 않는지 확인한다. | 사람 | 대기 | 자동 회귀는 컴포넌트·액터 수명 경로이며 실제 스트리밍 맵·연출 확인은 별도 |
| 새 게임 캐릭터 선택 | 프런트엔드에서 유효 캐릭터를 선택해 목적지에 선택한 Pawn으로 진입하는지 확인한다. | 사람 | 대기 | 자동 회귀에서는 실제 맵 이동 전에 테스트 월드를 정리한다. |

## 검증 근거

- **일시**: 2026-09-25 23:14 KST. 기준 커밋의 지적을 현재 작업 트리에 반영하고 사용자 단순화 지시를 적용한 뒤 검증했다.
- **최종 빌드 로그**: `C:/Wx/Saved/Logs/BuildDoctor/build_2026-09-25_232031_628_31828.log` — `Result: Succeeded`, `BUILD_DOCTOR_EXIT_CODE=0`. 추가 승인한 UI 테스트 소스까지 제거한 최종 빌드다. 컴파일·링크 성공, 종료 코드 0.
- **최종 회귀 보고서**: `C:/Wx/Saved/Automation/WxGameReview/20260925_231327/Report/index.json` — 수명 훅 제거 후 AbilityReregister·DeathNotification 2개 성공, 오류·경고·미실행 0. 두 차례 재등록의 스펙·인스턴스 복원, HP/SP·효과 핸들 유지, 사망 및 AI 이벤트 구독 일회성을 확인했다.
- **새 게임 검증 이력**: `C:/Wx/Saved/Automation/WxGameReview/20260925_225721/Report/index.json`의 NewGameValidation 성공. 이후 새 게임 구현은 바꾸지 않았으므로 이번 재실행에서는 제외했다.
- **저장 격리**: `-UserDir=C:/Wx/Saved/Automation/WxGameReview/20260925_222914/User`. 테스트는 이 전용 루트 안에서만 체크포인트를 생성·삭제한다.
- **테스트 소스 제거**: `Source/WxGame/Tests/WxGameReviewTests.cpp`를 검증 후 제거했고, 추가 승인에 따라 기존 UI 회귀 검사 3개가 있는 `Source/WxGame/Tests/WxUIPresentationTests.cpp`도 제거했다. 위 회귀 결과는 삭제 전 구현의 검증 이력이다. WxGame 소스에서 자동화 테스트 선언·조건부 코드·테스트 헤더 참조가 남지 않았음을 확인했다.
- **한계**: 자동 회귀는 등록·초기화·태그·저장 API의 동작을 확인한다. 실제 스트리밍 맵의 공격·래그돌 연출, 네트워크 PIE 및 프런트엔드의 목적지 진입은 이 실행 결과에 포함하지 않는다.

빌드 재실행은 `.agents/skills/build-doctor/scripts/Invoke-WxEditorBuild.ps1 -ProjectRoot C:/Wx`를 사용한다. 최종 실행기가 출력한 실제 명령:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" WxEditor Win64 Development "-Project=C:\Wx\Wx.uproject" -WaitMutex -NoHotReloadFromIDE
```

## 현재 정적 리뷰

캐릭터 재초기화·사망 통지·새 게임 선택과 UI 조립 경로를 현재 소스로 대조했다. 아래 검토 범위에서는 새로운 확정적 결함을 찾지 못했다. 위 승인 원문과 사람 확인 대기 항목은 유지하며, 과거 빌드·회귀 결과를 이번 실행 결과로 간주하지 않는다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 0 |
| 🟡 개선 | 0 |
| 🟢 사소 | 0 |

## 결과

현재 미해결로 유지할 코드 지적은 없다. 기존 세 지적은 위 수정 내용과 현재 소스를 대조해 해소를 확인했으므로 이전 정적 리뷰 원문을 제거한다. 코드 리뷰와 실제 레벨 재표시·새 게임 진입의 사람 검증은 여전히 필요하다.

## 검토 범위

- **깊게 본 파일**: `Source/WxGame/Character/WxCharacterBase.h`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Controller/WxNameplateManagerComponent.h`, `Source/WxGame/Controller/WxNameplateManagerComponent.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.h`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/Battle/WxBattleSubsystem.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Ability.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_AbilitySystem.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_PlayerCharacter.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Quest.cpp`이다.
- **훑은 파일**: `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`이다. 헤더의 인라인 정의를 검색했다.
- **교차 근거**: 기존 재등록 지적의 해결 여부 확인에 한해 `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:48`과 `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySet.cpp:58`의 최초 속성·GE 초기화와 누락 스펙 부여 분리를 대조했다.
- **미검토 / 한계**: 2026-09-26 작업 트리의 정적 리뷰이다. 소스 61개는 생성물 제외 `.h`·`.cpp` 개수이며 전 파일 정밀 검토를 뜻하지 않는다. 이번 리뷰에서 빌드·자동화 테스트·게임 실행을 다시 수행하지 않았으며, BP/WBP·DataTable·BT·StateTree 내부 및 멀티플레이 동작은 검증하지 않았다. 상단 테스트 결과는 이전 구현 작업의 검증 이력이다.

---
*문서 기준 커밋 `ad0db6de0` · 리뷰일 2026-09-26 · 소스 61파일 — `/module-review`로 갱신*
