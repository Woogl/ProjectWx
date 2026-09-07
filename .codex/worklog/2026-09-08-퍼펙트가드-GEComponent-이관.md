# 퍼펙트 가드 후처리 GEComponent 이관

## 계획

- 승인 범위: UWxAbility_Guard::HandlePerfectGuard의 GP 반사·Event.Hit.Parry·퍼펙트 가드 Cue를 UWxEffectComponent_DamageResponse::ProcessPerfectGuard로 이동한다.
- GuardReact가 사용하는 Event.PerfectGuard를 유지하고 Guard의 ListenForPerfectGuard, 이벤트 대기 태스크, 핸들러와 관련 include·주석을 정리한다.
- 가드 어빌리티의 이벤트 구독 여부 대신 GE의 퍼펙트 가드 판정에 따라 후처리를 실행한다. 공격자 그로기 시 GP 추가 금지, 패리 불가 시 반사·패리 금지, 방어자 원인 정보와 Cue 컨텍스트를 보존한다.
- 관련 자동화 테스트 확장 및 UE 5.8 WxEditor Development 빌드와 테스트로 검증한다. 기존 변경은 보존한다.

## 완료

- ProcessPerfectGuard에서 GuardReact 이벤트를 보낸 뒤 GP 반사·패리 이벤트·Cue를 직접 실행하도록 이관했다. 방어자 Avatar를 패리 Instigator로 사용하고 반사 GP 컨텍스트는 방어자 ASC에서 생성한다.
- Guard의 HandlePerfectGuard / ListenForPerfectGuard / WaitGameplayEvent 의존성을 제거하고 책임 설명을 정정했다.
- UE 5.8 WxEditor Win64 Development 빌드 성공(종료 코드 0).
- 빌드 로그: C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-08_003745_714_7964.log
- Wx.Combat.DamageResponse.Execution 성공: 기존 검증 외에 반사로 그로기 진입 후 패리, 기존 그로기 GP 보존, 패리 불가 제외, 반사량 0 패리, 방어자 원인 정보, Guard 비활성 상태에서도 GuardReact 통지 1회 및 반사 처리 검증.
- 테스트 로그: C:\Wx\Saved\Logs\PerfectGuardResponseAutomation.log (종료 코드 0). 테스트 외 초기화에서 HTTP 8000 포트 바인드 오류가 있었으나 해당 테스트는 Success로 완료했다.
- 수정한 Guard 파일의 diff 공백 검사 통과. 기존 변경 보존. 화면 없는 테스트이므로 Cue의 시청각 결과와 멀티플레이 몽타주는 별도 플레이 검증 대상이다.
