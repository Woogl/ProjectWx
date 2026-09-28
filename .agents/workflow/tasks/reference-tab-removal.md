# 대시보드 리뷰·참고 탭 제거

상태: 완료 · 체크리스트 4/4 통과
다음 행동: 변경 시 기록된 테스트 범위와 제약을 참고한다.

- 결정(2026-09-27): 상태 줄이 없는 기록(모듈 리뷰 등)은 할 일이 아니므로 대시보드에 보이지 않게 한다. 파일은 `tasks/`에 그대로 두고 `--tasks` 출력에만 나온다. 모듈 리뷰에 상태 줄을 넣는 안은 기각했다(상시 확인 대기로 남고, 작업 탭 추가 요청이 리뷰 문서에 승인·체크리스트를 쓰며, 재리뷰가 문서를 통째로 다시 써 그 기록을 지운다).

## 요청

- 사용자: 워크플로우 페이지의 리뷰·참고에 어떤 문서가 등록되는지, 웹에서 보여줄 필요가 있는지 물었다. AI가 탭 제거(workflow.js 네 번째 그룹·안내문, 테스트, process/index.md 상태 절 한 줄)를 제안했고, 사용자가 "네, 진행해주세요"로 승인했다.

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 워크플로우 노드 테스트 | TestWorkflowPage·TestWorkflowTestFeedback·TestWorkflowFeedbackUI·TestWorkflowProviders 실행 | AI | 통과 | 4개 모두 exit 0. TestWorkflowPage는 필터 3개, 상태 줄 있는 기록만 한 번씩 목록에 오름, 모듈 리뷰·자유 형식 상태 기록 제외를 확인한다 · 2026-09-27 |
| 대시보드 표시 | Export-WorkflowPage.ps1로 재생성한 뒤 헤드리스 Edge 1440px 캡처. 필터가 확인 대기·진행 중·완료 셋이고 모듈 리뷰가 보이지 않는다. | AI | 통과 | 필터 확인 대기 7·진행 중 0·완료 26, 안내문 33개 기록(전체 39 중 모듈 리뷰 6 제외) · 2026-09-27 |
| --tasks 출력 | node .agents/scripts/Workflow-Server.cjs --tasks | AI | 통과 | 모듈 리뷰 6건이 「상태 없음(대시보드에 안 보임)」 아래에 나온다 · 2026-09-27 |
| 코드 리뷰 | 변경 파일: .agents/scripts/workflow-page/workflow.js(그룹 셋, 상태 없는 기록 제외, 안내문 제거), Workflow-Server.cjs(--tasks 제목), TestWorkflowPage.cjs·TestWorkflowTestFeedback.cjs(단언), process/index.md 상태 절 한 줄. 볼 점: 상태 줄에 오타가 난 기록도 대시보드에서 빠지고 --tasks에만 남는다. | 사람 | 통과 | woogle 2026-09-27 |


## 사용자 테스트 결과 · 2026-09-27T12:30:06.472Z

<!-- test-feedback:request-a1a1e567-c567-4b65-a2d7-3772d07f781b:submitted -->
- 전달한 사람: woogle

> 통과 · 코드 리뷰
