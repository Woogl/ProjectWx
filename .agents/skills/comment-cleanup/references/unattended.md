# 일일 Routine 실행

클라우드 Routine 「일일 주석 정리 + 푸시」가 매일 새 클론에서 사람 없이 이 문서대로 실행한다. 실행 시각의 정본은 claude.ai의 루틴 설정이다. [SKILL.md](../SKILL.md)의 1~5절과 다른 점만 적는다.

아래 대상 기간의 06:00은 루틴 실행 시각(KST)과 같아야 한다. 루틴 시각을 바꾸면 이 기간도 함께 바꾼다.

**대상** — KST 어제 06:00 ~ 오늘 06:00에 main에 들어온 커밋이 바꾼 `.h`/`.cpp`다. 1절의 인자 처리 대신 아래로 정한다.

```bash
SINCE=$(TZ=Asia/Seoul date -d 'yesterday 06:00' --iso-8601=seconds)
UNTIL=$(TZ=Asia/Seoul date -d 'today 06:00' --iso-8601=seconds)
# 기간 안의 커밋이 얕은 클론의 경계가 되면 부모가 없어 저장소 전체가 바뀐 것으로 나오므로, 기간보다 7일 앞까지 받는다.
if [ "$(git rev-parse --is-shallow-repository)" = true ]; then
  git fetch -q --shallow-since="$(TZ=Asia/Seoul date -d 'yesterday 06:00 7 days ago' --iso-8601=seconds)" origin main
fi
BASE=$(git rev-list -1 --first-parent --before="$SINCE" origin/main)
TIP=$(git rev-list -1 --first-parent --before="$UNTIL" origin/main)
git log --no-merges --invert-grep --grep='제출 코드의 주석을 정리' \
  --name-only --diff-filter=d --pretty=format: "$BASE..$TIP" -- '*.h' '*.cpp' | sort -u
```

- `BASE`가 비면 기간 앞 7일에 커밋이 없다는 뜻이다. `git fetch -q --deepen=200 origin main` 뒤 다시 구한다.
- `--invert-grep`은 전날 이 Routine이 올린 정리 커밋을 뺀다.
- 결과에서 워킹트리에 없는 파일과 1절의 항상 제외 대상을 뺀다.
- 대상이 0개면 "정리할 제출분 없음"으로 보고하고 커밋 없이 끝낸다.

**바뀌는 규칙**

- 대상이 30개를 넘어도 알리거나 되묻지 않고 진행한다. 새 클론에서도 파일을 고치기 전에 SKILL.md 1절대로 작업 전 사본을 만든다. 각 차수 검증이 끝날 때까지 그 사본을 유지한다.
- 지연 대응의 PowerShell 명령은 bash로 바꾼다(예: `ls -lt --time-style=+%H:%M:%S <묶음 파일들> | head -5`).
- 언리얼 엔진이 없으니 빌드·에디터 실행은 시도하지 않는다.
- 2절 "줄인다"의 재사용할 결론은 파일로 남기지 않고 마무리 보고에 적는다.

**커밋·푸시** — 세션이 언제 끊길지 모르므로 마지막에 몰아서 올리지 않는다. SKILL.md 4절의 검증을 마친 서브에이전트 한 차수가 끝날 때마다 바로 main에 올린다.

1. 그 차수 파일에 SKILL.md 4절 검증을 돌린다. Linux에서는 `python3`를 사용한다. 검증기나 Python을 실행할 수 없거나 실패하면 커밋·푸시하지 않고 원인을 보고한다.
2. 바뀐 파일을 경로로 지정해 `git add` 한다(`-A`·`.` 금지). `git status`로 의도한 파일만 올라갔는지 본다.
3. 커밋 메시지는 `$(TZ=Asia/Seoul date -d yesterday +%F) 제출 코드의 주석을 정리`이고, 차수가 여럿이면 뒤에 ` (2/3)`처럼 붙인다. 다음 날 대상 결정이 이 문구로 이 커밋을 거르므로 정확히 지킨다.
4. `git push origin HEAD:main` 한다. 거절되면 `git pull --rebase origin main` 뒤 한 번 다시 시도한다. 충돌이 생기면 멈추고 보고한다. 재시도 전에 재배치된 커밋이 여전히 주석만 바꾸는지 새 부모 파일을 기준 사본으로 검증하며, 검증 실패 시 푸시하지 않는다.

바뀐 파일이 없는 차수는 커밋하지 않는다.

**보고** — 5절에 대상 기간과 커밋 해시·푸시 결과를 더한다. 실패하거나 건너뛴 단계는 숨기지 않는다.
