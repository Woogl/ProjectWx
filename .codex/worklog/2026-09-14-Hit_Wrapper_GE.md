# Hit Wrapper GE 도입

## 계획

- 승인된 범위: Instant WxEffect_Hit과 GE Component로 무적·회피·가드 선판정을 GAS 적용 경로에 통합한다.
- ApplyDamage는 Context·Spec 생성과 Wrapper 적용을 담당한다. 기존 호출부의 회피/실패 false, 피해 GE 적용 성공 true 의미와 히트스톱을 보존한다.
- WxEffect_Damage는 피해·반사량 계산, SP·HP·GP 반영과 DamageFloater를 담당한다. HitReact·GuardReact·퍼펙트 가드·반사·DamageDealt·추가 GE는 Wrapper에서 처리한다.
- 반응은 실제 Damage 실행 결과와 사망·그로기 상태가 확정된 직후 실행한다. 수식, 스탯 반영 순서, 0 피해·퍼펙트 가드의 추가 효과 정책을 보존한다.
- 실행별 상태는 Spec/Context와 호출 지역 데이터로 전달하며 GE Component 멤버에 저장하지 않는다. 기존 네트워크 실행 주체와 반응 GA 정책을 유지한다.
- 후속 요청 반영: 더 이상 타격 적용에 쓰이지 않는 EWxDamageCheck와 CheckDamage를 제거한다. 투사체의 연출·반사 분기에서 필요한 무적 상태 조회는 기존 조건을 보존한다.
- 검증: 변경 전후 호출부·수식·이벤트 순서를 대조하고 일반/회피/가드/퍼펙트 가드/0 피해/자식 거부 경로를 검증한다. 런처에서 UE 5.8 설치를 조회해 WxEditor Development를 빌드한다. 가능한 자동 검증과 실행하지 못한 PIE 검증을 구분해 기록한다.

## 완료

- `UWxEffect_Hit`과 `UWxEffectComponent_Hit`을 도입했다. 팀·사망·입력 행 조건은 GE 적용 요구사항으로 검사하고, 무적은 적용 후 회피 성공 이벤트만 보내는 분기로 처리한다.
- `FWxHitEffectContext`로 입력 행과 가드·퍼펙트 가드 스냅샷을 전달한다. Context 복사는 HitResult를 깊은 복사하며 로컬 적용 성공 상태를 초기화한다. 커스텀 직렬화를 구현했고 전역 ASC/Globals 할당 정책은 변경하지 않았다.
- 자식 Damage의 실제 실행 Spec을 ASC의 동기 적용 델리게이트에서 수집한다. Context별로 필터링하고 자식 적용 직후 구독을 해제한다. Component에는 타격별 가변 멤버를 두지 않았다.
- 피해 수식과 SP→IncomingDamage→GP 순서, 크리티컬·0 피해 처리, 퍼펙트 가드 반사량 계산은 유지했다. DamageResponse는 플로터만 담당하고, Hit·DamageDealt·퍼펙트 가드·반사·패리·추가 GE·Hit Cue는 Wrapper로 이동했다.
- 반응은 자식 적용 완료 후 실행한다. DamageFloater는 자식 실행 훅에서 발생하므로 이제 반응보다 앞서 출력된다. 퍼펙트 가드에서도 자식은 반사량 메타 속성만 계산하고 HP·SP·대상 GP를 변경하지 않는다.
- `ApplyDamage()`의 기존 bool 의미를 보존했다. 회피·자식 거부는 false, 0 피해와 퍼펙트 가드를 포함한 자식 GE 적용 성공은 true다.
- 후속 요청대로 `EWxDamageCheck`와 `CheckDamage()`를 제거했다. 투사체는 기존 적대·사망·무적 조건을 연출·반사·수명 분기에서만 조회한다. 소스 내 남은 참조가 없다.
- `Wx.Combat.HitWrapper.Application` 자동 테스트를 추가했다. 정상 피해·추가 GE, 회피, 퍼펙트 가드/반사, 가드 브레이크, 가드 불가 공격, 사망/그로기 이벤트 순서, 자식 거부, 0 피해, 0 반사, 아군/사망 거부, Damage 단독 적용의 반응 부재, Context 복사를 실제 ASC로 검증했다.
- 최종 UE 5.8.2 `WxEditor Win64 Development` 빌드 성공(종료 코드 0). 로그: `C:/Wx/Saved/Logs/BuildDoctor/build_2026-09-14_215002_270_32708.log`.
- 최종 NullRHI 자동 테스트 성공: 1개 테스트, 실패 0, 테스트 경고 0. 로그: `C:/Wx/Saved/Logs/HitWrapperAutomation.log`, 보고서: `C:/Wx/Saved/Automation/HitWrapper/index.json`.
- 빌드 재현: `& 'C:/Wx/.agents/skills/build-doctor/scripts/Invoke-WxEditorBuild.ps1' -ProjectRoot 'C:/Wx'`. 실제 명령: `& 'C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat' WxEditor Win64 Development '-Project=C:/Wx/Wx.uproject' -WaitMutex -NoHotReloadFromIDE`.
- `git diff --check` 통과. 모듈 의존성·반응 GA 네트워크 정책·기존 활성화 예측 키 전달 방식은 변경하지 않았다.
- 검증 한계: 자동 테스트는 BeginPlay·반응 몽타주·Cue 렌더링을 실행하지 않는 권위 ASC 검증이다. 네트워크 PIE, 실제 회피/반응 몽타주, 플로터 시각 출력, Context의 네트워크 직렬화 왕복은 실측하지 않았다. 클라이언트 독립 HitReact 예측은 이번 범위에 포함하지 않았다.
