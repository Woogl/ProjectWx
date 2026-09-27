# SpawnerLibrary 제거

상태: 완료 · 체크리스트 4/4 통과
다음 행동: 변경 시 기록된 테스트 범위와 제약을 참고한다.

- 요청: UWxSpawnerLibrary 제거, BP 작업 최소화.
- 구현 완료: AWxSpawner::RespawnAll C++ 전용 함수로 일괄 재생성을 이동. 플레이어 부활·StateTree의 C++ 호출부 전환. 라이브러리 헤더/cpp 삭제.
- 기존 동작: 로드된 스포너만 재생성, Manual 제외, 서버 권한과 영구 처치 제한 유지. 재생성 중 파괴된 스포너를 건너뛰도록 약참조 목록 사용.
- 정적 검증: Source/Plugins/Config의 기존 라이브러리 참조 없음. 에셋 바이너리 문자열 검색에서 직접 호출 참조 없음. git diff --check 통과.
- 검증: WxEditor Win64 Development 빌드 성공(종료 코드 0, 159.49초). 로그: Saved/Logs/BuildDoctor/build_2026-09-25_182323_851_33356.log. 인게임·에셋 로드 확인과 사람 리뷰는 미수행.
- Wiki: raw/notes/2026-09-25-spawner-library-removal.md와 world 기사 반영.


## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 에셋 로드 | 스포너·부활 StateTree 에셋을 새 프로세스로 로드해 경고·오류가 없는지 확인 | AI | 통과 | 2026-09-27 임시 자동화 테스트(헤드리스 Editor Development, 확인 뒤 삭제): ST_CheckPoint·BP_CheckPoint·ST_QuestStep_KillEnemies·ST_Quest_Main1·ST_Quest_Main2와 스포너가 배치된 외부 액터 패키지 23개(LV_DevCombat·LV_OpenWorld·SiegeCannonEmplacement01) 로드 성공, 오류 0. 경고 8건은 스포너와 무관한 없어진 GameplayTag(`Quest.Fail`·`Event.Device.Triggered`) 참조. 같은 날 실제 BP_CheckPoint 상호작용에서 「스포너 리스폰」 태스크도 오류 없이 실행 |
| 부활 시 적 재생성 | 헤드리스 게임: 적을 처치하고 사망·부활하면 처치한 일반 적이 다시 나타난다 | AI | 통과 | 2026-09-27 임시 자동화 테스트(확인 뒤 삭제): 헤드리스 게임(LV_DevCombat -game -nullrhi)에서 레벨이 쓰는 적 BP_Template(Enemy)로 일반 스포너를 세우고 치트로 처치 → 플레이어 사망 → 사망 화면에서 부활(RequestRespawn의 RespawnAll)하자 새 적이 스폰되고 시체는 정리됨. 로직 단위 테스트에서도 처치 통지·재생성·Manual 제외 확인 |
| 영구 처치 제한 | 헤드리스 게임: 영구 처치 대상은 부활 뒤에도 다시 나타나지 않는다 | AI | 통과 | 같은 헤드리스 게임에서 bNeverRevive 스포너의 적은 부활 뒤에도 나타나지 않고 처치 상태 유지·시체 정리. 로직 단위 테스트에서 살아 있는 영구 대상은 RespawnAll에 새 인스턴스로 되돌려짐도 확인 |
| 코드 리뷰 | AWxSpawner::RespawnAll과 부활·StateTree 호출부 | 사람 | 통과 | woogle 2026-09-27 |

## 사용자 테스트 결과 · 2026-09-27T12:18:56.518Z

<!-- test-feedback:request-a2682866-368a-4abc-9360-a8d3d826b4c7:submitted -->
- 전달한 사람: woogle

> 통과 · 코드 리뷰
