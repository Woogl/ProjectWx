# 도플갱어 어빌리티 종료 시 즉시 복귀

## 계획

- 사용자 요청에 따라 기존 BT 이동 서비스에서 Master 및 분신의 GAS 종료 통지를 구독한다. 종료 시 복귀를 예약하고, 두 캐릭터 모두 활성 어빌리티가 없어진 첫 BT 틱에 현재 Master 우측 100cm로 텔레포트한다.
- 어빌리티 활성 중에는 순간이동하지 않는다. 콤보 재발동이나 다른 어빌리티가 겹치면 마지막 종료까지 보류한다. 위치 충돌로 복귀가 실패하면 예약을 유지하고 재시도한다.
- 기존 일반 추종의 1초 텔레포트와 4타 미니언 소환 제한을 유지한다. WxAbility 코드는 수정하지 않는다.
- 종료 직후 복귀, 겹치는 활성 행동 중 보류, 일반 추종 1초 규칙을 회귀 테스트로 검증하고 UE 5.8 WxEditor Development를 빌드한다.

## 완료

- BT MirrorMovement가 Master와 분신 ASC의 `OnAbilityEnded`를 구독한다. 종료 시 복귀를 예약하며, 양쪽에 활성 어빌리티가 없는 첫 BT 틱에 현재 Master 우측 100cm로 텔레포트한다. 성공하면 예약/이동 타이머/잔여 이동 속도를 정리하고, 충돌로 실패하면 다음 틱에 재시도한다.
- 콤보 전환이나 다른 활성 어빌리티가 남아 있으면 예약을 보류한다. 서비스 해제 시 종료 구독과 예약을 제거한다. 일반 지상 추종의 1초 텔레포트 및 4타 미니언 소환 제한은 유지했다.
- WxAbility 계열 클래스에는 HEAD 대비 코드 변경이 없다.
- UE 5.8 WxEditor Win64 Development 빌드 성공: `Saved/Logs/BuildDoctor/build_2026-09-17_013132_844_19572.log`.
- `Wx.Combat.Doppelganger.MasterReplay` 성공. Master 가드 종료 뒤 분신만 가드가 남은 동안 복귀 보류, 마지막 종료 후 첫 BT 틱 복귀, 일반 장애물 추종의 1초 대기 및 텔레포트를 검증했다. 기존 행동 복제/소환 차단/수명 정리 검사도 함께 통과했다.
- 테스트 로그: `Saved/Logs/DoppelgangerAbilityEndTest.log`. 보고서: `Saved/Automation/DoppelgangerAbilityEnd/index.json`.
