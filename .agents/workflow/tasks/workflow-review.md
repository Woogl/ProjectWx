# Workflow 기획서 검토 전환

상태: 완료 · 2026-09-22

- 판단: 기획자 직접 사용 대신 개발자가 전달받은 기획서를 검토한다. Workflow 문서 검색을 제거한다.
- 구현: 단계 안내·AI 지침·검토 버튼 수정, 검색 링크·단축키·검색 주소 진입 제거. 기존 저장 키와 확정 인계 보존.
- 검증: TestWikiViewer.cjs, TestWikiSpaces.cjs, TestWikiAI.cjs 통과. 실제 AI 응답 품질·브라우저 육안 검증은 미실시.
- 지식 반영: [도구 구조](../../../.wiki/wiki/references/wiki-workflow.md).
- 별도 기존 오류: CheckWikiLinks.ps1에서 tasks/index.md의 두쫀쿠 회복 아이템 만들기 기획 인계 링크 1건 실패. 이번 변경 대상 밖이며 수정하지 않았다.

## 실행과 저장 안내 이미지 전환 · 2026-09-22

- 완료: 사용자의 요청에 따라 usage.md의 안내 본문을 assets/ai-workflow.png 이미지로 교체했다.
- 검증: Workflow 뷰어 재생성 및 TestWikiViewer.cjs 통과. 이미지 포함·상대 경로 해석·반응형 표시 규칙 확인. 브라우저 육안 확인은 미실시.
- 문서 표시만 변경하여 추가 Wiki 지식 반영은 생략했다.
- 후속 문구 변경: 안내 문서 제목과 Workflow 탐색 링크를 사용 방법 안내로 통일했다.
- 후속 메뉴 정리: 사용자 요청으로 왼쪽 탐색의 공통 절차 제목과 하위 링크 네 개를 제거했다. 절차 원본 문서는 유지한다.
- 후속 메뉴 정리: 개별 작업 설명 구역과 관련 지식 링크 모음도 탐색에서 제거했다. 기존 작업 선택 및 상단 Wiki 링크로 접근한다.

## Workflow 메뉴 5개 구성 · 2026-09-22

- 완료: 작업 현황 대시보드, 새 작업 만들기, 기존 작업 이어하기, 사용 방법 안내, LLM 위키 검색 순서로 메뉴를 고정했다. 중복 상단 Wiki 링크는 숨겼다.
- 동작: 새 작업과 기존 작업 메뉴는 각각 생성 입력과 저장 작업 선택을 표시한다. 안내는 기존 이미지 문서, 위키 검색은 knowledge.html에 연결한다.
- 검증: Export-Wiki.ps1 재생성, TestWikiViewer.cjs 및 TestWikiSpaces.cjs 통과. 메뉴 순서·링크·생성 및 이어하기 경로 검증 추가. 브라우저 육안 확인은 미실시.

## 단계별 일감 대시보드 · 2026-09-22

- 완료: 전체 공용 작업을 기획 검토·설계·구현·코드 리뷰·테스트·완료별 카드로 표시한다. 건수·상태·중단 표시와 카드 클릭 시 저장·재조회·작업 열기를 구현했다. 후속 변경 인계는 별도 표시한다.
- 검증: TestWikiViewer.cjs, TestWikiSpaces.cjs 통과. 분류·확정 무효·중단·인계·빈 상태 테스트 추가. 실제 브라우저 육안 확인은 미실시.
- 지식: .wiki/wiki/references/wiki-workflow.md에 동작과 검증 범위를 반영했다.

## 기획 검토 문서 파일명 변경 · 2026-09-22

- 사용자 지정 이름 design_review.md로 변경하고 문서 링크·화면 라우팅·관련 테스트 참조를 갱신했다. planning 저장 키와 설계 단계 design 식별자는 유지한다.
- 검증: Workflow 뷰어 재생성, TestWikiViewer.cjs·TestWikiSpaces.cjs 통과, CheckWikiLinks.ps1 링크 오류 0건.

## AI 워크플로우 점검 · 2026-09-22

상태: 점검 완료 · 아래 개선 사항 미해결. 코드 수정 없음.

- 범위: AGENTS.md, 공통 절차, Wiki 운영 문서, 실행·저장·화면 코드와 관련 회귀 테스트. 실제 AI 제공자 호출·브라우저 육안·UE 실행은 제외.
- P1 — 검증 재시도 시 코드 리뷰 버전 보장 누락: Workflow-Execution.cjs의 retry는 verify 단계를 유지하고 startedVersion을 현재 값으로 다시 설정한다. v1 리뷰 승인 → v2 외부 변경 → retry → accept 순서의 임시 모의 실행에서 approve=v1, accept=v2인데 complete가 저장됨을 확인했다. 검증 시작과 최종 수용 시 최신 코드 리뷰 승인 버전과 현재 버전을 비교하고 다르면 리뷰로 되돌려야 한다.
- P2 — 테스트 수용과 최종 정리 완료가 같은 상태: accept가 즉시 complete를 저장하고 대시보드는 완료로 분류한다. completion.md의 지식 반영·자료 정리 완료 여부는 상태에 없다. 테스트 수용 후 정리 대기와 최종 완료를 구분하고 Wiki 반영 여부 또는 생략 사유를 기록할 필요가 있다.
- P2 — 승인자 누락: 실행 decisions에는 action/codeVersion/at/feedback/confirmedChecks만 저장되어 코드 리뷰·테스트 수용의 승인자를 식별할 수 없다. 공통 절차가 요구하는 승인자 기록과 불일치한다. 승인자와 항목별 확인 결과를 함께 남길 필요가 있다.
- 검증: TestWorkflowExecution.cjs, TestWikiAI.cjs, TestWikiTasks.cjs, TestWikiRecovery.cjs, TestWikiSpaces.cjs, TestWikiViewer.cjs 모두 통과. CheckWikiLinks.ps1은 37개 문서에서 오류 0건. 기존 테스트는 위 재시도 버전 변경 사례를 포함하지 않는다.
- Wiki 반영: 미확정 개선안과 결함 추적은 이 Task에 유지하며, 구현·운영 결정이 확정되기 전 중복 Wiki 기사는 만들지 않는다.


## P2 개선 · 2026-09-22

상태: 구현·회귀 검증 완료. 사용자 요청으로 위 점검의 P2 두 항목을 수정했다. P1은 미해결이다.

- 테스트 수용은 cleanup(정리 대기), 지식·자료 정리 기록 후 finish는 complete(최종 완료)로 분리했다. Wiki 반영 근거 또는 지식 없음 사유, 자료 정리 결과와 확인자·시각·수용 버전을 보존한다.
- 코드 리뷰·테스트 수용에 직접 입력한 승인자 이름을 필수로 저장하고 화면에 표시한다. 계정 인증을 뜻하지 않는다.
- 과거 complete에 closure가 없으면 읽을 때만 정리 대기로 해석한다. 과거 승인자와 원본 파일은 추정·자동 덮어쓰기하지 않는다.
- 검증: TestWorkflowExecution, TestWikiViewer, TestWikiAI, TestWikiTasks, TestWikiRecovery, TestWikiSpaces 통과. 승인자 누락·미완료 정리 거부, 재시작·재전송, 과거 기록 보존, 화면 입력·대시보드 분류를 검증했다. 실제 AI 호출·브라우저 육안·UE 실행은 미실시.
- 운영 지식은 기존 Wiki 도구 구조 문서에 새 원자료를 근거로 통합한다.


## P1 추가 개선 · 2026-09-22

- 사용자 추가 요청으로 P1도 해결했다. 검증 재시도 시작과 테스트 수용 시 최신 코드 리뷰 승인 버전을 대조한다. 불일치하면 blocked/implement로 전환하고 AI 재검토 후 코드 리뷰를 다시 받는다.
- TestWorkflowExecution에서 v1 승인 → v2 변경 → retry가 검증을 시작하지 않고 차단됨을 확인했다. 재검토·재승인 후 수용, 동일 승인 버전의 재시도 허용, 결과 버전만 현재인 경우의 최종 수용 거부도 검증했다.
- P2 구현과 함께 기존 Wiki 도구 구조에 근거를 수집·통합했다.
