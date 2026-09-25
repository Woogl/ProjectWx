# WxGame — 코드 리뷰

상태: 확인 대기 · 단순화·테스트 코드 제거 후 빌드와 자동 회귀 통과
다음 행동: 코드 리뷰와 실제 레벨 재표시·새 게임 진입을 확인한다.

## 현재 작업

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
| Editor Development 빌드 | build-doctor 실행기 | AI | 통과 | `Saved/Logs/BuildDoctor/build_2026-09-25_231523_728_41392.log`: Succeeded, 종료 코드 0 |
| 어빌리티 재등록 | `Wx.Game.Review.AbilityReregister`: 두 차례 등록 왕복, 스펙·인스턴스 복원과 HP·SP·효과 핸들 보존 | AI | 통과 | 최종 회귀 보고서: Success, 오류·경고 0 |
| 태그 구독·처치 일회성 | `Wx.Game.Review.DeathNotification`: 반복 초기화 후 사망·래그돌 구독 각각 1개, 사망 태그 재적용과 시체 BeginPlay 재진입의 처치 통지 1회 | AI | 통과 | 최종 회귀 보고서: Success, 오류·경고 0; 재진입 두 차례 뒤 AI 피격 구독도 1개 |
| 새 게임 실패 시 저장 보존 | `Wx.Game.Review.NewGameValidation`: 없는 경로·추상 클래스·잘못된 부모 거절, 유효 선택만 저장 삭제 | AI | 통과 | 이전 회귀 보고서(20260925_225721): Success, 오류·경고 0; 이후 해당 구현 변경 없음, 전용 UserDir에서 실행 |
| 코드 리뷰 | WxAbilitySystemComponent·WxAbilitySet의 누락 스펙 보충, WxAIBehaviorComponent의 단일 구독, 캐릭터 사망 일회성, WxGameFlowSubsystem의 저장 삭제 전 검증을 확인한다. | 사람 | 대기 | 사용자 단순화 지시를 반영한 최종 구현 확인 |
| 레벨 재표시 전투·연출 | 게임: 적이 있는 로드 유지 서브레벨을 두 번 숨겼다 표시한다. 공격·피격·사망이 동작하고 래그돌·보상이 중복되지 않는지 확인한다. | 사람 | 대기 | 자동 회귀는 컴포넌트·액터 수명 경로이며 실제 스트리밍 맵·연출 확인은 별도 |
| 새 게임 캐릭터 선택 | 프런트엔드에서 유효 캐릭터를 선택해 목적지에 선택한 Pawn으로 진입하는지 확인한다. | 사람 | 대기 | 자동 회귀에서는 실제 맵 이동 전에 테스트 월드를 정리한다. |

## 검증 근거

- **일시**: 2026-09-25 23:14 KST. 기준 커밋의 지적을 현재 작업 트리에 반영하고 사용자 단순화 지시를 적용한 뒤 검증했다.
- **최종 빌드 로그**: `C:/Wx/Saved/Logs/BuildDoctor/build_2026-09-25_231523_728_41392.log` — `Result: Succeeded`, `BUILD_DOCTOR_EXIT_CODE=0`. 테스트 소스 제거 후 최종 빌드다. 앞선 링크는 다른 헤드리스 에디터 실행과 겹쳐 LNK1104로 실패했으며 해당 실행 종료 후 소스 변경 없이 재실행해 성공했다.
- **최종 회귀 보고서**: `C:/Wx/Saved/Automation/WxGameReview/20260925_231327/Report/index.json` — 수명 훅 제거 후 AbilityReregister·DeathNotification 2개 성공, 오류·경고·미실행 0. 두 차례 재등록의 스펙·인스턴스 복원, HP/SP·효과 핸들 유지, 사망 및 AI 이벤트 구독 일회성을 확인했다.
- **새 게임 검증 이력**: `C:/Wx/Saved/Automation/WxGameReview/20260925_225721/Report/index.json`의 NewGameValidation 성공. 이후 새 게임 구현은 바꾸지 않았으므로 이번 재실행에서는 제외했다.
- **저장 격리**: `-UserDir=C:/Wx/Saved/Automation/WxGameReview/20260925_222914/User`. 테스트는 이 전용 루트 안에서만 체크포인트를 생성·삭제한다.
- **테스트 소스 제거**: 사용자 요청으로 `Source/WxGame/Tests/WxGameReviewTests.cpp`를 검증 후 제거했다. 위 회귀 결과는 제거 직전의 구현 검증 이력이며, 제출본에는 이 테스트가 포함되지 않는다. 다른 작업의 테스트는 변경하지 않았다.
- **한계**: 자동 회귀는 등록·초기화·태그·저장 API의 동작을 확인한다. 실제 스트리밍 맵의 공격·래그돌 연출, 네트워크 PIE 및 프런트엔드의 목적지 진입은 이 실행 결과에 포함하지 않는다.

빌드 재실행은 `.agents/skills/build-doctor/scripts/Invoke-WxEditorBuild.ps1 -ProjectRoot C:/Wx`를 사용한다. 최종 실행기가 출력한 실제 명령:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" WxEditor Win64 Development "-Project=C:\Wx\Wx.uproject" -WaitMutex -NoHotReloadFromIDE
```

## 최초 리뷰 이력

아래는 사용자 지정 커밋 `39f3629a4`의 원래 지적이며, 현재 수정 상태는 위 기록과 테스트 체크리스트를 따른다.

<details>
<summary>수정 전 정적 리뷰 원문 — 심각 1·개선 2</summary>

> 캐릭터·컨트롤러·도메인 연결은 비교적 명확하나, 동일 캐릭터가 레벨 가시성 전환으로 재초기화되는 경로에서 어빌리티 소실과 구독 누적이 남아 있다. 캐릭터 수명, 같은 월드 부활·새 게임 실패 경로, 보스·인벤토리·대화·퀘스트 VM 연결을 중심으로 검토했다.

## 요약

| 심각도 | 개수 |
| --- | --- |
| 🔴 심각 | 1 |
| 🟡 개선 | 2 |
| 🟢 사소 | 0 |

## 결과

### 1. 🔴 숨겼다가 다시 표시한 레벨의 적이 어빌리티를 잃는다

- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:210`, `Plugins/WxCombat/Source/WxCombat/Private/AbilitySystem/WxAbilitySystemComponent.cpp:49`
- **범주**: 버그/정확성
- **문제**: 스트리밍 레벨을 로드된 상태로 유지하면서 가시성만 껐다 켜면 같은 캐릭터와 ASC가 재사용된다. UE 5.8은 숨길 때 ASC의 `OnUnregister` → `DestroyActiveState`에서 서버의 `ClearAllAbilities()`를 호출한다. 다시 표시할 때 적은 `PostInitializeComponents`에서 새 AIController에 빙의되고 `InitAbilitySystem`이 다시 실행되지만, ASC의 `bAbilitySetsGranted`가 여전히 `true`여서 부여를 건너뛴다. 결과적으로 적은 이동할 수 있어도 공격·피격·사망 어빌리티가 사라진다. `Event.Death`에 반응할 스펙도 없어 처치·보상 흐름까지 끊어진다. 객체를 실제 언로드·수거한 뒤 새로 생성하는 경우는 이 재현 조건에 포함하지 않는다.
- **제안**: 수정 주체는 WxCombat의 ASC 부여 상태이다. 일반 재빙의의 중복 부여 방지는 유지하면서, 엔진이 활성 상태와 스펙을 제거한 뒤 재등록되는 경우 필요한 스펙·효과를 복원하도록 상태를 구분한다. 단순히 bool을 내리고 AbilitySet 전체를 다시 적용하면 속성 초기화·남은 GE 중복이 생길 수 있으므로 기존 속성·효과의 보존 계약도 함께 정한다. 로드 유지 → 숨김 → 재표시 후 공격과 사망을 검증한다.
- **확신도**: 높음

### 2. 🟡 캐릭터 재초기화마다 사망·래그돌 콜백이 추가된다

- **위치**: `Source/WxGame/Character/WxCharacterBase.cpp:58`, `Source/WxGame/Character/WxCharacterBase.cpp:63`
- **범주**: 버그/정확성
- **문제**: `PostInitializeComponents`에서 두 태그 이벤트에 무조건 `AddUObject`를 호출하고 대응 해제나 중복 검사가 없다. UE 5.8은 레벨 제거 시 `bActorInitialized`를 내리고 동일 객체 재표시 시 이 함수를 다시 호출한다. ASC의 등록 해제·재초기화는 이 태그 델리게이트를 지우지 않는다. 따라서 위 1번과 같은 가시성 왕복마다 구독이 누적되고, 이후 해당 태그가 0에서 양수로 바뀌면 `HandleDeath`·`EnterRagdoll`이 여러 번 실행된다. 사망 콜백은 `OnDeath.Broadcast`를 거쳐 권위 측 `HandleOwnerDeath`의 처치 통지와 보상 지급까지 반복한다(`WxCharacterBase.cpp:283`, `WxEnemyCharacter.cpp:140`). 다만 현재 일반 서버 경로에서는 1번의 사망 스펙 소실이 먼저 발생할 수 있어, 가시성 왕복 뒤 평범한 공격만으로 보상이 반드시 두 배 된다고 단정하지 않는다.
- **제안**: SPD 구독처럼 객체별 중복 검사를 하거나, 델리게이트 핸들을 보관하여 등록·해제를 대칭으로 만든다. 사망의 일회 처리도 명시한다. 같은 객체에서 가시성을 두 번 왕복한 뒤 태그를 한 번 전이시켜 콜백·처치 통지가 한 번인지 확인한다.
- **확신도**: 높음

### 3. 🟡 선택한 캐릭터를 로드할 수 없어도 체크포인트를 먼저 삭제한다

- **위치**: `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp:43`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp:50`
- **범주**: 버그/정확성
- **문제**: 캐릭터 선택은 소프트 경로의 `IsNull()`만 검사한다. 설정에 남은 잘못된 클래스 경로나 생성할 수 없는 추상 Pawn 클래스도 통과하여 기존 체크포인트 슬롯을 삭제하고 레벨 이동을 성공으로 접수한다. 실제 클래스 로드는 목적지의 `GetSelectedPawnClass`에서야 일어나며(`WxGameFlowSubsystem.cpp:79`), 로드 실패 시 GameMode는 선택 실패를 알리는 대신 기본 Pawn으로 진행한다(`WxGameMode.cpp:19`). 사용자는 잘못된 캐릭터로 시작하거나 Pawn 생성에 실패하면서 이전 체크포인트도 잃는다. 현재 설정의 두 캐릭터 에셋이 깨졌다는 주장은 아니며, 공개 새 게임 API의 실패 처리 결함이다.
- **제안**: 슬롯 삭제 전에 선택 클래스를 로드하고 Pawn 상속과 생성 가능 여부를 검사한다. 실패하면 상태 문구를 반환하고 요청을 거절한다. 유효한 선택으로 수락한 뒤 클래스 로드 실패를 기본 Pawn으로 조용히 대체하는 경로도 구분한다.
- **확신도**: 높음

## 검토 범위

- **깊게 본 파일**: `Source/WxGame/Character/WxCharacterBase.h`, `Source/WxGame/Character/WxCharacterBase.cpp`, `Source/WxGame/Character/WxEnemyCharacter.h`, `Source/WxGame/Character/WxEnemyCharacter.cpp`, `Source/WxGame/Character/WxPlayerCharacter.cpp`, `Source/WxGame/Controller/WxAIController.cpp`, `Source/WxGame/Controller/WxNameplateManagerComponent.h`, `Source/WxGame/Controller/WxNameplateManagerComponent.cpp`, `Source/WxGame/Framework/WxRespawnLibrary.cpp`, `Source/WxGame/Framework/WxGameMode.cpp`, `Source/WxGame/FrontEnd/WxGameFlowSubsystem.cpp`, `Source/WxGame/FrontEnd/WxFrontEndLibrary.cpp`, `Source/WxGame/FrontEnd/WxFrontEndDeveloperSettings.cpp`, `Source/WxGame/MVVM/WxViewModel_Inventory.h`, `Source/WxGame/MVVM/WxViewModel_Inventory.cpp`, `Source/WxGame/MVVM/WxViewModel_InteractionList.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Quest.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_Dialogue.cpp`, `Source/WxGame/MVVM/WxViewModelResolver_BossCharacter.cpp`, `Source/WxGame/Battle/WxBattleSubsystem.cpp`이다.
- **훑은 파일**: `Source/WxGame/WxGame.Build.cs`, `Source/WxGame/Controller/WxPlayerController.cpp`, `Source/WxGame/Framework/WxGameState.cpp`, `Source/WxGame/Character/WxNpc.cpp`, `Source/WxGame/Character/Component/WxCharacterMovementComponent.cpp`, `Source/WxGame/Character/Component/WxMetaHumanComponent.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_Interact.cpp`, `Source/WxGame/AbilitySystem/Ability/WxAbility_UseItem.cpp`, `Source/WxGame/Cheat/WxCheatManager.cpp`, `Source/WxGame/FrontEnd/WxFrontEndDeveloperSettings.h`이다. 나머지 헤더는 명명·인라인 정의를 검색했다.
- **교차 근거**: WxCombat의 ASC·사망 어빌리티, WxUI의 기본·캐릭터 VM과 PlayerLayout, WxWorld의 CheckpointSaveGame을 호출 경로 확인에 한해 읽었다. 로컬 UE 5.8 `Engine/Source/Runtime/Engine/Private/LevelStreaming.cpp:744`·`:1138`의 로드 유지와 가시성 전환, `World.cpp:4199`·`:4284`와 `Level.cpp:1606`의 등록 해제, `Actor.cpp:3238`·`Level.cpp:3876`의 재초기화, `Pawn.cpp:134`·`:556`의 재빙의 경로를 대조했다. GAS는 `Engine/Plugins/Runtime/GameplayAbilities/Source/GameplayAbilities/Private/AbilitySystemComponent.cpp:236`, `AbilitySystemComponent_Abilities.cpp:109`·`:1386`을 확인했다.
- **미검토 / 한계**: 기준 커밋 `39f3629a4`에 대한 정적 리뷰이다. 검토 중 다른 작업이 수정한 캐릭터·UIData 코드와 새 Ability 리졸버는 제외했고, 캐릭터 파일은 해당 커밋 원문으로 재대조했다. 소스 55개는 기준 커밋의 생성물 제외 `.h`·`.cpp` 개수이며 전 파일 정밀 검토를 의미하지 않는다. 빌드·게임 실행, BP/WBP·BT·StateTree 내부와 멀티플레이 실행을 검증하지 않았다. Wiki의 기존 재초기화 설명은 참고 후 엔진 소스로 재검증했으며, 사망·보상 중복의 조건은 위 2번으로 한정한다.

---
*문서 기준 커밋 `39f3629a4` · 리뷰일 2026-09-25 · 소스 55파일 — `/module-review`로 갱신*

</details>
