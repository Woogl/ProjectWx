# HitStop CustomTimeDilation 단순화

## 계획

- 기존 HitStop GE, 태그, 무기·투사체 지속시간, 서버 적용·태그 복제 및 중첩 수명을 유지한다.
- WxHitStopComponent가 최초 태그 추가 시 기존 CustomTimeDilation을 저장하고 작은 배율을 적용하며, 마지막 태그 제거 및 EndPlay에서 복원한다. 별도 타이머를 추가하지 않는다.
- GlobalAnimRateScale 제어와 이동 컴포넌트의 HitStop 전용 분기를 제거한다. 점프·앉기 입력 제한은 유지하고 관련 주석을 갱신한다.
- 엔진의 컴포넌트 Tick, 네트워크 이동·루트모션 시간 경로와 GE 수명을 확인하고 UE 5.8 WxEditor Win64 Development를 빌드한다. 실제 멀티플레이 체감·위치 보정은 PIE 확인 항목으로 보고한다.

## 완료

- WxHitStopComponent가 소유자의 기존 CustomTimeDilation을 한 번 저장하고 0.001배를 적용한다. 마지막 태그 제거 및 EndPlay에서 저장한 값을 복원한다. 이미 적용된 상태의 중복 호출은 무시한다.
- GlobalAnimRateScale 제어, ControlledCharacterMove·SimulateMovement의 HitStop 전용 오버라이드와 IsHitStopped를 제거했다. 점프·앉기 제한과 GE 적용 호출부는 유지했다.
- 컴포넌트 Tick은 Actor.h의 ExecuteTickHelper에서 소유자의 CustomTimeDilation을 곱한다. CMC의 클라이언트·서버 이동 델타 한도도 액터 배율을 사용하며, 배율이 다른 SavedMove는 결합하지 않는 것을 확인했다.
- GameplayEffect.cpp의 DurationHandle은 월드 TimerManager에 등록되고 GetWorldTime은 World->GetTimeSeconds()를 반환한다. HitStop GE와 표준 GAS 쿨다운 수명에는 액터 배율이 적용되지 않는다.
- git diff --check 통과. 기존 사용자 변경 Content/Item/BP_Katana.uasset은 수정하지 않았다.
- 실행 검증 범위: 컴파일·링크 및 소스 경로 확인. PIE는 실행하지 않았다. 연타 중 마지막 GE 만료·기존 배율 복원, HitStop 중 쿨다운 만료, 공중·루트모션 공격, 이동 발판, 2인 PIE 지연 환경의 위치 보정·시뮬 프록시 움직임은 플레이 확인 항목이다. 기존 완전 정지에서 극저속으로 바뀌며, 액터의 다른 Tick 컴포넌트도 함께 느려진다.

### 빌드 결과

- 상태: 성공. UE 5.8.2, WxEditor Win64 Development, 종료 코드 0.
- 로그: C:\Wx\Saved\Logs\BuildDoctor\build_2026-09-16_011146_930_31464.log

### 원인 요약

- 최초 샌드박스 실행은 UBT 시작 메시지 뒤 종료됐고 사용자 폴더의 UBT 로그 접근도 거부됐다. 권한 문제로 추정하고 build-doctor 스크립트를 승인된 샌드박스 외 실행으로 재시도해 성공했다.

### 근거 로그

> Result: Succeeded
> BUILD_DOCTOR_RESULT=success
> BUILD_DOCTOR_EXIT_CODE=0

### 수정 방법

- 빌드 관련 추가 수정 불필요.

### 재실행 명령

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" WxEditor Win64 Development "-Project=C:\Wx\Wx.uproject" -WaitMutex -NoHotReloadFromIDE
```
