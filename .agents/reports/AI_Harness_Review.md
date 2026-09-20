# AI 하네스 점검

> 후속 상태(2026-09-20): 사용자 요청으로 아래 발견 5건을 수정했다. 아래 본문은 수정 전 점검 기록이다. 검증 결과와 변경 내용은 문서 끝의 후속 조치를 참조한다.

검토일: 2026-09-20 · 대상: 현재 작업 트리의 프로젝트 지침, 스킬 9개, 설정, 보조 스크립트, Wiki 점검

## 판단

스킬 정본과 junction 연결은 정상이다. 다만 프로세스 종료 범위, 실행 셸 호환성, 낡은 리뷰 규칙에 수정할 문제가 있다. 아래는 점검 결과이며 설정·스킬·게임 코드는 수정하지 않았다.

## 발견 사항

### P1 — run-editor가 다른 체크아웃의 프로세스를 종료할 수 있다

- 위치: [.agents/skills/run-editor/SKILL.md](../skills/run-editor/SKILL.md) 53~58행.
- 에디터 선택 조건이 프로젝트 절대경로 일치 외에 `*Wx.uproject*` 일치도 허용한다. `D:\OtherCheckout\Wx.uproject`를 실행한 에디터도 `C:\Wx` 작업의 종료 대상에 포함된다.
- 가상 명령줄에 실제 조건식을 적용했을 때 True를 확인했다. 실제 프로세스는 종료하지 않았다.
- 패키지 게임은 실행 파일 이름만 확인하므로 같은 이름의 다른 빌드까지 포함할 수 있다. 뒤에서 `Stop-Process -Force`를 실행하므로 저장하지 않은 작업 손실 가능성이 있다.
- 권고: 에디터는 파싱·정규화한 프로젝트 경로, 패키지 게임은 실행 파일의 실제 경로로 소유 프로젝트를 확인한다. 모호한 대상은 자동 종료하지 않는다.

### P2 — Build Doctor에 PowerShell 7 전제가 명시되어 있지 않다

- 위치: [Invoke-WxEditorBuild.ps1](../skills/build-doctor/scripts/Invoke-WxEditorBuild.ps1) 119·150행, [스킬 실행 지침](../skills/build-doctor/SKILL.md).
- 스크립트의 `Set-Content`/`Add-Content -Encoding utf8NoBOM`은 Windows PowerShell 5.1에서 실패한다. 스킬에는 셸 최소 버전이나 `pwsh` 명시 실행이 없다.
- 설치된 Windows PowerShell 5.1.26100.9444에서 같은 옵션의 매개변수 바인딩 오류를 재현했다. 빌드 자체는 실행하지 않았다.
- 권고: PowerShell 7을 명시적으로 요구하고 실행 명령도 통일하거나, 인코딩 처리를 5.1 호환 API로 바꾼다. 현재 Codex의 PowerShell 7 실행에서는 이 문제를 만나지 않을 수 있다.

### P2 — module-review의 규칙 예시가 현행 AGENTS.md와 다르다

- 위치: [module-review/SKILL.md](../skills/module-review/SKILL.md) 88행.
- WxCore 외 참조, 델리게이트 Handle 접두사, Super 호출, BlueprintCallable 사용, 람다 등에 관한 예시를 프로젝트 규칙 위반으로 제시하지만 현재 AGENTS.md에는 해당 규칙이 없다.
- 같은 스킬 138행에서는 현행 AGENTS.md가 권위라고 하므로 지침 내부에도 모순이 생긴다. 리뷰가 삭제된 정책을 다시 강제할 위험이 있다.
- 권고: 현행 규칙만 위반 근거로 인정하도록 예시를 정리한다. 구조 개선 의견은 규칙 위반과 구분한다.

### P2 — discord-report가 저장소 위치를 C:\Wx로 고정한다

- 위치: [discord-report/SKILL.md](../skills/discord-report/SKILL.md) 51·59행.
- 다른 경로의 클론·worktree에서 스킬을 실행하면 그 체크아웃의 스크립트가 아닌 C:\Wx의 스크립트와 웹훅 설정을 사용한다. 해당 경로가 없으면 실행되지 않는다.
- 전송 스크립트 자체는 PSScriptRoot 기준으로 설정을 읽으므로 문제는 스킬의 절대경로 지시다.
- 권고: 읽은 SKILL.md 위치 또는 현재 저장소 루트에서 스크립트 경로를 계산한다. 외부 전송은 이번 점검에서 수행하지 않았다.

### P3 — 폐지한 worklog 게이트 스크립트가 다시 남아 있다

- 위치: `.agents/scripts/check-worklog.py`(수정 전 경로, 현재 삭제됨) 171행, [.gitignore](../../.gitignore) 78행.
- 스크립트는 존재하지 않는 `.agents/worklog`의 오늘 기록을 확인하도록 남아 있다. gitignore 주석도 worklog를 정본 구성요소로 설명한다.
- 프로젝트 settings.json은 `{}`이며 로컬 설정에도 활성 worklog 훅은 없다. 따라서 현재 편집을 막는 활성 장애로 보지는 않는다.
- 권고: 사용하지 않는 스크립트와 오래된 설명을 제거한다. 이번 점검에서 재등장 원인은 확인하지 않았다.

## 확인한 정상 항목

- 9개 스킬 모두 SKILL.md, 이름, 설명이 있으며 폴더명과 스킬명이 일치한다. YAML 전체 스키마 검증은 아니다.
- `.claude/skills`는 `C:\Wx\.agents\skills`를 가리키는 junction이다. SetupAgentHarness.ps1을 현재 상태에서 실행하면 기존 연결을 인식하고 종료한다. 신규 생성·잘못된 대상 교체 경로는 실행하지 않았다.
- Codex의 `.agents/skills` 사용은 [공식 스킬 문서](https://learn.chatgpt.com/docs/build-skills)의 저장소 탐색 위치와 일치한다.
- PowerShell 스크립트 4개는 현재 셸의 구문 분석을 통과했다. 이는 모든 셸 버전에서 실행된다는 뜻은 아니다.
- `.mcp.json`과 `.codex/config.toml`의 Unreal MCP 주소는 모두 `http://127.0.0.1:8000/mcp`다. 실제 연결·도구 호출은 검증하지 않았다.
- Discord 웹훅 설정, Claude 로컬 설정, junction은 Git ignore 대상이다. 비밀값은 출력하거나 전송하지 않았다.
- Wiki 점검은 구조·링크 오류 0건, 재검토 알림 15건이다. 모듈 9개 이관 상태, 피니시 미결정, WxDialogue 근거 변경 4개, AGENTS.md 변경 1개를 보고했다. 해시를 갱신해 알림을 없애지는 않았다.

## Claude 지침 로딩은 조건부 확인

설치 버전은 Claude Code 2.1.278이며 저장소에는 CLAUDE.md가 없다. 최신 [Claude 공식 문서](https://code.claude.com/docs/en/memory)에 따르면 지원 조건을 만족하는 기본 설정에서는 AGENTS.md를 직접 읽는다. 따라서 CLAUDE.md 부재 자체를 결함으로 판정하지 않는다.

다만 구버전, 기능 플래그 미수신, 일부 제공자·조직 설정에서는 자동 지원되지 않는다. 이번 점검에서는 새 Claude 세션의 실제 지침 로딩까지 확인하지 않았다. 여러 환경의 호환성이 필요하면 `@AGENTS.md`를 담은 작은 CLAUDE.md를 두는 방법을 검토할 수 있다.

## 검증 한계

실제 pull/push·stash·빌드·에디터 종료·Discord 전송·MCP 호출은 실행하지 않았다. Git 스킬은 정적 검토만 수행했다. 저장소가 다른 작업으로 바뀌고 있으므로 이 결과는 점검 시점의 상태다.

우선 수정 순서는 run-editor 종료 범위, 빌드 셸 계약, 리뷰 규칙 동기화다. 스킬 개수나 도구를 늘리기보다 현재 절차의 경계와 실행 조건을 명확히 하는 것이 먼저다.

## 후속 조치 — 5건 수정 완료

- run-editor: 프로세스 선택을 `Get-WxProjectProcess.ps1`로 분리했다. 에디터는 단일 절대 프로젝트 인자를 정규화해 비교하고, 게임은 해당 프로젝트 Binaries/Win64의 실행 경로로 확인한다. 상대경로·복수 프로젝트 인자·정보 누락은 제외한다.
- Build Doctor: 로그 전체를 .NET UTF-8 API로 기록해 5.1 비호환 옵션과 Tee-Object의 셸별 인코딩 차이를 제거했다. 실행기 소스는 한글 파싱을 위해 UTF-8 BOM으로 저장하고 스킬에 5.1/7 지원을 명시했다.
- module-review: 현행 AGENTS.md의 명시적 조항만 규칙 위반 근거로 사용하도록 수정했다. 삭제된 규칙 예시를 제거했다.
- discord-report: C:\Wx 고정 경로를 제거하고 읽은 스킬 및 현재 저장소 위치에서 전송 스크립트를 찾도록 변경했다. 해당 체크아웃에 설정이 없으면 전송하지 않는다.
- worklog: 사용되지 않는 게이트 스크립트와 gitignore의 오래된 설명을 제거했다.

[TestAgentHarness.ps1](../../BatchFiles/TestAgentHarness.ps1)을 Windows PowerShell 5.1과 PowerShell 7에서 실행했다. 가상 프로세스의 선택·제외, 공백/대소문자/정규화 경로, 가짜 빌드의 성공(0)·실패(7)·사전 실패(2), UTF-8 로그 일관성 검증을 통과했다. 실제 에디터 종료·UE 빌드·Discord 전송은 수행하지 않았다.
