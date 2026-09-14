# ActionPhase 이벤트 UI 갱신

## 계획

- 승인된 전용 GameplayEvent 방식으로 `Event.Ability.ActivationStateChanged` 공용 태그를 WxCore에 추가한다.
- WxAbilityBase의 단계 변경을 Setter로 통합하고 실제 변경 시 해당 ASC에 로컬 이벤트를 보낸다. 단계 조회는 Getter로 제한한다.
- 각 Ability ViewModel이 같은 ASC의 이벤트를 구독하여 기존 일회성 예약으로 후보와 발동 상태를 재평가한다. 신규 모듈 의존성이나 폴링, 단계 복제는 추가하지 않는다.
- 태그·자원·쿨다운 변화 없이 콤보 창 개폐, 후딜, 재발동 초기화, 중복 단계 설정, 구독 해제를 회귀 검증하고 UE 5.8 WxEditor Development를 빌드한다.

## 완료

- WxCore에 전용 로컬 이벤트 태그를 추가하고, 발동 트리거로 사용하지 않는 알림 계약을 명시했다.
- ActionPhase를 비공개로 전환하고 Getter/Setter를 도입했다. 콤보 창 개폐, 후딜, 재발동 초기화는 실제 값 변경 시에만 이벤트를 보낸다. ASC의 외부 단계 조회도 Getter로 변경했다.
- 각 Ability ViewModel이 자기 ASC의 이벤트를 구독하며 태그 변경과 동일한 일회성 예약을 공유한다. 기존 FlushActivationRefresh가 후보 재선택 후 발동 상태를 다시 평가하므로 같은 후보여도 갱신된다. 해제 시 구독·예약을 정리한다.
- `Wx.UI.Ability.ActionPhaseRefresh` 자동화 테스트 성공, 종료 코드 0. 태그·비용·쿨다운 변화 없이 콤보 창 개폐, 다른 슬롯의 후딜 해제, 재발동 Blocking 복귀, 동일 단계 무통지, 예약 후 VM 해제를 검증했다.
- UE 5.8.2 WxEditor Win64 Development 빌드 성공. 수정 소스의 diff 공백 검사 통과.
- 빌드 로그: `C:/Wx/Saved/Logs/BuildDoctor/build_2026-09-15_010542_650_35284.log`
- 테스트 표준 출력: `C:/Wx/Saved/Logs/ActionPhaseTest-stdout.log`
- 테스트는 독립 로컬 월드에서 수행했다. 실제 네트워크 PIE는 실행하지 않았으며 이벤트나 ActionPhase 자체의 네트워크 복제는 이번 범위에 추가하지 않았다.
- 기존 이펙트 목록 수정 및 병행 작업은 보존했다.
- 후속 검토를 반영해 직접 이벤트 맵 접근 대신 `AddGameplayEventTagContainerDelegate` / `RemoveGameplayEventTagContainerDelegate`와 구독 핸들을 사용한다. 하위 태그 수신은 콜백의 정확 일치 검사로 제외한다. 아래 최종 검증 결과를 추가한다.
- 전용 등록·해제 API 반영 후 최종 빌드 성공: `C:/Wx/Saved/Logs/BuildDoctor/build_2026-09-15_010825_197_27128.log`. ActionPhaseRefresh 테스트 재실행 성공(종료 코드 0): `C:/Wx/Saved/Logs/ActionPhaseDelegateTest-stdout.log`.
- 후속 사용자 요청으로 ActionPhase 테스트 코드 두 파일을 제거했다. 앞의 테스트 통과 기록은 제거 전 실행 결과이다.
- 테스트 제거 후 WxEditor Development 빌드 성공: `C:/Wx/Saved/Logs/BuildDoctor/build_2026-09-15_011215_238_34676.log`.
