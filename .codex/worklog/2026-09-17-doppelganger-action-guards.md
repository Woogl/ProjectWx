# 도플갱어 행동 중 텔레포트 및 4타 소환 제한

## 계획

- 사용자 요청 범위에 따라 BT MirrorMovement에서 Master 또는 분신의 활성 어빌리티가 있으면 강제 텔레포트와 타이머 누적을 막는다. 행동 종료 후 지상 추종 시간을 다시 1초부터 센다.
- 기존 소환 노티파이에 특정 활성 소환물 클래스가 있으면 건너뛰는 선택적 조건을 추가한다. Attack_Light 4타에만 BP_Doppelganger를 차단 클래스로 지정한다.
- 소환수 로스터의 기존 질의를 클래스 필터로 확장해 실제 활성 소환물 기준으로 검사한다. 기존 WxAbility 계열 클래스는 수정하지 않는다.
- 기존 BT 통합 테스트에 행동 중 1초 초과 텔레포트 방지/종료 후 재개, 도플갱어 보유 중 4타 소환 차단/부재 시 소환/일반 미니언 보유 시 기존 교체를 추가한다. UE 5.8 WxEditor Development 빌드와 에셋 저장 검증을 수행한다.

## 완료

- MirrorMovement에서 Master와 분신 양쪽 ASC의 활성 스펙을 확인한다. 가드·질주처럼 유지되는 어빌리티도 포함하며, 활성 중에는 텔레포트 타이머를 초기화한다. 종료 후 연속 지상 추종 1초가 지나야 텔레포트한다.
- WxAnimNotify_SpawnMinion에 선택적 `BlockingMinionClass`를 추가하고 기존 로스터 질의 `FindActiveMinion`에 클래스 필터를 추가했다. 필터가 없으면 기존 반환 동작을 유지한다.
- `AM_HGTest_Attack_LLLL`의 BP_Minion 소환 노티파이에 BP_Doppelganger를 차단 클래스로 저장했다. 궁극기 소환은 제한하지 않는다. 저장 결과: `Saved/DoppelgangerActionGuards.json`.
- 기존 WxAbility 계열 소스에 HEAD 대비 변경이 없음을 확인했다.
- UE 5.8 WxEditor / Win64 / Development 빌드 성공: `Saved/Logs/BuildDoctor/build_2026-09-17_010302_751_20944.log`.
- 통합 테스트 `Wx.Combat.Doppelganger.MasterReplay` 성공. 1.5초 가드 중 텔레포트 방지, Master가 쉬고 분신만 가드하는 경우의 방지, 종료 후 타이머 재시작과 1초 후 텔레포트를 검증했다. 저장된 4타 노티파이를 통해 도플갱어 보유 시 소환 차단/도플갱어 유지, 부재 시 BP_Minion 소환, 일반 미니언 보유 시 기존 교체, 궁극기의 도플갱어 재소환도 검증했다. 기존 이동·어빌리티 복제 및 소환 수명 검증도 함께 통과했다.
- 테스트 로그: `Saved/Logs/DoppelgangerActionGuardsTest.log`. 보고서: `Saved/Automation/DoppelgangerActionGuards/index.json`. 이전 검증에서 명시한 기존 데이터 누락 진단은 동일한 예상 횟수로 분리했다.
