---
title: "Workflow 기획서 검토 전환과 검색 제거"
source: "MANUAL"
type: "notes"
ingested: "2026-09-22"
tags: ["wx", "workflow"]
summary: "사용자 요청: 개발자가 전달받은 기획서를 검토하고 Workflow 문서 검색을 제거한다."
---

# 사용자 결정

2026-09-22 대화에서 사용자는 기획자에게 제공하기에는 툴의 난도가 높으므로 기획 단계를 기획서 검토 단계로 바꾸고, Workflow의 문서 검색을 제거하도록 요청했다.

# 반영 범위

개발자가 외부에서 작성된 기획서를 입력하고 AI가 누락·충돌·구현 영향을 조사한다. 기획 판단은 담당자와 확인한 답변으로 기록하고 검토본을 확정해 설계로 넘긴다. AI가 없는 기획이나 수치를 임의로 작성하지 않는다.
Workflow 검색 링크·단축키·검색 주소 진입을 제거한다. Wiki 검색은 유지한다. 기존 planning 저장 키와 확정 인계 구조는 유지한다.

# 확인

TestWikiViewer.cjs, TestWikiSpaces.cjs, TestWikiAI.cjs 통과. 모의 UI·저장·인계 검증이며 실제 AI 응답 품질과 브라우저 육안 검증은 포함하지 않는다.
