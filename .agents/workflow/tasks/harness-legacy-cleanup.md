# 하네스 레거시 정리

상태: 확인 대기 · 체크리스트 6/8 통과
다음 행동: 사람은 코드 리뷰와 정리 확인을 한다.

- 날짜: 2026-09-27
- 계기: Wiki 최신화 스킬을 두지 않기로 하면서 AI가 9/22에 지운 옛 프로젝트 스킬의 빈 폴더를 알렸고, 사용자가 그 폴더와 다른 옛 흔적을 모두 지우라고 했다. 범위는 AI 하네스·워크플로우·Wiki 쪽과 지난 AI 작업이 이 PC에 남긴 파일이다. 게임 코드·에셋, 엔진·에디터가 쓰는 데이터, 작업 기록의 이력 문장은 대상이 아니다.

## 요청

- 요청 · 이우성 2026-09-27

> 네 지우세요. 그리고 그밖에 옛 레거시나 흔적도 전부 지우세요

- 추가 요청 · 이우성 2026-09-27

> 출처를 알 수 없어 남긴 파일 지워주세요.

- 추가 요청 · 이우성 2026-09-27

> 말씀해주신 스크립트들 중에 안쓰는거나 쓸모없는거는 제거하고, 쓰는 것이라면 더 적절한 이름으로 바꿔주세요.

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
   - 남기는 것: 엔진·에디터 데이터(Autosaves·Config·SaveGames·Collections·AssetData·SourceControl·Screenshots·Crashes·Cooked·StagedBuilds·Shaders 등), 쓰는 중인 `Saved/Workflow`·`Saved/AbilitySystemLists`, 출처를 모르는 `Saved/skill3_head.uasset`(5/14, 기록·Git 이력 어디에도 없음). 이 파일과 `Saved/Logs/Mannequin.log`·`Mannequins.log`는 추가 요청으로 지웠다(아래 정리 결과).
6. 검증: 워크플로우 테스트 4개, 문서 링크 검사(Saved 링크 0), 하네스 테스트, 휴지통으로 보낸 목록 대조.
   - 테스트 체크리스트 초안: AI 항목은 위 검사들, 사람 항목은 코드 리뷰와 정리 확인(휴지통을 보고 비울지 정함).
7. 추가 요청: 이름에 옛 "Wiki"가 남은 스크립트를 정리한다. 모두 대시보드 실행 파일·서로의 코드·테스트·module-review 스킬·작업 절차에서 쓰여 지울 것은 없고, 하는 일에 맞게 이름을 바꾼다.
   - `Wiki-AI.cjs` → `Workflow-Server.cjs`(대시보드 로컬 서버와 `--tasks` 목록), `Start-WikiAI.ps1` → `Start-WorkflowServer.ps1`
   - `Wiki-AI-Providers.cjs` → `Workflow-Providers.cjs`(AI CLI 호출), `TestWikiProviders.cjs` → `TestWorkflowProviders.cjs`, `wiki-gemini-settings.json` → `workflow-gemini-settings.json`
   - `Export-Wiki.ps1` → `Export-WorkflowPage.ps1`, `wiki-viewer/` → `workflow-page/`, `TestWikiViewer.cjs` → `TestWorkflowPage.cjs`
   - `CheckWikiLinks.ps1` → `CheckDocLinks.ps1`(작업 절차 문서와 README의 링크 검사)
   - 그대로 둔다: `Wiki-Obsidian.cjs`(Wiki의 claude-obsidian 래퍼라 이름이 맞음), `Workflow-Runner.cjs`, 작업 기록의 이력 문장, 브라우저 저장 키 `wx-wiki-workflow-v1`(바꾸면 저장해 둔 입력을 잃음).
   - 참조를 고친다: `OpenWorkflow.bat`, 코드·테스트, vendor README, module-review 스킬, 작업 절차의 `--tasks` 명령.
   - 실행 중인 서버는 옛 경로로 떠 있어 새 시작 스크립트가 알아보지 못하므로, 바꾼 뒤 한 번 멈추고 새 이름으로 다시 띄운다.
   - 검증: 새 이름의 테스트 4개, 문서 링크, 하네스, 새 이름으로 서버 시작과 페이지 만들기.

구현 승인: 이우성 2026-09-27 (대화: "네 지우세요. 그리고 그밖에 옛 레거시나 흔적도 전부 지우세요". 출처를 모르던 파일 삭제는 대화: "출처를 알 수 없어 남긴 파일 지워주세요". 스크립트 이름 정리는 대화: "말씀해주신 스크립트들 중에 안쓰는거나 쓸모없는거는 제거하고, 쓰는 것이라면 더 적절한 이름으로 바꿔주세요.")

## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 워크플로우 Node 테스트 | TestWorkflowTestFeedback·TestWikiProviders·TestWorkflowFeedbackUI·TestWikiViewer | AI | 통과 | 4개 모두 exit 0 |
| 문서 링크 | CheckWikiLinks.ps1 | AI | 통과 | 37개 문서 오류 0. 작업 기록의 Saved 링크 73개를 경로 글자로 바꿔 남은 Saved 링크 0 |
| 하네스 | TestAgentHarness.ps1 | AI | 통과 | exit 0 |
| 휴지통 이동 대조 | 옮길 목록을 먼저 만들어 검토한 뒤 휴지통으로 보내고 남은 것이 없는지 확인 | AI | 통과 | 203개 이동, 실패 0, 남음 0, 약 241MB. 빈 스킬 폴더 2, Saved 바로 아래 82, AI 폴더 4(Submit·Tests·Automation·Diff), Temp 5, 로그 110(BuildDoctor 폴더 포함). 추가 요청으로 출처를 모르던 3개(약 0.4MB)도 옮김, 남음 0 |
| 원격 브랜치 | 지운 브랜치의 기록이 main에 있는지, 지운 커밋이 로컬에 남는지 확인 | AI | 통과 | main의 전환 기록 310줄(브랜치는 84줄 초안), e9dd2956f 로컬에 있음 |
| 스크립트 이름 정리 | 새 이름으로 서버 시작·재시작, 테스트 4개, 문서 링크, 하네스, 페이지 만들기, --tasks 목록을 돌리고 옛 이름이 남은 곳을 찾음 | AI | 통과 | 옛 경로로 떠 있던 서버를 멈추고 Start-WorkflowServer.ps1로 띄운 뒤 다시 실행해도 새 서버를 알아보고 바꿔 띄움. 테스트 4개 exit 0, 37개 문서 링크 오류 0, 하네스 exit 0, Export-WorkflowPage.ps1로 페이지 생성. 스크립트·스킬·작업 절차·실행 파일·README에 옛 이름 0(작업 기록의 이력 문장은 그대로) |
| 코드 리뷰 | 변경: 테스트 3개(옛 기능 이름을 찾던 단언을 지우고 동작 검사는 중립 이름으로), comment-cleanup 스킬 한 문장, 작업 기록 5개의 Saved 링크를 경로 글자로, 스크립트 9개 이름과 그 참조(OpenWorkflow.bat·코드·테스트·vendor README·module-review 스킬·작업 절차). 볼 점: 지운 단언이 지금 동작을 가리던 것은 아닌지, 새 이름이 하는 일과 맞는지 | 사람 | 대기 |  |
| 정리 확인 | 휴지통에서 이번에 보낸 항목을 보고 되살릴 것이 없으면 비운다. | 사람 | 대기 |  |

## 정리 결과 · 2026-09-27

- Git 밖(이 PC): 빈 스킬 폴더 둘과, 지난 AI 작업이 Saved에 남긴 스크립트·패치·목록·결과·화면 캡처·실행 로그·오래된 엔진 임시 파일 203개를 휴지통으로 보냈다. 되살리려면 휴지통에서 원래 위치로 복원한다.
- 원격: `claude/ai-game-workflow-plugin-6sxoqf`를 지웠다. 다시 필요하면 로컬에 남은 커밋으로 `git push origin e9dd2956f:refs/heads/claude/ai-game-workflow-plugin-6sxoqf`.
- Git 안: 작업 기록 5개(확인 대기 4, 완료 1)의 Saved 링크 73개를 경로 글자로 바꿨다. 완료 기록(animnotify-labels)은 다음 Wiki 갱신이 새 사본으로 다시 수집한다. 테스트 3개에서 없앤 기능 이름을 찾던 단언을 지우고, comment-cleanup 스킬의 worklog 훅 문장을 지웠다.
- 남긴 것: 엔진·에디터 데이터, 쓰는 중인 Saved/Workflow·Saved/AbilitySystemLists, 작업 기록의 이력 문장.
- 추가 요청: 출처를 몰라 남겼던 Saved/skill3_head.uasset(5/14)과 Saved/Logs의 Mannequin.log·Mannequins.log(3/8, 명령줄 없는 로그)도 휴지통으로 보냈다.
- 계획 밖이라 하지 않은 것: 스크립트 이름에 남은 옛 "Wiki"(Export-Wiki.ps1·Wiki-AI.cjs·Start-WikiAI.ps1·wiki-viewer·TestWikiViewer). 지금은 Workflow 페이지와 로컬 서버를 가리키지만, 이름 바꾸기는 지우기와 달리 실행 파일·배치 파일·문서를 함께 고쳐야 해 따로 정한다.
- 추가 요청(스크립트 이름): 말씀드린 스크립트는 모두 쓰이고 있어 지우지 않고 이름만 바꿨다. `Wiki-AI.cjs` → `Workflow-Server.cjs`, `Start-WikiAI.ps1` → `Start-WorkflowServer.ps1`, `Wiki-AI-Providers.cjs` → `Workflow-Providers.cjs`, `TestWikiProviders.cjs` → `TestWorkflowProviders.cjs`, `wiki-gemini-settings.json` → `workflow-gemini-settings.json`, `Export-Wiki.ps1` → `Export-WorkflowPage.ps1`, `wiki-viewer/` → `workflow-page/`, `TestWikiViewer.cjs` → `TestWorkflowPage.cjs`, `CheckWikiLinks.ps1` → `CheckDocLinks.ps1`. 이 PC의 서버는 새 이름으로 다시 띄웠다. 다른 PC는 옛 서버가 떠 있으면 새 시작 스크립트가 알아보지 못하므로, 그때는 옛 서버(작업 관리자의 node, 명령줄에 Wiki-AI.cjs)를 한 번 끄고 OpenWorkflow.bat을 다시 실행한다.
- 이름에 남긴 옛 흔적: 브라우저 저장 키 `wx-wiki-workflow-v1`(바꾸면 저장해 둔 입력을 잃음)과 페이지 안의 식별자(`renderWikiDiagrams`·`.wiki-diagram`·`wiki-data`). 테스트 결과 기능 시절 이름이 남은 `Workflow-TestFeedback.cjs`·`test-feedback.js`와 서버 경로 `/test-feedback`도 이번 범위(말씀드린 스크립트) 밖이라 두었다.
