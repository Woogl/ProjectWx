---
type: source
title: "작업 - workflow-codex-mcp-connection"
created: 2026-09-29
updated: 2026-09-29
status: developing
tags:
  - "source"
  - "작업-기록"
  - "워크플로우"
  - "Codex"
  - "MCP"
summary: "Workflow Codex 구현 실행이 꺼진 로컬 unreal-mcp 서버에 연결하다 실패하던 문제를, 연결 거부가 확인된 서버만 그 실행에서 빼도록 고친 2026-09-30 완료 작업 기록"
source_type: task-record
source_id: src-8cf2df7806143046de76
sha256: cb5c1ef3d9ba32c338e232757fb8092e8f8015b3214346706c56a879c023f2eb
authority: primary
independence_key: ".agents/workflow/tasks/workflow-codex-mcp-connection.md"
review_state: active
refresh_due: 2027-03-28
original_paths:
  - ".agents/workflow/tasks/workflow-codex-mcp-connection.md"
raw_copy: ".raw/captured/cb5c1ef3d9ba32c338e232757fb8092e8f8015b3214346706c56a879c023f2eb.md"
claim_ids:
  - clm-cb5c1ef3d9-c1
  - clm-cb5c1ef3d9-c2
  - clm-cb5c1ef3d9-c3
key_claims:
  - "2026-09-30 Workflow Codex 구현 실행은 꺼진 로컬 unreal-mcp(127.0.0.1:8000)에 연결하다 실패했고, 원인은 에디터 MCP 리스너가 멈춘 뒤 work 실행이 꺼진 서버에도 연결한 것이었다."
  - "수정 뒤 Workflow-Providers.cjs는 Codex 구현 실행 전에 unreal-mcp 로컬 HTTP 서버를 확인해 TCP 연결 거부일 때만 그 실행에서 빼고, 정상·원격·다른 MCP와 설정 파일은 유지하며 제외 사실을 콘솔·AI 입력·결과 근거에 남긴다."
  - "MCP 연결 수정은 실제 Codex 모델 응답까지 서버 꺼짐·켜짐 두 경우로 확인했고, 코드 리뷰는 사용자 지시로 AI 검토로 대신했으며 Unreal 에디터 도구 호출은 검증하지 않았다."
---

# 작업 - workflow-codex-mcp-connection

- 원본: `.agents/workflow/tasks/workflow-codex-mcp-connection.md`
- 원자료 사본: `.raw/captured/cb5c1ef3d9ba32c338e232757fb8092e8f8015b3214346706c56a879c023f2eb.md`
- 수집: 2026-09-29 UTC · 재확인 기한: 2027-03-28

## 개요

사용자가 Workflow에서 Codex 실행이 MCP 연결 오류로 실패한 원인을 확인해 고치라고 요청한 작업 기록이다(“어떻게 고쳐야할지 확인해서 올바른 방법으로 고쳐주세요.”, 2026-09-30). 상태는 완료(실제 Codex 실행 및 Workflow 회귀 검증 통과)다. 지금 지킬 규칙은 정본 `.agents/workflow/process/index.md`를 본다.

## 원인 조사

- `.codex/config.toml`과 실제 `codex --disable plugins mcp list --json`에서 unreal-mcp가 `http://127.0.0.1:8000/mcp`로 켜져 있었다.
- `Saved/Logs/Wx.log`에 2026-09-29T15:12:53Z 에디터 HttpListener가 8000번 포트 수신을 멈춘 기록이 있고, 사용자 오류는 약 65초 뒤부터 났다. 에디터 MCP 리스너가 꺼진 뒤 Codex가 연결을 시도한 것이다.
- 기존 `Workflow-Providers.cjs`는 정하기(plan)에서만 MCP를 격리하고, 구현(work)에서는 꺼진 서버에도 연결했다.

## 구현 결과

- Workflow Codex 구현 실행 전에 실제 CLI 설정의 unreal-mcp 로컬 HTTP 서버를 확인하고, TCP 연결 거부가 확인된 경우에만 그 실행에서 서버를 뺀다. 정상 서버·원격 서버·다른 MCP·설정 파일은 그대로 둔다.
- 뺀 사실은 콘솔·AI 입력·결과 근거(evidence)에 남기고, 실행하지 못한 에디터 검증을 통과로 처리하지 않도록 안내한다.
- 사전 확인 뒤에 서버가 꺼지는 경우나 HTTP 프로토콜 오류까지 해결하는 변경은 아니다.

## 검증 범위

- AI: 임시 Node 회귀(실제 로컬 TCP 열림·닫힘, 콘솔·입력·근거, 정상·비활성·다른 MCP·원격 유지, 목록 실패), `TestWorkflowProviders.cjs`·`TestWorkflowTestFeedback.cjs`, `node --check`, `git diff --check`.
- 2026-09-30 완료 검증: 실제 `Workflow-Runner.cjs` → 설치된 Codex → 실제 모델 응답으로, 8000번 서버가 꺼진 상태에서 MCP_SMOKE_OK와 제외 근거를, 별도 로컬 테스트 MCP 서버로 initialize·tools/list와 MCP_ONLINE_OK(제외 안내 없음)를 확인했다. Unreal 에디터 도구 자체는 부르지 않았다.
- 코드 리뷰는 사용자 원문 “직접 테스트해보고 문제 없으면 완료처리 하세요.”에 따라 AI 검토로 대신했고, 기록은 사람이 직접 코드를 리뷰했다고 적지 않는다.

## 관련 주제

- [[작업 절차(Workflow)]]

## 핵심 주장

- 2026-09-30 Workflow Codex 구현 실행은 꺼진 로컬 unreal-mcp(127.0.0.1:8000)에 연결하다 실패했고, 원인은 에디터 MCP 리스너가 멈춘 뒤 work 실행이 꺼진 서버에도 연결한 것이었다. ^c1
- 수정 뒤 Workflow-Providers.cjs는 Codex 구현 실행 전에 unreal-mcp 로컬 HTTP 서버를 확인해 TCP 연결 거부일 때만 그 실행에서 빼고, 정상·원격·다른 MCP와 설정 파일은 유지하며 제외 사실을 콘솔·AI 입력·결과 근거에 남긴다. ^c2
- MCP 연결 수정은 실제 Codex 모델 응답까지 서버 꺼짐·켜짐 두 경우로 확인했고, 코드 리뷰는 사용자 지시로 AI 검토로 대신했으며 Unreal 에디터 도구 호출은 검증하지 않았다. ^c3
