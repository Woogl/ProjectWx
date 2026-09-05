# GameMode 기본 부활 API 사용

## 계획

- 승인받은 대로 WxGameMode의 RestartPlayer 오버라이드 및 CanRespawnPlayer/RespawnPlayer를 제거하고 기존 Experience 처리는 유지한다.
- RespawnLibrary에서 사망 여부를 검증하고 기존 Pawn 빙의를 해제한 후 AGameModeBase의 RestartPlayerAtTransform 또는 체크포인트가 없으면 RestartPlayer를 호출한다.
- 실패 시 기존 Pawn 복구, 성공 시 기존 Pawn 정리, HP/MP 회복, 스트리밍 준비, 적 리스폰 및 사망 화면 종료를 유지한다.
- UE 5.8 WxEditor Development 빌드 및 실제 ST/버튼 경로의 연속 PIE 부활로 검증한다.

## 완료

- WxGameMode의 부활 함수와 RestartPlayer 오버라이드를 제거했다. WxGameMode cpp/h는 Git 기준 기존 코드와 동일하다.
- RespawnLibrary에서 AGameModeBase 기본 재시작 API를 직접 사용하며 체크포인트 조회, 사망/싱글플레이 검증, 실패 복구 및 성공 후 처리를 유지했다. Controller/HUD/UIManager/Blueprint 버튼은 추가 변경하지 않았다.
- UE 5.8 WxEditor Development 빌드 성공(종료 코드 0): Saved/Logs/BuildDoctor/build_2026-09-06_033714_055_12036.log.
- NullRHI PIE에서 실제 ST 체크포인트 상호작용 후 사망 화면 버튼으로 연속 2회 부활을 검증했다. 동기 Pawn 교체/화면 종료, 중복 요청 거절, 동일 월드/Controller, 체크포인트 위치, HP/MP 회복, Device 상태 유지, 적 리스폰, HUD 교체 및 입력 복구가 통과했다.
- 일반 빙의 해제·재빙의 HUD 검증도 통과했다. 결과: Saved/Logs/BaseGameModeRespawnPIEResult.json. 화면 렌더링의 시각적 검증은 포함하지 않는다.
- git diff --check 통과.
