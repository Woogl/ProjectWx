# 주간 Routine 실행

클라우드 Routine 「Wiki 주간 적재·점검 + 푸시」가 매주 새 클론에서 사람 없이 이 문서대로 실행한다(일정의 정본은 claude.ai의 루틴 설정). [SKILL.md](../SKILL.md)의 1~7절과 다른 점만 적는다.

**점검 스크립트** — Linux라 `powershell` 대신 `pwsh -NoProfile -File .agents/skills/wiki-lint/scripts/Invoke-WikiLint.ps1`로 돌린다.
- 새 클론은 얕은 클론이라 먼저 `git fetch --unshallow origin main`으로 전체 이력을 받는다. 얕은 클론이면 스크립트가 멈춘다.
- `pwsh`가 없으면 PowerShell 공식 linux-x64 tar.gz(예: v7.4.6)를 `/tmp`에 받아 풀어 쓴다.
- 그래도 돌릴 수 없으면 `/wiki-lint` 1절 표의 항목을 git·grep으로 직접 점검하고, 그렇게 했다고 로그에 적는다.

**대상** — 점검 스크립트의 '미적재 자료'와 '낡은 자료'다. 낡은 자료는 기존 요약 문서를 고쳐 다시 적재한다. 둘 다 없으면 적재를 건너뛰고 점검으로 넘어간다.

**바뀌는 규칙**
- 1절의 사용자 확인을 건너뛴다.
- 판단이 필요한 모순은 한쪽을 고르지 않고 양쪽을 날짜와 함께 적은 뒤 미결 절에 남긴다.
- 언리얼 엔진이 없으니 에디터·MCP는 쓰지 않는다. 구현 확인은 git으로만 한다.

**점검** — 적재 뒤 [위키 무인 점검 절차](../../wiki-lint/references/unattended.md)대로 점검하고 결과를 로그에 남긴다.

**커밋·푸시**
1. `Wiki/` 아래 바뀐 파일만 경로를 지정해 `git add` 한다(`-A`·`.` 금지). `git status`로 의도한 파일만 올라갔는지 본다.
2. 커밋 메시지는 `YYYY-MM-DD 위키 주간 적재·점검`(KST 날짜)이고, 본문에 적재한 자료 수와 점검 결과를 한 줄씩 적는다.
3. `git push origin HEAD:main` 한다. 거절되면 `git pull --rebase origin main` 뒤 한 번 다시 시도한다.
4. 바뀐 파일이 없으면 커밋하지 않는다.

**보고** — 7절에 적재한 자료 수, 점검 항목별 건수, 커밋 해시와 푸시 결과를 더한다. 실패하거나 건너뛴 단계는 숨기지 않는다.
