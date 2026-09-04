# BossComponent 보스전 HUD 상태 이관

## 계획

- `UWxBossComponent` 클래스 이름은 유지한다.
- 컴포넌트의 교전 상태 조회는 `IsEngaged()`를 사용하고, HUD 표시 상태는 `bBossBattleActive`로 구분한다.
- 보스의 AI 타깃 변경 및 사망 생명주기 구독을 `UWxViewModel_BossCharacter`에서 `UWxBossComponent`로 옮기고, 컴포넌트가 상태 변경 신호를 발행하게 한다.
- 뷰모델은 보스 컴포넌트를 선택하고 컴포넌트 상태를 FieldNotify로 중계하는 UI 어댑터 역할만 맡긴다. HUD 위젯 생성과 CommonUI 레이어 관리는 기존 HUD 경로에 남긴다.
- 현재 별도의 보스룸 진입 신호가 없으므로 표시 시작·종료 시점은 기존 AI 타깃 유무 기준을 보존한다. 이후 보스룸 트리거가 생기면 컴포넌트 내부 상태 입력만 교체할 수 있게 한다.
- 직렬화된 `WBP_Nameplate_Boss` MVVM 바인딩을 새 상태 필드로 맞추고, 기존 표시 동작이 유지되는지 확인한다.
- 변경 후 `WxEditor` Development 타겟을 빌드하고, 실패하면 `build-doctor` 절차로 최초 인과 오류를 진단한다.

## 완료

- `UWxBossComponent`에 `IsEngaged()`, `OnEngagementChanged`, `OnBossEndPlay`를 추가했다.
- `IsEngaged()` 확정에 맞춰 컴포넌트 내부 상태와 변경 신호도 `bEngaged`·`OnEngagementChanged`로 통일하고, HUD 뷰모델의 `bBossBattleActive`는 표시 의미로 분리해 유지했다.
- 보스의 AI 타깃 변경 및 사망 구독과 상태 계산을 컴포넌트로 옮겼다. `EndPlay`에서 구독과 상태를 정리하고 HUD 관찰자에게 종료를 알린다.
- `UWxViewModel_BossCharacter`는 적 캐릭터의 락온/EndPlay를 직접 구독하지 않고, 현재 보스 컴포넌트의 상태와 생명주기만 FieldNotify로 중계하도록 정리했다.
- 뷰모델의 표시 필드를 `bBossBattleActive`로 변경했다. 실행 중인 에디터가 `WBP_Nameplate_Boss`를 점유해 자동 재저장이 불가능했으므로, 사용자가 에디터에서 바인딩을 확인·저장하기 전까지 기존 `bHasAITarget` 바인딩을 새 필드로 해석하는 `PropertyRedirects`를 남겼다.
- UE 5.8 `WxEditor Win64 Development` 전체 빌드 성공: `Saved/Logs/BuildDoctor/build_2026-09-04_204907_543_31292.log`.
- 최종 명칭 변경 후 UE 5.8 `WxEditor Win64 Development` 재빌드도 성공했다.
- 에디터 후속 작업: 새 DLL로 에디터를 다시 연 뒤 `WBP_Nameplate_Boss`의 Visibility 소스를 `bBossBattleActive`로 확인하고 컴파일·저장한다. 저장 후 임시 `PropertyRedirects`를 제거할 수 있다.
- PIE에서 AI 타깃 획득/해제 및 보스 사망 시 HUD 표시 전환 확인은 에디터 후속 작업과 함께 진행한다.
