---
title: "테스트 결과 처리 AI 선택"
source: "MANUAL"
type: notes
ingested: 2026-09-25
tags: [wx, workflow]
summary: "테스트 결과의 처리 AI를 Codex·Claude Code·Gemini CLI에서 선택하고 재시도 이력을 보존하는 변경"
---

# 요청과 구현 관찰

사용자가 테스트 결과를 처리하는 AI를 Codex 외에도 선택하도록 요청했다. 기준 HEAD `38d4dde08`과 미커밋 작업 트리에서 확인했다.

- 서버의 테스트 결과 list/read 응답에 사용 가능한 제공자 목록을 포함한다. 화면의 처리할 AI 선택은 해당 목록을 사용하고 이전 서버는 Codex 경로만 표시한다.
- 선택한 제공자를 초안·접수·응답 유실 재전송·처리 결과에 보존한다. 실패·중단 후 다른 AI로 재시도할 수 있고 이전 제공자·실패 결과도 attempts에 남긴다. 기존 접수에 provider가 없으면 당시 실행 경로인 Codex로 해석한다. 사용할 수 없는 선택을 다른 제공자로 임의 대체하지 않는다.
- Wiki-AI-Providers의 검토/실행 모드를 구분한다. 기획·설계 검토는 기존 읽기 전용을 유지한다. 테스트 결과 실행은 Codex workspace-write, Claude acceptEdits, Gemini auto_edit를 사용한다. 기존 권한·관리자 제한을 보존하고 전면 권한 우회 옵션을 사용하지 않는다.
- Claude 실행에는 읽기·편집·Bash 도구가 제공되지만 읽기만 미리 허용한다. 추가 승인이 필요한 도구는 비대화형 실행에서 거부되며 permission_denials가 있으면 blocker를 추가한다. Gemini는 기존 관리자 tools.core와 허용 가능한 작업 도구의 교집합을 사용한다. 두 제공자의 권한 거부·미실행은 결과에 남기도록 지시한다.
- 같은 작업 결과 스키마를 사용한다. Claude structured_output과 Gemini response를 읽고 기존 검증·완료/재확인 판정을 적용한다. 임시 응답·스키마·자식용 설정 파일을 정리하고 PID·타임아웃을 기존 실행 관리에 연결한다.
- 테스트 결과 처리 경로의 변경이며 기획·설계 확정 후 별도 구현 실행의 기본 제공자는 기존 Codex다.

# 확인한 실행 문서

- 설치된 Claude Code와 Gemini CLI의 `--help`에서 사용 옵션을 대조했다.
- [Claude Code 비대화형 실행](https://code.claude.com/docs/en/headless): print·JSON Schema·구조화된 출력과 권한 거부의 처리.
- [Gemini CLI 비대화형 실행](https://geminicli.com/docs/cli/headless/): JSON 응답 envelope.
- [Gemini CLI 설정](https://geminicli.com/docs/reference/configuration/): auto_edit와 관리자 설정 위치.

# 입력 식별

| 파일 | SHA-256 |
| --- | --- |
| `.agents/scripts/Wiki-AI-Providers.cjs` | `B0FBE35E01CD80B58598D0A3E605016288D5314E090DBABCB5DE9A0EBE3E3875` |
| `.agents/scripts/Workflow-TestFeedback.cjs` | `5964A0F8698D76F8798C0D5F6902C7A7ACFC1EBEEB89BB013EDDF636A415EA9B` |
| `.agents/scripts/wiki-viewer/test-feedback.js` | `08C1958DB13BCB2DD0BC65432FAB84A9F0F8BA9E993B0EEC4F09B02A7A7E4D61` |

# 검증과 제한

TestWorkflowTestFeedback·TestWorkflowFeedbackUI·TestWikiProviders·TestWorkflowExecution·TestWikiAI·TestWikiViewer·TestWikiSpaces 통과. 모의 CLI·HTTP·DOM으로 제공자별 실행 인자·구조화 응답·설정 보존·권한 거부·임시 파일 정리·선택 유지·중복 방지·AI 변경 재시도·기존 데이터 호환을 검증했다. 실제 Claude/Gemini AI 요청과 브라우저 육안 확인은 수행하지 않았다. 진행 중이던 다른 접수의 실행과 결과 원문은 보존했다.
