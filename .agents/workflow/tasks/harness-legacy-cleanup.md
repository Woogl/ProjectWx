# 하네스 레거시 정리

상태: 확인 대기 · 체크리스트 5/7 통과
다음 행동: 사람은 코드 리뷰와 정리 확인을 한다.

- 날짜: 2026-09-27
- 계기: Wiki 최신화 스킬을 두지 않기로 하면서 AI가 9/22에 지운 옛 프로젝트 스킬의 빈 폴더를 알렸고, 사용자가 그 폴더와 다른 옛 흔적을 모두 지우라고 했다. 범위는 AI 하네스·워크플로우·Wiki 쪽과 지난 AI 작업이 이 PC에 남긴 파일이다. 게임 코드·에셋, 엔진·에디터가 쓰는 데이터, 작업 기록의 이력 문장은 대상이 아니다.

## 요청

- 요청 · 이우성 2026-09-27

> 네 지우세요. 그리고 그밖에 옛 레거시나 흔적도 전부 지우세요

## 구현 계획

1. 빈 스킬 폴더 `.agents/skills/wiki-lint`·`.agents/skills/readme-writer`를 지운다(9/22에 SKILL.md만 지우고 남은 폴더, Git 밖).
2. 원격의 버려진 브랜치 `claude/ai-game-workflow-plugin-6sxoqf`를 지운다. 9/25 클라우드 세션이 만든 전환 기록 초안 한 커밋(`e9dd2956f`)뿐이고, 같은 기록은 main에서 이어 썼다.
3. 작업 기록 5개가 Git 밖 로컬 파일(`Saved/Logs`·`Saved/Automation`·`Saved/Tests`)에 건 링크 73개를 경로 글자로 바꾼다. 다른 PC에서는 이미 깨진 링크이고, 파일을 지우면 여기서도 깨진다. 근거 문장은 그대로 둔다.
4. 테스트와 스킬에 남은 옛 기능 이름을 지운다.
   - 없앤 Wiki 페이지·웹 작업 경로·`codeVersion`·`blockers`·옛 요청문 문구가 돌아오지 않는지 보는 단언과, 패널마다 AI를 고르지 않는지 보는 단언.
   - comment-cleanup 스킬의 worklog 훅 문장.
   - 동작 검사는 중립 이름으로 남긴다: 모르는 경로는 404, 단계 밖 칸은 버림, 문서 데이터 칸.
5. 지난 AI 작업이 이 PC에 남긴 Git 밖 파일을 휴지통으로 보낸다(되살릴 수 있게 영구 삭제하지 않는다).
   - `Saved/` 바로 아래의 스크립트·패치·목록·결과·화면 캡처와 오래된 엔진 임시 파일(`*.tmp`).
   - `Saved/Submit`·`Saved/Tests`·`Saved/Automation`·`Saved/Diff` 전체, `Saved/Temp`의 AI 파일(엔진의 `Win64`는 남김).
   - `Saved/Logs`의 AI 실행 로그와 `BuildDoctor` 로그(엔진 로그 `Wx*`·`UnrealVersionSelector*`·`AutoSDKInfo*`·`cef3`·`UnrealPak`·`WorldPartition`은 남김).
   - 남기는 것: 엔진·에디터 데이터(Autosaves·Config·SaveGames·Collections·AssetData·SourceControl·Screenshots·Crashes·Cooked·StagedBuilds·Shaders 등), 쓰는 중인 `Saved/Workflow`·`Saved/AbilitySystemLists`, 출처를 모르는 `Saved/skill3_head.uasset`(5/14, 기록·Git 이력 어디에도 없음).
6. 검증: 워크플로우 테스트 4개, 문서 링크 검사(Saved 링크 0), 하네스 테스트, 휴지통으로 보낸 목록 대조.
   - 테스트 체크리스트 초안: AI 항목은 위 검사들, 사람 항목은 코드 리뷰와 정리 확인(휴지통을 보고 비울지 정함).

구현 승인: 이우성 2026-09-27 (대화: "네 지우세요. 그리고 그밖에 옛 레거시나 흔적도 전부 지우세요")

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 워크플로우 Node 테스트 | TestWorkflowTestFeedback·TestWikiProviders·TestWorkflowFeedbackUI·TestWikiViewer | AI | 통과 | 4개 모두 exit 0 |
| 문서 링크 | CheckWikiLinks.ps1 | AI | 통과 | 37개 문서 오류 0. 작업 기록의 Saved 링크 73개를 경로 글자로 바꿔 남은 Saved 링크 0 |
| 하네스 | TestAgentHarness.ps1 | AI | 통과 | exit 0 |
| 휴지통 이동 대조 | 옮길 목록을 먼저 만들어 검토한 뒤 휴지통으로 보내고 남은 것이 없는지 확인 | AI | 통과 | 203개 이동, 실패 0, 남음 0, 약 241MB. 빈 스킬 폴더 2, Saved 바로 아래 82, AI 폴더 4(Submit·Tests·Automation·Diff), Temp 5, 로그 110(BuildDoctor 폴더 포함) |
| 원격 브랜치 | 지운 브랜치의 기록이 main에 있는지, 지운 커밋이 로컬에 남는지 확인 | AI | 통과 | main의 전환 기록 310줄(브랜치는 84줄 초안), e9dd2956f 로컬에 있음 |
| 코드 리뷰 | 변경: 테스트 3개(옛 기능 이름을 찾던 단언을 지우고 동작 검사는 중립 이름으로), comment-cleanup 스킬 한 문장, 작업 기록 5개의 Saved 링크를 경로 글자로. 볼 점: 지운 단언이 지금 동작을 가리던 것은 아닌지 | 사람 | 대기 |  |
| 정리 확인 | 휴지통에서 이번에 보낸 항목을 보고 되살릴 것이 없으면 비운다. 출처를 모르는 Saved/skill3_head.uasset(5/14)을 지울지 정한다. | 사람 | 대기 |  |

## 정리 결과 · 2026-09-27

- Git 밖(이 PC): 빈 스킬 폴더 둘과, 지난 AI 작업이 Saved에 남긴 스크립트·패치·목록·결과·화면 캡처·실행 로그·오래된 엔진 임시 파일 203개를 휴지통으로 보냈다. 되살리려면 휴지통에서 원래 위치로 복원한다.
- 원격: `claude/ai-game-workflow-plugin-6sxoqf`를 지웠다. 다시 필요하면 로컬에 남은 커밋으로 `git push origin e9dd2956f:refs/heads/claude/ai-game-workflow-plugin-6sxoqf`.
- Git 안: 작업 기록 5개(확인 대기 4, 완료 1)의 Saved 링크 73개를 경로 글자로 바꿨다. 완료 기록(animnotify-labels)은 다음 Wiki 갱신이 새 사본으로 다시 수집한다. 테스트 3개에서 없앤 기능 이름을 찾던 단언을 지우고, comment-cleanup 스킬의 worklog 훅 문장을 지웠다.
- 남긴 것: 엔진·에디터 데이터, 쓰는 중인 Saved/Workflow·Saved/AbilitySystemLists, 출처를 모르는 Saved/skill3_head.uasset(5/14)과 Saved/Logs의 Mannequin.log·Mannequins.log(3/8, 명령줄 없는 로그), 작업 기록의 이력 문장.
- 계획 밖이라 하지 않은 것: 스크립트 이름에 남은 옛 "Wiki"(Export-Wiki.ps1·Wiki-AI.cjs·Start-WikiAI.ps1·wiki-viewer·TestWikiViewer). 지금은 Workflow 페이지와 로컬 서버를 가리키지만, 이름 바꾸기는 지우기와 달리 실행 파일·배치 파일·문서를 함께 고쳐야 해 따로 정한다.
