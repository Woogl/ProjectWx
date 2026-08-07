# AssetDump

에셋을 JSON으로 덤프한 텍스트 미러다. 에셋 내용 검색은 여기서 grep으로 한다. 갱신은 `/dump-assets`.

본문 덤프는 `Blueprints/`·`DataAssets/`·`DataTables/`·`StateTrees/`·`Widgets/`에 에셋당 1파일이다. 몽타주·BehaviorTree·레벨·아트·마켓플레이스 에셋은 본문 덤프가 없다 — 에셋의 존재·경로는 `Content/`의 `.uasset`이 원본이므로 거기서 직접 찾는다.

*문서 기준 커밋 `63ea3f2f` · 생성일 2026-08-07 · 에셋 725개 — `/dump-assets`로 갱신*
