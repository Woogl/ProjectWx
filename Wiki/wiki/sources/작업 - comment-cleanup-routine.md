---
type: source
title: "작업 - comment-cleanup-routine"
created: 2026-09-27
updated: 2026-09-27
status: developing
tags:
  - "source"
  - "작업-기록"
  - "워크플로우"
summary: "일일 주석 정리 Routine이 Git에 없는 스킬 경로를 가리키고 얕은 클론 경계로 대상이 저장소 전체로 잡힐 수 있던 결함을 고쳐 절차를 스킬 6절로 옮긴 2026-09-27 작업 기록으로, 체크리스트 5/5 통과로 완료됐다."
source_type: task-record
source_id: src-1c802b86b4e0c4b04c6f
sha256: 6181e48bb2dd601aa5de9613f0bb5dd0f28ea92203ce0b50e35803c606d1b738
authority: primary
independence_key: ".agents/workflow/tasks/comment-cleanup-routine.md"
review_state: active
refresh_due: 2027-03-25
original_paths:
  - ".agents/workflow/tasks/comment-cleanup-routine.md"
raw_copy: ".raw/captured/6181e48bb2dd601aa5de9613f0bb5dd0f28ea92203ce0b50e35803c606d1b738.md"
claim_ids:
  - clm-56dd9d7bc4-c1
  - clm-56dd9d7bc4-c2
  - clm-56dd9d7bc4-c3
  - clm-56dd9d7bc4-c4
key_claims:
  - "매일 06:00(KST) main에 바로 푸시하는 클라우드 Routine 「일일 주석 정리 + 푸시」는 Git에 없는 `.claude/skills/comment-cleanup/SKILL.md`를 가리켰고, 얕은 클론 경계 커밋 때문에 대상이 저장소 전체로 잡힐 수 있었다(9/26 실행이 523개로 잡았다가 126개로 바로잡음)."
  - "이우성의 2026-09-27 승인(\"네, 고쳐주세요\")으로 주석 정리 무인 실행 절차는 `.agents/skills/comment-cleanup/SKILL.md` 6절로 옮겨졌고, 대상은 기간 앞 마지막 main 커밋 BASE와 기간 끝 앞 마지막 커밋 TIP 사이 범위로 고르며 푸시는 `git push origin HEAD:main`으로 한다."
  - "얕은 클론 재현에서 6절 절차는 9/25 기간 대상 129개로 전체 이력의 정답과 목록까지 같았고(옛 절차 523개), 2026-09-27 수동 실행 한 번이 6절대로 대상 0개를 구해 커밋·푸시 없이 끝났다."
  - "comment-cleanup-routine 작업은 작업 절차에서 Git에 파일이 없는 `Docs/Programmer/` 읽기 전용 규칙을 지웠고, 기록 형식 두 건과 옛 이름 잔재는 고치지 않았으며 꺼진 옛 Routine 3개 삭제는 사용자가 따로 정한다고 남겼다."
---

# 작업 - comment-cleanup-routine

- 원본: `.agents/workflow/tasks/comment-cleanup-routine.md`
- 원자료 사본: `.raw/captured/6181e48bb2dd601aa5de9613f0bb5dd0f28ea92203ce0b50e35803c606d1b738.md` (수집 2026-09-27, 재확인 기한 2027-03-25)

## 개요

사용자의 워크플로우 점검 요청에서 일일 주석 정리 Routine의 결함 두 가지를 실행 기록으로 확인하고 고친 작업 기록이다. 상태는 완료(체크리스트 5/5 통과)다. 지금 지킬 규칙은 정본 `.agents/workflow/process/index.md`와 `.agents/skills/comment-cleanup/SKILL.md`를 본다.

## 요청과 결정

> 이우성 2026-09-27: "우리 프로젝트의 워크플로우를 점검해주세요." → (AI가 점검 결과와 고칠 것·그대로 둘 것을 제안한 뒤) "고치는게 나을까요?" → (AI가 중간 두 건과 작업 절차 한 줄은 고치고 나머지는 두자고 답한 뒤) "네, 고쳐주세요."

- 확인한 결함: (1) Routine 지시문이 Git에 없는 로컬 junction 경로의 스킬을 가리켰다. 9/26 06:00 실행이 스스로 `.agents/skills/`를 찾아 따랐다. (2) 얕은 클론을 기간 시작 시각까지만 받아 기간의 첫 커밋이 부모 없는 경계가 되어 저장소 전체가 대상으로 잡혔다. 같은 실행이 523개로 잡았다가 알아채고 126개로 바로잡았다. 알아채지 못하면 저장소 전체 주석 변경이 사람 리뷰 없이 main에 올라간다.
- 결정: 이 두 가지와 없는 폴더를 가리키는 작업 절차 규칙 한 줄만 고친다. 기록 형식 두 건과 옛 이름 잔재는 리뷰 부담에 비해 이득이 없어 고치지 않는다. 꺼진 옛 Routine 3개 삭제는 사용자가 따로 정한다.
- 구현 승인: 이우성 2026-09-27(대화: "네, 고쳐주세요").

## 구현 결과

- 무인 실행 절차를 Routine 지시문에서 comment-cleanup 스킬의 새 6절로 옮기고, 지시문에는 스킬 링크 두 문장만 둔다(Wiki 정기 갱신과 같은 방식).
- 대상 결정: 얕은 클론이면 기간보다 7일 앞까지 받고, BASE~TIP 범위로 파일을 고른다. BASE가 비면 `--deepen=200`으로 더 받는다.
- 푸시는 `git push origin HEAD:main`이라 세션이 `claude/` 브랜치에서 시작해도 main에 올라간다. "줄인다"의 재사용할 결론은 파일로 남기지 않고 보고에 적는다.
- Routine 지시문 교체와 함께 웹 저장이 붙인 `outcomes`와 쓰지 않는 커넥터를 뗐다(커넥터는 `clear_mcp_connections: true`로 뗌). 환경·일정(`0 21 * * *` UTC)·모델·허용 도구는 그대로다.
- 작업 절차 기록 절의 "`Docs/Programmer/`는 읽기만 합니다"를 지웠다.

## 검증 범위

- AI: blob 없는 얕은 클론 재현(9/25 기간 옛 523개·6절 129개 = 정답 129개, 9/24 108=108, 9/26 0=0, BASE가 빈 경우 `--deepen=200`이 경계를 9/7로 옮김), 문서 링크 0 errors, Routine update 응답 확인, 수동 실행 `cse_01JJDGWXzBLEiV4TGpA2MJ7F`(2026-09-27 17:32 KST)가 6절대로 BASE `d707ceb5`·TIP `128fc895`를 구해 대상 0개로 종료.
- 사람: 코드 리뷰 woogle 2026-09-27 통과.
- 이 작업은 게임 코드 동작과 무관하다.

## 관련 주제

- [[작업 절차(Workflow)]]

## 핵심 주장

- 매일 06:00(KST) main에 바로 푸시하는 클라우드 Routine 「일일 주석 정리 + 푸시」는 Git에 없는 `.claude/skills/comment-cleanup/SKILL.md`를 가리켰고, 얕은 클론 경계 커밋 때문에 대상이 저장소 전체로 잡힐 수 있었다(9/26 실행이 523개로 잡았다가 126개로 바로잡음). ^c1
- 이우성의 2026-09-27 승인("네, 고쳐주세요")으로 주석 정리 무인 실행 절차는 `.agents/skills/comment-cleanup/SKILL.md` 6절로 옮겨졌고, 대상은 기간 앞 마지막 main 커밋 BASE와 기간 끝 앞 마지막 커밋 TIP 사이 범위로 고르며 푸시는 `git push origin HEAD:main`으로 한다. ^c2
- 얕은 클론 재현에서 6절 절차는 9/25 기간 대상 129개로 전체 이력의 정답과 목록까지 같았고(옛 절차 523개), 2026-09-27 수동 실행 한 번이 6절대로 대상 0개를 구해 커밋·푸시 없이 끝났다. ^c3
- comment-cleanup-routine 작업은 작업 절차에서 Git에 파일이 없는 `Docs/Programmer/` 읽기 전용 규칙을 지웠고, 기록 형식 두 건과 옛 이름 잔재는 고치지 않았으며 꺼진 옛 Routine 3개 삭제는 사용자가 따로 정한다고 남겼다. ^c4
