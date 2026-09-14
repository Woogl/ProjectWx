# Hit Wrapper 리뷰 회귀 수정

## 계획

- 사용자가 승인한 두 리뷰 항목을 수정한다. 기존 Wrapper 선판정과 피해 후 반응 순서를 유지한다.
- 회피 어빌리티에서 순정 GAS NetworkSyncPoint(OnlyClientWait)를 사용한다. 소유 클라이언트는 회피 활성화 중 성공 신호를 기다리고, 서버는 회피 성공 시 동일 회피 활성화의 클라이언트로 신호를 보낸다. 로컬 성공과 확정 신호가 겹쳐도 몽타주 전환은 회피당 한 번만 처리한다.
- 타격 Cue는 권위 측에서 실제 피해 출력 또는 퍼펙트 가드 반사 출력이 존재할 때만 실행한다. 클라이언트 예측 Cue와 반사량 0인 퍼펙트 가드는 보존한다.
- 기존 테스트에서 Cue 라우팅을 관찰하도록 회귀 검증을 확장하고, 회피 성공 신호의 수명·활성화 키 구분을 가능한 범위에서 검증한다. UE 5.8 WxEditor Development 빌드와 관련 자동 테스트를 실행한다.

## 완료

- 회피의 소유 클라이언트는 `NetworkSyncPoint(OnlyClientWait)`로 서버 성공 신호를 기다린다. 서버의 `HandleDodgeSuccess`는 원격 플레이어의 동일 회피 활성화에 신호를 보내고 즉시 기존 몽타주 처리를 계속한다. 공격자의 예측 키와 무관하며 별도 커스텀 RPC는 추가하지 않았다.
- 회피당 `bDodgeSuccessHandled`를 초기화하여 로컬 이벤트와 서버 확정이 겹쳐도 중복 전환을 막았다. 대기 태스크는 회피 어빌리티 소유로 생성한다.
- 엔진 `AbilityTask_NetworkSyncPoint.cpp`에서 OnlyClientWait의 ClientSetReplicatedEvent 전송, SpecHandle·ActivationPredictionKey별 대기, 선도착 신호 소비 경로를 확인했다. 실제 네트워크 왕복·회피 몽타주 전환은 PIE로 실측하지 않았다.
- Hit Cue는 권위 측에서 양수 IncomingDamage 출력 또는 IncomingReflect 출력이 있을 때만 실행한다. 비권위 예측 Cue와 반사량 0인 퍼펙트 가드 Cue는 유지한다.
- 기존 `Wx.Combat.HitWrapper.Application`에 실제 Cue 라우팅 관찰을 추가했다. Cue 억제를 해제한 상태에서 0 피해 Hit Cue 0회, 반사량 0인 퍼펙트 가드 1회, 정상 피해 1회를 검증했다. 기존 타격 판정·스탯·이벤트 검증도 함께 통과했다.
- UE 5.8.2 WxEditor Win64 Development 빌드 성공(종료 코드 0). 로그: `C:/Wx/Saved/Logs/BuildDoctor/build_2026-09-14_215917_991_7556.log`.
- 자동 테스트 성공: 테스트 1개, 실패 0, 테스트 경고 0. 로그: `C:/Wx/Saved/Logs/HitWrapperReviewFixes.log`, 보고서: `C:/Wx/Saved/Automation/HitWrapperReviewFixes/index.json`.
- `git diff --check` 통과. 판정 수식·HitReact 순서·추가 GE 정책·어빌리티 NetExecutionPolicy는 변경하지 않았다.
- 빌드 재현: `& 'C:/Wx/.agents/skills/build-doctor/scripts/Invoke-WxEditorBuild.ps1' -ProjectRoot 'C:/Wx'`. 실제 명령: `& 'C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat' WxEditor Win64 Development '-Project=C:/Wx/Wx.uproject' -WaitMutex -NoHotReloadFromIDE`.
