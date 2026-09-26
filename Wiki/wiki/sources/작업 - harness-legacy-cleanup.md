---
type: source
title: "작업 - harness-legacy-cleanup"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "작업-기록"
  - "워크플로우"
  - "하네스"
summary: "AI 하네스·워크플로우·Wiki의 옛 흔적과 지난 AI 작업이 로컬 Saved에 남긴 파일을 정리하고, 이름에 옛 Wiki가 남은 워크플로우 스크립트를 하는 일에 맞게 바꾼 2026-09-27 작업 기록으로, 체크리스트 6/6 통과로 완료됐다."
source_type: task-record
source_id: src-4e997e9f95cc087a86db
sha256: 03225507b8679f0bb027863cefa33473c986069dc1ddf4400d75696a19208130
authority: primary
independence_key: ".agents/workflow/tasks/harness-legacy-cleanup.md"
review_state: active
refresh_due: 2027-03-25
original_paths:
  - ".agents/workflow/tasks/harness-legacy-cleanup.md"
raw_copy: ".raw/captured/03225507b8679f0bb027863cefa33473c986069dc1ddf4400d75696a19208130.md"
claim_ids:
  - clm-7980fd340e-c1
  - clm-7980fd340e-c2
  - clm-7980fd340e-c3
key_claims:
  - "2026-09-27 사용자 요청으로 AI 하네스·워크플로우·Wiki 쪽의 옛 흔적과 지난 AI 작업이 이 PC의 Saved에 남긴 파일 203개가 휴지통으로 옮겨졌고, 게임 코드·에셋과 엔진·에디터 데이터는 대상에서 빠졌다."
  - "harness-legacy-cleanup 작업은 작업 기록 5개가 Git 밖 로컬 파일에 건 링크 73개를 경로 글자로 바꾸고 근거 문장은 그대로 두었다."
  - "2026-09-27 사용자 추가 요청으로 이름에 옛 Wiki가 남은 워크플로우 스크립트는 모두 쓰이고 있어 지우지 않고 이름만 바꿨고(Wiki-AI.cjs → Workflow-Server.cjs, Export-Wiki.ps1 → Export-WorkflowPage.ps1 등), Wiki-Obsidian.cjs는 그대로 두었다."
---

# 작업 - harness-legacy-cleanup

- 원본: `.agents/workflow/tasks/harness-legacy-cleanup.md`
- 원자료 사본: `.raw/captured/03225507b8679f0bb027863cefa33473c986069dc1ddf4400d75696a19208130.md` (수집 2026-09-26, 재확인 기한 2027-03-25)

## 개요

Wiki 최신화 스킬을 두지 않기로 하면서 AI가 9/22에 지운 옛 프로젝트 스킬의 빈 폴더를 알렸고, 사용자가 그 폴더와 다른 옛 흔적을 모두 지우라고 해서 시작한 2026-09-27 작업 기록이다. 상태는 완료(체크리스트 6/6 통과)다. 범위는 AI 하네스·워크플로우·Wiki 쪽과 지난 AI 작업이 이 PC에 남긴 파일이며, 게임 코드·에셋, 엔진·에디터가 쓰는 데이터, 작업 기록의 이력 문장은 대상이 아니다.

## 요청과 승인

> 이우성 2026-09-27: "네 지우세요. 그리고 그밖에 옛 레거시나 흔적도 전부 지우세요"

> 이우성 2026-09-27: "출처를 알 수 없어 남긴 파일 지워주세요."

> 이우성 2026-09-27: "말씀해주신 스크립트들 중에 안쓰는거나 쓸모없는거는 제거하고, 쓰는 것이라면 더 적절한 이름으로 바꿔주세요."

- 구현 승인: 위 세 대화를 각 범위의 승인으로 기록했다.

## 정리 결과

- Git 밖(이 PC): 빈 스킬 폴더 `.agents/skills/wiki-lint`·`readme-writer`와, `Saved/`에 남은 스크립트·패치·목록·결과·화면 캡처·AI 실행 로그·`BuildDoctor` 로그·오래된 엔진 임시 파일 203개(약 241MB)를 휴지통으로 보냈다. 추가 요청으로 출처를 모르던 `Saved/skill3_head.uasset`과 `Saved/Logs`의 `Mannequin.log`·`Mannequins.log`도 옮겼다. 엔진·에디터 데이터와 쓰는 중인 `Saved/Workflow`·`Saved/AbilitySystemLists`는 남겼다.
- 원격: 버려진 브랜치 `claude/ai-game-workflow-plugin-6sxoqf`(클라우드 세션의 전환 기록 초안 한 커밋 `e9dd2956f`)를 지웠다. 같은 기록은 main에서 이어 썼다([[작업 - wiki-claude-obsidian-migration]]).
- Git 안: 작업 기록 5개(확인 대기 4, 완료 1)의 `Saved` 링크 73개를 경로 글자로 바꿨다. 완료 기록 animnotify-labels는 이번 갱신에서 새 사본으로 다시 수집됐다([[작업 - animnotify-labels]]). 테스트 3개에서 없앤 기능 이름을 찾던 단언과 comment-cleanup 스킬의 worklog 훅 문장을 지웠다.
- 스크립트 이름(추가 요청): `Wiki-AI.cjs` → `Workflow-Server.cjs`, `Start-WikiAI.ps1` → `Start-WorkflowServer.ps1`, `Wiki-AI-Providers.cjs` → `Workflow-Providers.cjs`, `TestWikiProviders.cjs` → `TestWorkflowProviders.cjs`, `wiki-gemini-settings.json` → `workflow-gemini-settings.json`, `Export-Wiki.ps1` → `Export-WorkflowPage.ps1`, `wiki-viewer/` → `workflow-page/`, `TestWikiViewer.cjs` → `TestWorkflowPage.cjs`, `CheckWikiLinks.ps1` → `CheckDocLinks.ps1`. `Wiki-Obsidian.cjs`(Wiki의 claude-obsidian 래퍼)와 `Workflow-Runner.cjs`는 그대로 두었다.
- 이름에 남긴 옛 흔적: 브라우저 저장 키 `wx-wiki-workflow-v1`(바꾸면 저장해 둔 입력을 잃음), 페이지 안 식별자(`renderWikiDiagrams` 등), `Workflow-TestFeedback.cjs`·`test-feedback.js`와 서버 경로 `/test-feedback`.
- 앞선 결정과의 관계: 2026-09-26 워크플로우 종합 점검 Q9는 이 도구 이름을 그대로 두기로 했으나([[작업 - workflow-inspection]]), 2026-09-27 이 작업의 사용자 추가 요청으로 이름을 바꿨다.

## 검증 범위

- AI 항목 6개 통과(도구 실행): 워크플로우 Node 테스트 4개, 문서 링크 37개 오류 0(남은 Saved 링크 0), 하네스, 휴지통 이동 대조(이동 203개·실패 0·남음 0, 추가 3개), 원격 브랜치 삭제 전 대조, 새 이름으로 서버 시작·재시작·테스트·페이지 만들기·`--tasks` 목록과 옛 이름 검색 0.
- 다른 PC에서 옛 서버가 떠 있으면 새 시작 스크립트가 알아보지 못하므로, 옛 서버를 한 번 끄고 `OpenWorkflow.bat`을 다시 실행해야 한다는 안내가 기록에 있다.
- 사람 항목: 2026-09-27 통폐합에서 사람 코드 리뷰는 사용자 결정으로 이번에만 AI 코드 리뷰로 대신했고(다음에는 사람이 리뷰), 휴지통을 보고 비울지 정하는 정리 확인은 확인 대기 기록 `workflow-wrapup-checks.md`로 옮겼다.
- 이 작업은 게임 코드·빌드와 무관하다.

## 관련 주제

- [[작업 절차(Workflow)]]
- [[Wiki 운영]]
- [[작업 - workflow-inspection]]

## 핵심 주장

- 2026-09-27 사용자 요청으로 AI 하네스·워크플로우·Wiki 쪽의 옛 흔적과 지난 AI 작업이 이 PC의 Saved에 남긴 파일 203개가 휴지통으로 옮겨졌고, 게임 코드·에셋과 엔진·에디터 데이터는 대상에서 빠졌다. ^c1
- harness-legacy-cleanup 작업은 작업 기록 5개가 Git 밖 로컬 파일에 건 링크 73개를 경로 글자로 바꾸고 근거 문장은 그대로 두었다. ^c2
- 2026-09-27 사용자 추가 요청으로 이름에 옛 Wiki가 남은 워크플로우 스크립트는 모두 쓰이고 있어 지우지 않고 이름만 바꿨고(Wiki-AI.cjs → Workflow-Server.cjs, Export-Wiki.ps1 → Export-WorkflowPage.ps1 등), Wiki-Obsidian.cjs는 그대로 두었다. ^c3
