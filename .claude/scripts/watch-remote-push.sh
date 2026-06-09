#!/usr/bin/env bash
# 원격(origin)에 "다른 사람"의 새 커밋이 푸시되면 종료한다.
# Claude Code 백그라운드 태스크로 실행되며, 종료 시 Claude가 다시 깨어나 알림을 띄운다.
# 사용법: watch-remote-push.sh [branch=main] [interval_sec=180]
set -u
cd /c/Wx || exit 1
export GIT_TERMINAL_PROMPT=0

SELF="$(git config user.email)"
BRANCH="${1:-main}"
INTERVAL="${2:-180}"

git fetch -q origin "$BRANCH" 2>/dev/null
old="$(git rev-parse "origin/$BRANCH")"

while true; do
  sleep "$INTERVAL"
  git fetch -q origin "$BRANCH" 2>/dev/null || continue
  new="$(git rev-parse "origin/$BRANCH")"
  [ "$new" = "$old" ] && continue

  # 이번 푸시 범위에 본인(SELF) 외 작성자가 한 명이라도 있으면 알린다.
  others="$(git log "$old..$new" --no-merges --format='%ae' | grep -viF "$SELF" | head -n1)"
  if [ -n "$others" ]; then
    echo "=== REMOTE PUSH DETECTED on origin/$BRANCH ==="
    git log "$old..$new" --no-merges --format='%h | %an | %s'
    exit 0
  fi
  old="$new"
done
