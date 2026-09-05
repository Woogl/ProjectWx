# Controller 부활 코드 제거

## 계획

- 승인받은 즉시 부활 흐름으로 PlayerController의 부활 함수, 페이드, 일시정지, 입력 차단, Ticker 상태를 제거한다. 기존 CheatManager와 컴포넌트 등록·해제는 유지한다.
- RespawnLibrary에서 호출 위젯과 로컬 Controller를 검증하고 GameMode의 RespawnPlayer를 직접 호출한다. 성공한 경우에만 사망 화면을 닫는다.
- 체크포인트, 적 리스폰, Device 상태 유지 및 HUD 빙의 갱신 동작을 보존한다.
- UE 5.8 WxEditor Development 빌드와 PIE 연속 즉시 부활 검증을 수행한다.

## 완료

- PlayerController의 부활 기능을 전부 제거해 해당 파일은 Git 기준 기존 코드와 동일해졌다. CheatManager와 컴포넌트 등록·해제는 보존했다.
- RespawnLibrary가 유효한 활성 사망 화면과 로컬 Controller를 검증하고 GameMode에서 즉시 부활한 후 성공 시 화면을 닫는다. Blueprint 버튼 연결은 그대로 사용한다.
- 최초 빌드에서 EndPlay에 남은 RespawnTicker 참조의 C2065 오류를 확인해 제거했다. UE 5.8 WxEditor Development 재빌드 성공(종료 코드 0): Saved/Logs/BuildDoctor/build_2026-09-06_033244_055_35436.log.
- NullRHI PIE에서 실제 ST 체크포인트 상호작용 및 사망 화면 버튼으로 연속 2회 부활 통과. 버튼 호출 반환 전에 Pawn 교체와 화면 종료 완료, 중복 요청 거절, 체크포인트 위치, HP/MP 회복, 동일 월드/Controller, Device 상태 유지, 적 교체, HUD 단일 활성 및 입력/일시정지 복구를 검증했다.
- 일반 빙의 해제·재빙의의 HUD 정리/재생성도 통과했다. 결과: Saved/Logs/ImmediateRespawnPIEResult.json. NullRHI이므로 시각적 화면 검증은 포함하지 않는다.
- git diff --check 통과.
