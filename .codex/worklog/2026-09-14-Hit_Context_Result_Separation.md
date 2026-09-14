# Hit Context 적용 결과 분리

## 계획

- 승인 범위는 FWxHitEffectContext의 bDamageApplied 반환 경로 분리다. 공격 행·방어 판정의 기존 복제 데이터와 직렬화 형식은 유지한다.
- ApplyDamage의 지역 관찰자가 ASC의 실제 Damage 적용 콜백을 수집하여 bool을 반환한다. 타격마다 생성한 Context의 동일성으로 다른 타격·Wrapper·추가 효과와 구분한다.
- Wrapper와 자식 Damage는 같은 타격 Context를 사용한다. 방어 판정은 자식 적용 전에 확정하고, 결과는 Context에 쓰지 않는다. Context의 명시적 Duplicate는 기존처럼 HitResult를 깊은 복사한다.
- 정상/0 피해/퍼펙트 가드 성공 true, 회피/Wrapper 거부/자식 거부 false 계약과 기존 반응·Cue 순서를 보존한다.
- 기존 자동 테스트를 유지하고 재진입 타격의 반환값 격리를 검증한다. UE 5.8 WxEditor Development 빌드, 관련 자동 테스트, diff 검사를 실행한다.

## 완료

- Context의 bDamageApplied 멤버와 Duplicate/NetSerialize의 해당 초기화를 제거했다. DataTable·RowName·방어 플래그의 복제 필드 및 직렬화 순서·비트 수는 그대로 유지했다.
- ApplyDamage의 지역 FWxDamageApplicationObserver가 대상 ASC의 Damage 적용 콜백을 수집한다. 동일 타격 Context와 Damage GE 타입을 확인하고 호출 종료 전 구독을 해제한다. 성공 여부를 Context나 공유 Component에 기록하지 않는다.
- 자식 Damage를 같은 타격 Context에 연결하되 LinkedSpec의 Source 태그 스냅샷을 재수집하지 않도록 설정했다. Context의 명시적 Duplicate는 여전히 독립적인 방어 스냅샷과 깊은 복사 HitResult를 제공한다.
- 회피 성공 이벤트 안에서 무적을 해제하고 별도의 타격을 재진입시키는 검증을 추가했다. 내부 타격은 true·100 피해, 바깥 회피는 false로 반환되어 결과가 섞이지 않음을 확인했다.
- UE 5.8.2 WxEditor Win64 Development 최종 빌드 성공(종료 코드 0). 로그: `C:/Wx/Saved/Logs/BuildDoctor/build_2026-09-14_221107_118_32160.log`.
- 최종 `Wx.Combat.HitWrapper.Application` 성공: 테스트 1개, 실패 0, 테스트 경고 0. 기존 정상/0 피해/퍼펙트 가드/회피/거부/이벤트 순서/Cue/Context 복사 검증과 재진입 검증을 모두 포함한다.
- 자동 테스트 로그: `C:/Wx/Saved/Logs/HitContextResult.log`, 보고서: `C:/Wx/Saved/Automation/HitContextResult/index.json`.
- `git diff --check` 통과. 네트워크 PIE 및 직렬화 왕복은 이번 작업에서 별도 실행하지 않았다.
- 빌드 재현: `& 'C:/Wx/.agents/skills/build-doctor/scripts/Invoke-WxEditorBuild.ps1' -ProjectRoot 'C:/Wx'`. 실제 명령: `& 'C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat' WxEditor Win64 Development '-Project=C:/Wx/Wx.uproject' -WaitMutex -NoHotReloadFromIDE`.
