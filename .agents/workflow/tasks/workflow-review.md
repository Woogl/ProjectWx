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
