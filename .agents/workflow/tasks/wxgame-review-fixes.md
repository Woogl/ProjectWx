# WxGame 코드 리뷰 지적 해결

상태: 완료 · 체크리스트 9/9 통과
다음 행동: 변경 시 기록된 테스트 범위와 제약을 참고한다.

- 계기: [WxGame — 코드 리뷰](module_review_WxGame.md)의 지적 세 개를 해결한 구현 작업이다. 2026-09-26 워크플로우 종합 점검(D6)에서 리뷰 문서에 있던 구현 부분을 이 기록으로 옮겼다.
- **요청**: 사용자(2026-09-25) — “WxGame — 코드 리뷰 의 지적사항을 모두 올바른 방법으로 해결해주세요.”
- **후속 지시**: 사용자(2026-09-25) — “테스트 코드 제거 후 제출해주세요.”, “EWxAbilitySetGrantState  이라는 State가 추가된 것도 마음에 안들어요”. 추가로 “OnRegister, Unregister 자체를 아예 override하지 않았으면 해요. EndPlay도요”, “단순하고 직관적인 코드가 좋거든요”를 지시했다. 상태 enum과 이번 작업에서 추가한 수명 훅을 제거하고 실제 스펙 보유 여부로 판단한다. 검증용 테스트 코드를 제거해 제출한다.
- **추가 삭제 승인**: 사용자(2026-09-25) — 기존 UI 테스트 `Source/WxGame/Tests/WxUIPresentationTests.cpp`의 회귀 검사 3개도 삭제할지 확인한 뒤 “네, 제거하고 제출합시다”라고 승인했다. 해당 파일을 제거하고 재제출한다.
- **범위**: 아래 세 지적을 현재 작업 트리에 반영한다. 같은 재진입 테스트에서 드러난 WxAI 행동 컴포넌트의 컨트롤러 변경·피격 이벤트 중복 구독 방지도 포함한다. 다른 작업의 UI 리졸버·표시 계약과 Exclusive 정책 변경은 보존한다.
- **수정 방향**: ASC의 최초 속성·효과 초기화와 등록 해제 후 기본 어빌리티 재부여를 구분한다. 태그 구독은 중복을 막고 처치 통지를 객체 수명당 한 번으로 제한한다. 선택 Pawn 클래스를 저장 삭제 전에 검증하고 목적지까지 유지한다.

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| Editor Development 빌드 | build-doctor 실행기 | AI | 통과 | `Saved/Logs/BuildDoctor/build_2026-09-25_232031_628_31828.log`: Succeeded, 종료 코드 0 |
| WxGame 테스트 코드 제거 | 자동화 테스트 선언·조건부 코드·테스트 헤더 참조 검색 | AI | 통과 | WxGame 소스 검색 결과 0건, UI 표시 테스트 파일 삭제 확인 |
| 어빌리티 재등록 | `Wx.Game.Review.AbilityReregister`: 두 차례 등록 왕복, 스펙·인스턴스 복원과 HP·SP·효과 핸들 보존 | AI | 통과 | 최종 회귀 보고서: Success, 오류·경고 0 |
| 태그 구독·처치 일회성 | `Wx.Game.Review.DeathNotification`: 반복 초기화 후 사망·래그돌 구독 각각 1개, 사망 태그 재적용과 시체 BeginPlay 재진입의 처치 통지 1회 | AI | 통과 | 최종 회귀 보고서: Success, 오류·경고 0; 재진입 두 차례 뒤 AI 피격 구독도 1개 |
| 새 게임 실패 시 저장 보존 | `Wx.Game.Review.NewGameValidation`: 없는 경로·추상 클래스·잘못된 부모 거절, 유효 선택만 저장 삭제 | AI | 통과 | 이전 회귀 보고서(20260925_225721): Success, 오류·경고 0; 이후 해당 구현 변경 없음, 전용 UserDir에서 실행 |
| 코드 리뷰 | WxAbilitySystemComponent·WxAbilitySet의 누락 스펙 보충, WxAIBehaviorComponent의 단일 구독, 캐릭터 사망 일회성, WxGameFlowSubsystem의 저장 삭제 전 검증을 확인한다. 2026-09-27 추가: WxCombat `AWxWeaponBase::PostInitializeComponents`의 겹침 구독을 `AddDynamic` → `AddUniqueDynamic`(레벨 재표시 때 같은 처리기가 두 번 붙어 엔진 ensure가 나던 것) | 사람 | 통과 | woogle 2026-09-28 |
| 레벨 재표시 전투 | 헤드리스 게임(LV_DevCombat -game -nullrhi): 적이 든 서브레벨을 로드 유지한 채 두 번 숨겼다 표시한다. 같은 적이 능력·효과·HP를 유지하고, 다시 보인 뒤 플레이어를 공격하고 피격·사망이 동작하며, 처치 통지·보상은 한 번이고 래그돌이 된다. 엔진 ensure가 없다 | AI | 통과 | 2026-09-27 임시 자동화 테스트와 임시 서브레벨(적 1명, 확인 뒤 삭제): 숨김마다 능력 7→0, 표시마다 7로 재부여, 효과 중복 없음, HP 80 유지, 사망·래그돌 구독 1개. 재표시 뒤 공격(플레이어 HP 72→65)·피격(80→70)·사망(처치 통지 1회, 보상 100 = 대조군 100, 래그돌). 처음 실행에서 무기 겹침 구독 중복 ensure를 찾아 고쳤고 고친 뒤 ensure 0 |
| 재표시 뒤 연출 | 게임: 적이 있는 레벨을 숨겼다 다시 보인 뒤 적의 공격·피격 모션과 사망 래그돌이 자연스럽다 | 사람 | 통과 | woogle 2026-09-28 |
| 새 게임 캐릭터 선택 | 헤드리스 게임(LV_FrontEnd -game -nullrhi): 프런트엔드에서 New Game → 캐릭터 BP_HGTest → 레벨 LV_DevCombat → 확인 Yes를 누르면 목적지에 선택한 Pawn으로 들어간다 | AI | 통과 | 2026-09-27 임시 자동화 테스트(확인 뒤 삭제): 버튼마다 CommonUI의 실제 클릭 처리기로 누름. LV_DevCombat 로드 뒤 플레이어 폰 BP_HGTest_C_0(클래스 BP_HGTest_C), 게임 흐름의 선택 폰도 BP_HGTest_C. 기본값이 아닌 캐릭터라 선택이 전달됨을 가림 |

## 구현

- **구현**: 최초 속성·GE 초기화와 어빌리티 부여 수명을 분리했다. 캐릭터 태그와 ASC SP 구독 중복을 막았고, 사망 처리 및 적의 BeginPlay 보상 재진입을 객체 수명당 한 번으로 제한했다. 선택 Pawn은 삭제 전에 검증하고 강한 참조로 유지한다.
- **자체 검증**: 수명 훅 제거 후 어빌리티 재등록·사망 및 AI 구독 회귀 2개 통과(오류·경고·미실행 0). 새 게임 저장 보존 검사는 변경 없는 구현의 이전 통과 결과를 유지한다. 테스트 코드를 제거한 최종 Editor Development 빌드도 통과했다.

1. **ASC 재등록** — `GiveAbilitySets`는 실제 보유한 스펙을 확인해 없는 어빌리티만 채운다. `bAbilitySetsInitialized` 하나로 속성·GE를 최초 한 번만 적용하며 HP/SP 초기화와 효과 중복을 막는다. 미등록·비권위 ASC는 부여하지 않는다. SP 변화 구독은 객체당 한 번이다. 부여 상태 enum·`bAbilitySetsGranted`·`OnRegister`·`OnUnregister`·`DestroyActiveState` 오버라이드를 추가하지 않는다.
2. **사망·이벤트 구독** — 캐릭터 사망·래그돌 태그 구독을 객체당 한 번으로 제한한다. `bDeathHandled`와 `bDeathNotified`로 사망 처리 및 시체 BeginPlay 재진입의 처치·보상 중복을 막는다. 부활은 새 Pawn을 생성하는 기존 계약을 따른다. WxAI의 컨트롤러 이벤트는 `AddUniqueDynamic`, 피격 이벤트는 유효한 구독 핸들이 없을 때만 등록한다. 콜백은 플레이 중에만 처리하므로 새 `EndPlay` 오버라이드가 필요 없다. 기존 적 캐릭터의 교전 정리용 EndPlay는 이번 변경 대상이 아니다.
3. **새 게임 검증 순서** — 체크포인트 삭제 전에 선택 클래스를 동기 로드하고 Pawn 상속 및 Abstract·Deprecated·NewerVersionExists 플래그를 확인한다. 실패하면 상태 문구만 갱신하고 요청을 거절한다. 유효한 클래스는 `UPROPERTY TSubclassOf<APawn>`으로 유지하여 목적지에서 다시 로드하지 않는다.

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

## 헤드리스 테스트로 옮김 · 2026-09-27

- 계기: 작업 절차의 헤드리스 규칙에 따라 사람 항목 두 개(레벨 재표시, 새 게임 캐릭터 선택)를 AI 항목으로 옮기고 AI가 끝까지 테스트했다(사용자 결정: `workflow-recheck.md` Q2 "지금 한 건씩 차례로"). 재표시 뒤의 모션·래그돌 모양은 사람 항목 「재표시 뒤 연출」로 남겼다.
- 레벨 재표시 방법: 게임 맵이 모두 World Partition이라 일반 스트리밍 서브레벨이 없다. 그래서 적 1명만 둔 임시 레벨(`/Game/Tests/LV_VerifyStreamedEnemy`, 헤드리스 파이썬으로 생성)을 `ULevelStreamingDynamic::LoadLevelInstance`로 불러, 적이 플레이어를 마주 보게 두고 `SetShouldBeVisible` false/true를 두 번 반복했다. 보상 1회는 한 번도 숨기지 않은 대조군 적의 보상량(100)과 비교했다.
- 발견·수정: 처음 실행에서 서브레벨을 다시 보일 때 `AWxWeaponBase::PostInitializeComponents`(WxCombat)가 무기 겹침 처리기를 한 번 더 등록해 엔진 ensure(`InvocationList[...] != InDelegate`)가 났다. 엔진은 ensure 뒤에도 등록을 더해 겹침 처리기가 여러 번 불린다. 이 작업이 AI 컨트롤러 이벤트에 쓴 방식과 같게 `AddUniqueDynamic`으로 고쳤다(이미 승인된 범위: 레벨 재표시 때 중복 구독 방지). 다른 액터의 `PostInitializeComponents`에는 같은 패턴이 없다.
- 확인한 사실: 숨기면 적의 AI 컨트롤러가 떨어지고 능력 스펙이 비며, 다시 보이면 새 AI 컨트롤러가 빙의해 능력이 다시 부여된다. 작은 피해(10)에는 대조군도 다시 보인 적도 피격 반응 태그가 붙지 않았다.
- 새 게임 흐름: 프런트엔드는 캐릭터·레벨 버튼을 고르면 확인 팝업(Yes/Cancel)이 뜨고, Yes를 눌러야 이동한다.
- 확인: 고친 뒤 새 프로세스에서 두 테스트 모두 Result=Success, ensure 0. 임시 테스트·임시 레벨을 지운 뒤 WxEditor Development 빌드가 성공했다.


## 사용자 테스트 결과 · 2026-09-28T13:02:30.191Z

<!-- test-feedback:request-99144b98-af67-4ae4-b97f-5f38ccc7ac28:submitted -->
- 전달한 사람: woogle

> 통과 · 코드 리뷰
> 통과 · 재표시 뒤 연출
