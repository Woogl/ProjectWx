# Claude 활용

WX 개발에서 Claude(Claude Code)를 쓰는 방향과, 저장소에 갖춰 둔 지침·스킬·MCP·클라우드 루틴을 모은 문서다.

## 기획

- 게임 프로그래밍의 AI 사용은 보수적으로 접근한다. 24시간 자동 개발보다 학습·의사결정·검증의 파트너로 개인 능력을 키우는 쪽을 택한다.
- 근거는 세 가지다. 코드베이스가 커서 긴 컨텍스트에서 토큰이 늘고 정확도가 떨어진다. AI가 게임을 플레이하거나 버그를 찾기 어려워 사람 개입을 없앨 수 없다. 이해하지 못한 코드가 쌓이면 이해 부채가 된다.
- AI가 줄여 준 시간만큼 사람의 코드 리뷰는 더 엄격해야 한다.
- 사람은 AI의 소비자가 아니라 관리자다. 결과가 의도와 다르면 고쳐 달라고 하고, 검토 없이 쓰지 않고, 지시하지 않은 작업을 방치하지 않는다.
- 잘 쓰는 방법은 프롬프트(원하는 결과까지 구체적으로 반복 질문), 하네스(역할·`AGENTS.md`·스킬로 일할 환경을 갖춤), 스캐폴딩(구조화된 컨텍스트 제공), MCP(외부 도구 연결), 팀 노하우 공유다.
- 공용 스킬은 누구나 추가할 수 있다.
- 팀 규칙: Unreal MCP로 만든 것은 반드시 사람이 검토한다(07-19). 기획자는 이해하지 못한 AI 생성물을 빼고, AI가 설계한 내용을 프로그래머에게 넘기지 않으며, 기획 의도를 AI에 맡기지 않는다(07-31).

## 구현

### 지침

- 루트 `AGENTS.md`가 모든 AI의 공통 지침이다(역할, 한국어 응답, 코딩 규칙, AI 작업 안내).
- Claude Code와 Codex는 `AGENTS.md`를 바로 읽고, Gemini CLI는 `.gemini/settings.json`의 `context.fileName`으로 읽는다.

### 스킬

- 정본은 `.agents/skills/`이고, Claude Code는 junction `.claude/skills`로 읽는다.
- junction은 git이 추적하지 않으므로 클론 직후 저장소 루트에서 `cmd /c mklink /J .claude\skills .agents\skills`로 만든다.
- 채팅에서 `/<스킬 이름>`으로 부른다.

| 스킬 | 하는 일 |
|---|---|
| `build-doctor` | WxEditor Development 빌드를 돌리고 환경 오류와 첫 컴파일 오류를 구분해 진단한다 |
| `run-editor` | 에디터·게임을 끄고 WxEditor를 빌드한 뒤 에디터를 다시 연다 |
| `generate-project-files` | `BatchFiles/GenerateProjectFiles.bat`으로 `Wx.sln`을 다시 만든다 |
| `pull` | 원격 최신을 가져오고 충돌은 양쪽 변경을 살려 병합한다 |
| `push` | 변경을 커밋하고 푸시한다 |
| `comment-cleanup` | 지정한 범위의 C++ 주석을 정리한다(코드 라인은 건드리지 않음) |
| `discord-report` | 최근 24시간 내 커밋을 요약해 디스코드 ai-room 채널에 보낸다 |
| `wiki-ingest` | 새 자료를 읽어 위키에 요약 문서를 쓰고 색인·엔티티·개념 문서를 함께 고친다 |
| `wiki-query` | 위키에서 관련 문서를 찾아 출처를 달아 답하고, 원하면 답을 개념 문서로 저장한다 |
| `wiki-lint` | 위키의 모순·낡은 주장·고아 문서·빠진 연결을 찾아 보고하고, 고른 것만 고친다 |

### 스크립트

- `.agents/scripts/Export-AbilitySystemLists.ps1`은 에디터 없이 에셋 패키지와 C++을 읽어 `Saved/AbilitySystemLists/`에 어빌리티·이펙트·캐릭터 목록을 만든다. 어빌리티·GE·캐릭터 구성 조사는 이 목록부터 본다.
- `.agents/scripts/Send-DiscordReport.ps1`은 디스코드 웹훅으로 메시지를 보낸다. 웹훅 주소는 git이 무시하는 `.agents/discord-webhook.local.json`에 둔다.

### MCP

- `.mcp.json`이 unreal-mcp(`http://127.0.0.1:8000/mcp`)를 등록하고, Codex는 `.codex/config.toml`에 같은 주소를 둔다.
- 서버는 실행 중인 언리얼 에디터가 연다(엔진 플러그인 `ModelContextProtocol`·`AllToolsets`). 에디터가 꺼져 있으면 연결이 거부된다.
- 프로젝트 플러그인 `Plugins/WxToolset`이 WX 에셋 저작 도구를 더한다(AnimMontage·Blueprint·Landscape·MVVM·StateTree·Water).

### 위키와 메모리

- `Wiki/`가 팀이 함께 보는 지식이다. 질문은 `Wiki/index.md`부터 보고, 고칠 때는 `Wiki/AGENTS.md`를 따른다.
- 새 자료는 `/wiki-ingest`로 적재한다. 요약 문서를 쓴 뒤 색인과 관련 엔티티·개념 문서를 함께 고친다.
- 질문은 `/wiki-query`로 한다. 관련 문서를 찾아 출처를 달아 답하고, 저장을 요청받지 않으면 위키를 고치지 않는다.
- 점검은 `/wiki-lint`로 한다. 낡은 자료·고아 문서·깨진 링크 같은 기계 점검은 `.agents/skills/wiki-lint/scripts/Invoke-WikiLint.ps1`이 하고, 모순과 낡은 주장은 문서를 읽어 찾는다. 매주 월요일 루틴이 적재 뒤에 점검해 결과를 로그에 남긴다.
- `Wiki/` 안의 파일을 읽으면 Claude Code가 `Wiki/AGENTS.md`도 함께 읽는다.
- Claude Code의 자동 메모리(`~/.claude/projects/<프로젝트>/memory/`)는 사람마다 따로 쌓이고 저장소에 올라가지 않는다.

### 클라우드 루틴 (claude.ai)

- 켜져 있는 것은 둘이다.
  - 「일일 주석 정리 + 푸시」: 매일 06:00(KST)에 `comment-cleanup` 스킬의 6절(무인 실행)대로 전날 제출된 C++ 주석을 정리해 `main`에 푸시한다. 커밋 메시지는 `YYYY-MM-DD 제출 코드의 주석을 정리`다.
  - 「Wiki 주간 적재·점검 + 푸시」: 매주 월요일 07:00(KST)에 `wiki-ingest` 스킬의 8절(무인 실행)대로 새 자료와 바뀐 자료를 적재하고, 점검 결과를 로그에 남긴 뒤 `main`에 푸시한다. 커밋 메시지는 `YYYY-MM-DD 위키 주간 적재·점검`이다. 옛 「Wiki 정기 갱신 + 푸시」 루틴을 고쳐 다시 켠 것이다.
- 꺼져 있는 것은 「주간 모듈 코드 리뷰 + 푸시」·「README 정기 갱신」·「스테일 BP 스냅샷 정리」다.

## 결정

- 2026-08-08 에셋을 JSON으로 덤프해 grep하던 방식을 관리 부담 때문에 폐기했다. 에셋 내부 값은 실행 중인 에디터의 unreal-mcp로 하나씩 조회한다. (사용자 결정)
- 2026-10-01 옛 LLM Wiki(claude-obsidian vault)와 AI 워크플로우(작업 절차·작업 기록·대시보드)를 복잡도가 통제를 벗어나 모두 제거했다. (커밋 1ddf0ae3d, 사용자 결정)
- 2026-10-01 위키를 Karpathy LLM Wiki 패턴으로 다시 만들었다(요약·엔티티·개념). 위키는 사용자가 요청할 때만 고친다. (사용자 결정)
- 2026-10-01 위키 적재·질문·점검은 `/wiki-ingest`·`/wiki-query`·`/wiki-lint` 스킬로 한다. 적재는 요약 문서를 쓴 뒤 색인·엔티티·개념 문서를 함께 고치고, 질문은 관련 문서를 찾아 출처를 달아 답하며, 점검은 모순·낡은 주장·고아 문서·빠진 연결을 찾는다. (사용자 결정)
- 2026-10-01 위키 적재·점검을 매주 월요일 07:00(KST) 클라우드 루틴으로 돌리고, 결과는 main에 바로 올린다. 무인 점검은 보고만 하고 고치지 않는다. 자료가 주말 회의 무렵에 몰리고 코드가 주 단위로 크게 바뀌어 주 1회로 정했다. (사용자 결정)

## 미결

- 기획서의 스킬 목록이 지금 저장소와 다르다. 기획서의 `ability-guide`·`stat-guide`는 없고, 예정이던 `code-reviewer`·`code-commenter`도 없으며, 지금 있는 `comment-cleanup`·`discord-report`·`generate-project-files`·`run-editor`는 기획서에 없다.
- 꺼진 루틴 3개는 지우지 않고 남아 있다.
- 클라우드 환경에 PowerShell 7(`pwsh`)이 있는지, 받아서 쓸 수 있는지 확인하지 않았다. 안 되면 주간 루틴은 점검 스크립트 대신 git·grep으로 점검한다. 첫 실행(10-05)에서 확인해야 한다.

## 관련

- [개발 진행과 작업 규칙](개발-진행과-작업-규칙.md)
- [기획 작업 도구](기획-작업-도구.md)

## 출처

- [ai_game_dev](../요약/ai_game_dev.md)
- [LLM_lecture](../요약/LLM_lecture.md)
- [2026-07-19_meeting](../요약/2026-07-19_meeting.md)
- [2026-07-31-회의-안건](../요약/2026-07-31-회의-안건.md)
- `AGENTS.md` (9529b18ca)
- `.agents/skills/` (dbcd7fdb9)
- `.agents/scripts/` (c90ccf76b)
- `.gitignore` (c90ccf76b)
- `.mcp.json` (06c0610d2)
- `.codex/config.toml` (b48c1930a)
- `.gemini/settings.json` (66e9f28a1)
- `Wx.uproject` (d37e1dd32)
- `Plugins/WxToolset/` (d37e1dd32)
- claude.ai 루틴 목록 (2026-10-01 조회)
