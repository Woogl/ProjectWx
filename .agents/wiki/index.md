# WX Wiki

프로젝트 지식의 시작점입니다. 모듈 설명의 본문은 이 Wiki에서 관리합니다. 원자료인 기획·회의록·코드와 시점별 리뷰는 기존 위치에 보존합니다.

브라우저에서 한눈에 보려면 [OpenWiki.bat](../../BatchFiles/OpenWiki.bat)을 실행하세요. 전체 문서 검색·분류·검증 상태 필터를 제공하며, 실행할 때마다 최신 문서로 화면을 생성합니다.

## 모듈 지도

아래 9개는 기존 README를 이관했습니다. 과거 기준 커밋을 유지하며 전체 재검증 전 상태입니다.
- [WxAI](modules/WxAI.md) — needs-review, 원문 기준 2872e9a
- [WxCombat](modules/WxCombat.md) — needs-review, 원문 기준 2872e9a
- [WxCore](modules/WxCore.md) — needs-review, 원문 기준 047197a
- [WxDialogue](modules/WxDialogue.md) — needs-review, 원문 기준 047197a
- [WxInventory](modules/WxInventory.md) — needs-review, 원문 기준 047197a
- [WxQuest](modules/WxQuest.md) — needs-review, 원문 기준 047197a
- [WxUI](modules/WxUI.md) — needs-review, 원문 기준 047197a
- [WxWorld](modules/WxWorld.md) — needs-review, 원문 기준 2872e9a
- [WxGame](modules/WxGame.md) — needs-review, 원문 기준 2872e9a

WxEditor, WxToolset, BoxComponentVisualizerEditor는 기존 README 이관 대상이 없었으며 아직 모듈 페이지가 없습니다. 프로젝트 전체 문서화가 완료되었다는 뜻은 아닙니다.

## 전투 파일럿

- [그로기](systems/groggy.md) — 기획과 정적 C++ 진입·종료 경로, 에셋·실행 미검증
- [그로기 피니시](systems/finisher.md) — 대상 선정·이동 주체 기획 충돌로 blocked
- [전투 자원](systems/combat-resources.md) — 공통 기획과 현광 구현 보고의 구분
- [미결정과 재검토](questions/open-questions.md) — Q-001~004

## 운영과 근거

- [문서 통합 결정](decisions/wiki-ownership.md)
- [운영 절차](maintenance.md) · [작성 규칙](AGENTS.md) · [변경 이력](log.md)
- [원자료 해시와 페이지 의존 목록](sources.json)
- [도입 검토서](../reports/LLM_Wiki_Feasibility.md) — 도입 전 검토 기록
- [AI 리뷰·검토 목록](../reports/index.md) — 시점별 검토 결과, 제안은 확정 결정이 아님

자료가 바뀌면 저장소 루트에서 `powershell -NoProfile -ExecutionPolicy Bypass -File BatchFiles/CheckWiki.ps1`을 실행하세요. 도구는 구조·링크·근거 변경을 검사하며 문장의 의미나 게임 실행을 검증하지 않습니다.
