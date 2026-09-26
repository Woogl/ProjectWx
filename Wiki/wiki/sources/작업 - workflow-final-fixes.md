---
type: source
title: "작업 - workflow-final-fixes"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "작업-기록"
  - "워크플로우"
  - "Wiki"
summary: "워크플로우 최종 마무리 코드 리뷰의 확인된 지적을 고치고, 실행 제한·자유 답변 질문·기록 열기 방식을 정하며, 워크플로우와 Wiki를 독립으로 관리한다는 원칙에 따라 Wiki 안내의 정리 단계를 뺀 2026-09-27 작업 기록으로, 체크리스트 6/6 통과로 완료됐다."
source_type: task-record
source_id: src-5a9d3c91e03b611a51b4
sha256: 9e5f21eddd56c690e6229d52af085197c8a9342c56298a67c33275b09602ae76
authority: primary
independence_key: ".agents/workflow/tasks/workflow-final-fixes.md"
review_state: active
refresh_due: 2027-03-25
original_paths:
  - ".agents/workflow/tasks/workflow-final-fixes.md"
raw_copy: ".raw/captured/9e5f21eddd56c690e6229d52af085197c8a9342c56298a67c33275b09602ae76.md"
claim_ids:
  - clm-9693278370-c1
  - clm-9693278370-c2
  - clm-9693278370-c3
  - clm-9693278370-c4
key_claims:
  - "이우성은 2026-09-27 AI 실행 제한을 정하기 30분·그 밖 60분으로 하고, 선택지 없는 자유 답변 질문을 허용하며, 대시보드의 기록 열기가 서버에 연결돼 있으면 최신 내용을 보이게 하기로 답했다."
  - "사용자는 2026-09-27 워크플로우와 claude-obsidian 문서는 서로 독립적으로 관리하되 서로를 참고하는 방향을 밝혔고, 이에 따라 Wiki 안내 수집 단계에 넣었던 워크플로우 규칙 요약 페이지를 정본 경로로 줄이는 단계를 뺐다."
  - "workflow-final-fixes 작업은 Codex 정하기(읽기 전용)에서 --disable plugins --disable apps와 mcp list로 고른 설정 MCP 서버의 enabled=false 덮어쓰기로 MCP 서버를 끄고, 전후 비교와 실제 읽기 전용 실행으로 확인했다."
  - "프로젝트 .codex/config.toml의 unreal-mcp가 다른 PC의 Codex에서 mcp list에 나오는지는 workflow-final-fixes 완료 시점에 확인하지 못했다."
---

# 작업 - workflow-final-fixes

- 원본: `.agents/workflow/tasks/workflow-final-fixes.md`
- 원자료 사본: `.raw/captured/9e5f21eddd56c690e6229d52af085197c8a9342c56298a67c33275b09602ae76.md` (수집 2026-09-26, 재확인 기한 2027-03-25)

## 개요

사용자가 워크플로우를 최종 마무리하려고 코드 리뷰를 요청해 시작한 기록이다. 검토자 셋(서버·실행기, 대시보드 화면, 문서·테스트)이 읽기 전용으로 점검했고 AI가 핵심 지적을 코드와 실측으로 다시 확인했다. 상태는 완료(체크리스트 6/6 통과)다. 여기서 정한 규칙의 현재 내용은 정본 `.agents/workflow/process/index.md`와 `Wiki/README.md`를 본다.

## 요청과 결정

> 이우성 2026-09-26: "워크플로우를 최종 마무리하려고 합니다. 코드 리뷰해주세요."

> 이우성 2026-09-27: "네 고칩시다"

- 사용자가 결과를 보고 "네 고칩시다"라고 해, AI 대화에서 명확히 지시한 요청은 구현 승인이라는 규칙에 따라 계획을 승인된 것으로 보고 정할 것 셋만 물었다.
- Q1 AI 실행 시간 제한(당시 모든 실행 30분 강제 종료): 이우성 2026-09-27 "작업 60분·정하기 30분".
- Q2 선택지가 없는 질문(자유 답변): 이우성 2026-09-27 "허용하고 정본에 적기".
- Q3 대시보드의 기록 열기가 보이는 내용: 이우성 2026-09-27 "최신 내용 보이기".

> 이우성 2026-09-27: "워크플로우가 claude-obsidian의 편찬 작업을 방해할까요? 저는 워크플로우와 claude-obsidian의 문서는 서로 독립적으로 관리되지만, 서로를 참고해서 DB를 더욱 견고하게 쌓아올리는 방향을 추구합니다." → (AI가 Wiki 안내 단계를 빼고 서술 규칙을 다듬자고 제안한 뒤) "이걸 추가하는게 나을까요?" → (AI가 다듬는 문장은 더하지 않고 단계 한 줄만 빼자고 답한 뒤) "네 그렇게 합시다."

## 구현 결과

- 커밋: `7a1e8a0e4`(코드·테스트·작업 절차·Wiki 안내).
- 결과가 틀어지는 결함 수정: 승인 전 계획의 빈 줄 소실, 계획의 `#` 줄을 모두 목록으로 바꾸던 문제, AI 요청문에 웹 처리 맥락이 없어 AI가 기록을 직접 고치면 결과가 버려지던 문제, 꺾쇠 글자(`<UWxAbilityBase>` 등)가 뒤 글을 삼키거나 사라지던 문제.
- 드문 결함 수정: 서버 시작 복구 순서, Wiki 작업 트리의 sparse·LF 설정을 갱신마다 다시 적용하고 남은 rebase·merge를 정리, 완료된 작업의 추가 요청·테스트 결과·터미널을 서버도 거부, 표 형식 오류가 있으면 답변·승인·추가 요청·다시 시도도 받지 않음(`tableError`), 실행 제한을 넘으면 하위 프로세스까지 끝냄.
- 화면: 기록을 못 읽을 때 이유 표시, 대시보드에 들어올 때마다 목록 다시 받기, 기록 열기는 서버가 연결돼 있으면 최신 내용, 목차 이동, 접근성(`aria-current`·제목 포커스·포커스 표시·글자 대비).
- Codex 정하기 격리: `codex exec`의 `--disable plugins --disable apps`로 플러그인 서버를 끄고, 설정 파일의 MCP 서버는 `mcp list --json`에서 켜진 것만 골라 `-c mcp_servers.<이름>.enabled=false`로 끈다. 목록을 못 읽으면 정하기를 시작하지 않는다. `--ignore-user-config`는 모델·샌드박스 설정까지 버려 쓰지 않았고, `-c mcp_servers={}`는 병합이라 효과가 없으며 플러그인 서버를 `-c`로 끄면 Codex가 오류를 낸다(실측).

## Wiki와의 관계 점검 (2026-09-27)

- 막지 않는 부분: Wiki는 Wiki 갱신만 쓰고, 원자료는 완료된 작업 기록을 바이트 그대로 떠서 한 방향으로만 가며, Wiki 갱신의 커밋 경로(`Wiki/wiki`·`Wiki/.raw`)와 작업 쪽 커밋 경로가 겹치지 않는다.
- 끼어든 곳: 이번 구현에서 `Wiki/README.md` 수집 단계에 넣은 "워크플로우 규칙을 요약한 기존 페이지를 정본 경로로 줄이고 옛 주장을 새 주장이 대체" 단계가 원자료 변화 없이 워크플로우 정책으로 Wiki를 고쳐 쓰게 해, 사용자 원칙과 순정 방식(주장은 증거로만 바꾸고 반대 증거 보존)에 어긋났다. 그 단계를 뺐고 서술 규칙은 더하지 않았다.
- 늦어지는 곳: 완료된 기록만 수집하므로 확인 대기 기록은 사람 항목이 끝날 때까지 Wiki에 들어가지 못한다. 확인 대기 기록까지 잠정 수집하는 안은 사본이 쌓이고 검증 안 된 내용이 들어가 권하지 않았다.
- AI 판단: 결정 이력 편찬은 순정 wiki-ingest의 기본 동작이고, 서술 규칙의 "정본 경로만"이 글자 그대로 읽혀 이력이 빠지면 "만"을 빼는 정도로 고친다.

## 검증 범위

- AI 항목 6개 통과(도구 실행): 워크플로우 Node 테스트 4개, 결함 주입 44건 모두 검출, Codex 정하기 격리 실측(고치기 전 `cua_repl`이 보였고 고친 뒤 보이는 도구는 functions·clock·image_gen·web·collaboration뿐), 응답 없는 가짜 서버를 둔 서버 재시작 판단, 헤드리스 Edge 1440px·375px 18개 항목(확인 중 찾은 목차 결함 1건 수정 뒤 재확인), 문서 링크 36개 오류 0·하네스.
- 남은 확인: 프로젝트 `.codex/config.toml`의 unreal-mcp가 다른 PC의 Codex에서 `mcp list`에 나오는지는 이 PC에서 재현하지 못했다.
- 사람 항목: 2026-09-27 통폐합에서 사람 코드 리뷰는 사용자 결정으로 이번에만 AI 코드 리뷰로 대신했고(다음에는 사람이 리뷰), 대시보드 사용감은 확인 대기 기록 `workflow-wrapup-checks.md`로 옮겼다.
- 이 작업은 게임 코드·빌드와 무관하다.

## 관련 주제

- [[작업 절차(Workflow)]]
- [[Wiki 운영]]
- [[작업 - dashboard-work-tab-split]]
- [[작업 - workflow-inspection]]

## 핵심 주장

- 이우성은 2026-09-27 AI 실행 제한을 정하기 30분·그 밖 60분으로 하고, 선택지 없는 자유 답변 질문을 허용하며, 대시보드의 기록 열기가 서버에 연결돼 있으면 최신 내용을 보이게 하기로 답했다. ^c1
- 사용자는 2026-09-27 워크플로우와 claude-obsidian 문서는 서로 독립적으로 관리하되 서로를 참고하는 방향을 밝혔고, 이에 따라 Wiki 안내 수집 단계에 넣었던 워크플로우 규칙 요약 페이지를 정본 경로로 줄이는 단계를 뺐다. ^c2
- workflow-final-fixes 작업은 Codex 정하기(읽기 전용)에서 --disable plugins --disable apps와 mcp list로 고른 설정 MCP 서버의 enabled=false 덮어쓰기로 MCP 서버를 끄고, 전후 비교와 실제 읽기 전용 실행으로 확인했다. ^c3
- 프로젝트 .codex/config.toml의 unreal-mcp가 다른 PC의 Codex에서 mcp list에 나오는지는 workflow-final-fixes 완료 시점에 확인하지 못했다. ^c4
