# SpawnerLibrary 제거

상태: 확인 대기 · SpawnerLibrary 제거·빌드 확인
다음 행동: 에셋 로드와 부활 시 적 재생성, 영구 처치 제한을 확인한다.

- 요청: UWxSpawnerLibrary 제거, BP 작업 최소화.
- 구현 완료: AWxSpawner::RespawnAll C++ 전용 함수로 일괄 재생성을 이동. 플레이어 부활·StateTree의 C++ 호출부 전환. 라이브러리 헤더/cpp 삭제.
- 기존 동작: 로드된 스포너만 재생성, Manual 제외, 서버 권한과 영구 처치 제한 유지. 재생성 중 파괴된 스포너를 건너뛰도록 약참조 목록 사용.
- 정적 검증: Source/Plugins/Config의 기존 라이브러리 참조 없음. 에셋 바이너리 문자열 검색에서 직접 호출 참조 없음. git diff --check 통과.
- 검증: WxEditor Win64 Development 빌드 성공(종료 코드 0, 159.49초). 로그: Saved/Logs/BuildDoctor/build_2026-09-25_182323_851_33356.log. 인게임·에셋 로드 확인과 사람 리뷰는 미수행.
- Wiki: raw/notes/2026-09-25-spawner-library-removal.md와 world 기사 반영.


## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 에셋 로드 | 스포너·부활 StateTree 에셋을 새 프로세스로 로드해 경고·오류가 없는지 확인 | AI | 대기 |  |
| 부활 시 적 재생성 | 게임: 적을 처치하고 사망·부활하면 처치한 일반 적이 다시 나타난다 | 사람 | 대기 |  |
| 영구 처치 제한 | 게임: 영구 처치 대상은 부활 뒤에도 다시 나타나지 않는다 | 사람 | 대기 |  |
| 코드 리뷰 | AWxSpawner::RespawnAll과 부활·StateTree 호출부 | 사람 | 대기 |  |