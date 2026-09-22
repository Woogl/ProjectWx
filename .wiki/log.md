# Wiki Activity Log

## [2026-09-22] init | 사용자 승인으로 WX Wiki를 팀 공유 프로젝트 로컬 .wiki 구조로 전환. 기존 문서 18개와 상태 메타데이터를 raw/notes에 보존하고 지식·운영 기사 16개로 연결. 코드·에셋 재검증 아님.

## [2026-09-22] lint | local command: 0 critical, 0 warnings, 0 suggestions, 21 auto-fixed

## [2026-09-22] lint | 순정 로컬 lint: critical/warning/suggestion/info 0. 저장소 링크 44개 문서 오류 0; 뷰어 검색·Workflow 모의 실행·Edge Mermaid 검증 통과. 게임 코드·에셋 재검증 아님.

## [2026-09-22] schema | 순정 기능과 중복되는 readme-writer 스킬을 제거하고 WX 모듈 근거 확인 지침만 schema.md에 통합.

## [2026-09-22] compile | Workflow의 최신 확정 기준과 Wiki 과거 사본의 경계, 읽기 전용 조사, 완료 후 지식 반영 절차 및 웹 완료 버튼의 실제 범위를 명확히 함.

## [2026-09-22] lint | 18 checks, 0 critical, 17 warnings, 3 suggestions, 0 candidates, 0 auto-fixed. 보고 전용: 관련 문서 역링크 누락 16건, inbox/_index.md가 순정 --inbox 수집 대상이 되는 문제 1건. 게임 코드·에셋 재검증 아님.

## [2026-09-22] lint | 정정: 18 checks, 0 critical, 16 warnings, 3 suggestions, 0 candidates, 0 auto-fixed. 직전 기록의 inbox/_index.md 경고는 순정 init이 만드는 색인이라 철회하며, 관련 문서 역링크 누락 16건만 유효.

## [2026-09-22] compile | 사용자 결정: 기획은 속도보다 충실하게 검토하되 기존 구조를 신규 기획의 허용 조건으로 삼지 않음. 기획 검토 프롬프트와 절차에 Wiki·원자료 대조 및 변경·확장·대체 방안 검토를 반영.

## [2026-09-22] compile | 기획 절차를 입력·검토·판단·확정과 핵심 검토 원칙으로 축약. 신중한 조사와 신규 기획 수용에 관한 기존 합의는 유지.

## [2026-09-22] schema | 사용자 승인으로 Wiki 링크만 담은 내부 모듈 README 9개를 제거하고 모듈 설명·탐색을 Wiki로 통합. 루트 README는 유지. 기존 raw 원문은 당시 기록으로 보존.

## [2026-09-22] compile | process 공통·설계·테스트·완료·기록 지침을 사람의 판단과 AI 수행 중심으로 축약. 기획 문서를 design.md로 변경하고 관련 링크·뷰어 경로를 갱신.

## [2026-09-22] compile | 단계 완료 시 Task 기록, 작업 완료 시 Wiki 반영으로 절차 통합. 별도 records.md를 제거하고 공통·완료 규칙과 관련 참조를 갱신.

## [2026-09-22] compile | 사용자 요청으로 테스트 결과 수용 버튼의 표시를 설계 확정으로 변경. 테스트 수용·완료 동작은 유지하며 안내를 갱신.

## [2026-09-22] compile | 사용자 정정에 따라 테스트 수용 버튼 표시를 기획 확정으로 수정. 기존 수용 동작 유지.

## [2026-09-22] compile | 사용자 요청으로 테스트 수용 버튼 표시를 테스트 완료로 변경. 기존 수용·완료 동작 유지.

## 2026-09-22 — Workflow 기획서 검토 전환

사용자 결정을 raw/notes/2026-09-22-workflow-review.md에 수집하고 wiki/references/wiki-workflow.md에 통합했다. 개발자의 기획서 검토 역할과 Workflow 검색 제거를 반영했다. 원자료 색인·원자료 수를 갱신했다. TestWikiViewer·TestWikiSpaces·TestWikiAI 통과; 실제 AI 응답 품질·브라우저 육안 검증 아님.

## [2026-09-22] ingest | 현재 작업 트리의 모듈·핵심 C++·설정·기획 조사 원자료 15개 수집. 파일별 SHA-256과 행 번호 발췌. 기존 Workflow 결정 원자료 1개 보존. 빌드·실행·바이너리 에셋 미검증.

## [2026-09-22] compile | 사용자 요청으로 기존 16개 기사 전체 재편찬 및 편집기 도구 기사 1개 추가. Q-001~003 미결정과 태그 단일 위치 규칙 유지. 두쫀쿠는 사용자 지시로 편찬 제외. schema의 인간 검증 근거 규칙에 따라 verified를 새로 부여하지 않음.

## [2026-09-22] cleanup | 출처 교체와 미결정 보존 후 대체된 legacy 원자료 19개 삭제. 기존 로그와 Workflow 판단 원본은 유지. 민감 문자열 삭제가 아닌 사용자가 요청한 자료 대체이므로 전체 Wiki 문자열 치환은 수행하지 않음.

## [2026-09-22] lint | 전체 재편찬 최종 검사: 원자료 16개·기사 17개, 순정 lint critical/warning/suggestion/info 0. 순정 색인 렌더러로 7개 색인 갱신. 저장소 링크 37개 문서 오류 0. TestWikiViewer·TestWikiSpaces·TestWikiDiagrams 통과. Edge에서 17개 기사·6개 다이어그램 실제 표시 확인. Wiki 화면의 Workflow 전용 함수 호출 오류를 화면 분기로 수정. 이미 삭제된 두쫀쿠 작업의 잔여 링크 정리. 게임 빌드·실행 검증은 수행하지 않음.

## [2026-09-22] lint | local command: 0 critical, 0 warnings, 0 suggestions, 2 auto-fixed

## 2026-09-22 · Workflow 대시보드 증분 반영

- 사용자 요청·코드 관찰·모의 DOM 검증을 raw/notes/2026-09-22-workflow-dashboard.md에 수집하고 기존 wiki/references/wiki-workflow.md에 통합했다. 인간 검증일은 부여하지 않았다.

## 2026-09-22 · Workflow P1·P2 증분 반영

- ingest: 사용자 개선 요청, 코드 해시·구현 관찰·회귀 검증 범위를 raw/notes/2026-09-22-workflow-closure.md에 수집했다.
- compile: 기존 wiki/references/wiki-workflow.md에 승인 버전 검사, 테스트 수용/정리 완료 분리, 승인자 기록과 과거 기록 해석을 통합했다. 인간 검증일은 추가하지 않았다.

## [2026-09-22] lint | local command: 0 critical, 0 warnings, 0 suggestions, 1 auto-fixed
- 검증·색인: 순정 Lint 오류 0, 저장소 링크 37개 문서 오류 0, 관련 회귀 6개 통과. 실제 원자료 18개로 상위 색인 통계를 정정하고 뷰어를 재생성했다.

## [2026-09-22] lint | 18 checks, 0 critical, 0 warnings, 3 suggestions, 0 candidates, 0 auto-fixed
- 보고 전용 에이전트 lint: 원자료 18개·기사 17개, 링크 157개(코드 38) 오류 0, 관련 문서 역링크 82쌍 양방향. 제안: inventory·datasets 계층 미사용(schema 방침상 의도), workflow-review 원자료 태그 tooling→workflow 통일 검토. 신선도 전 기사 75점(verified 없음 상한), 2026-10-06부터 70 미만. 게임 코드·에셋 재검증 아님.

## [2026-09-22] lint | 태그 정리: raw/notes/2026-09-22-workflow-review.md의 tooling 태그를 workflow로 통일하고 raw/notes/_index.md 행을 맞춤. 본문·출처 변경 없음, 순정 lint 0건.
