---
title: "Workflow 승인 버전·정리 완료·승인자 기록"
source: "MANUAL"
type: "notes"
ingested: "2026-09-22"
tags: [wx, workflow]
summary: "P1·P2 개선 요청에 따라 승인 버전 검증, 테스트 수용과 정리 완료 분리, 승인자 기록을 구현한 근거"
---

# 사용자 요청과 범위

AI 워크플로우 점검의 P2 두 항목 개선 요청 후 P1도 추가 요청했다. 코드 리뷰 승인 버전 보장, 테스트 수용과 최종 정리 완료 분리, 승인자 누락을 수정했다.

# 구현 관찰

Workflow-Execution.cjs는 검증 시작과 accept 시 현재 코드 버전을 마지막 approve의 codeVersion과 대조한다. 다르면 blocked/implement로 돌려 재검토·리뷰가 필요하다. 재시도 직전 코드가 바뀐 경우 검증을 시작하지 않는다.

accept는 cleanup을 저장한다. finish는 승인자 이름, wikiStatus(reflected/skipped), wikiEvidence, cleanupEvidence가 있어야 complete와 closure를 저장한다. closure는 확인 시각과 수용 코드 버전도 보존한다. 실제 Wiki 작업을 자동 실행하거나 기록 내용의 진위를 인증하지 않는다.

approve와 accept는 이름이 비어 있으면 거부하고 decisions.actor를 저장한다. 이름은 직접 입력한 표시명으로 계정 인증이 아니다. 과거 승인자의 신원은 추정하지 않는다.

readExecution은 closure 없는 과거 complete를 읽을 때 cleanup으로 해석하며 원본 파일을 자동 수정하지 않는다. 대시보드는 정리 대기와 완료를 별도로 표시한다. 정리 화면에서 Wiki 반영 근거 또는 지식이 없는 사유, 자료 정리 결과를 입력해 최종 완료한다. 자료 정리는 AI가 실제 수행하고 결과를 준비한다.

# 검증 범위

TestWorkflowExecution.cjs: 승인 버전과 다른 코드의 재시도·최종 수용 거부, 동일 버전 재시도 허용, 승인자 누락·불완전 정리 거부, 재시작·재전송, 과거 기록 보존 통과.
TestWikiViewer.cjs: 승인자·정리 입력, 재렌더 보존과 대시보드 분류의 모의 DOM 검증 통과. 실제 AI 호출·브라우저 육안·UE 실행은 미실시.

# 구현 입력 해시

- `.agents/scripts/Workflow-Execution.cjs`: SHA-256 `effab1af7c1526764eeb871c913104936585eb82befe94c564303b5343890379`
- `.agents/scripts/wiki-viewer/execution.js`: SHA-256 `9708ee004e75a33604c8360fa280a0917456359b1534431bcf9757beed2cfb5f`
- `.agents/scripts/wiki-viewer/workflow.js`: SHA-256 `b0489ca17fcea2b62834ca43ce9cd05eb3fb1634d67c1d5c762976b2c23c237a`
- `.agents/scripts/TestWorkflowExecution.cjs`: SHA-256 `831bf9ba276ccf997b71fda1f7e9402c6cce711812019ef47e8ea210e396e129`
- `.agents/scripts/TestWikiViewer.cjs`: SHA-256 `a13337b01b06c1d6d146460364101f0ee34b5043a204d9ff07467f2428262679`
