# Copyright Woogle. All Rights Reserved.
# PreToolUse 게이트: 편집하려는 게임 소스가 오늘자 worklog의 수정 범위에 선언돼 있지
# 않으면 사용자 승인을 요청한다. CLAUDE.md '작업 워크플로우' 규칙의 강제 수단.
# 단 주석만 바뀌는 편집은 사소 작업 예외라 게이트를 통과시킨다(/comment-cleanup 서브에이전트).
# stdin으로 PreToolUse 페이로드(JSON)를 받는다.
import sys
import os
import re
import json
import glob
import datetime

# 게이트 대상 확장자 (게임 소스 코드)
SOURCE_EXTS = ("cpp", "h", "hpp", "inl", "cs")


def passthrough():
    # 결정을 내리지 않고 정상 권한 흐름에 맡긴다.
    sys.exit(0)


def decide(decision, reason):
    out = {
        "hookSpecificOutput": {
            "hookEventName": "PreToolUse",
            "permissionDecision": decision,
            "permissionDecisionReason": reason,
        }
    }
    # ensure_ascii=True(기본): 한글을 \uXXXX로 출력해 stdout 인코딩에 비의존.
    # 하네스 JSON 파서가 \uXXXX를 한글로 복원하므로 사용자에게는 정상 표시된다.
    print(json.dumps(out))
    sys.exit(0)


def scan(text, in_block=False, in_line=False):
    # 주석을 걷어낸 코드 잔여물과, 텍스트 끝에서의 주석 상태를 함께 돌려준다.
    # 문자열·문자 리터럴을 추적하므로 TEXT("http://a") 속 //를 주석으로 오인하지 않는다.
    # 시작 상태를 받는 이유는 편집 조각이 주석 한가운데서 시작할 수 있기 때문이다.
    lines = []
    cur = []
    i = 0
    n = len(text)
    in_str = False
    in_char = False
    while i < n:
        c = text[i]
        nxt = text[i + 1] if i + 1 < n else ""
        if c == "\n":
            lines.append("".join(cur).rstrip())
            cur = []
            # 리터럴과 줄 주석은 줄을 넘지 않는다. 블록 주석 상태는 줄을 넘겨 유지한다.
            in_str = False
            in_char = False
            in_line = False
            i += 1
            continue
        if in_line:
            i += 1
            continue
        if in_block:
            if c == "*" and nxt == "/":
                in_block = False
                i += 2
                continue
            i += 1
            continue
        if in_str or in_char:
            cur.append(c)
            if c == "\\" and nxt:
                cur.append(nxt)
                i += 2
                continue
            if in_str and c == '"':
                in_str = False
            elif in_char and c == "'":
                in_char = False
            i += 1
            continue
        if c == "/" and nxt == "/":
            in_line = True
            i += 2
            continue
        if c == "/" and nxt == "*":
            in_block = True
            i += 2
            continue
        if c == '"':
            in_str = True
        elif c == "'":
            in_char = True
        cur.append(c)
        i += 1
    lines.append("".join(cur).rstrip())
    # 주석 줄을 지우면 잔여물이 빈 줄로 남으므로 빈 줄은 비교에서 뺀다.
    return [ln for ln in lines if ln.strip()], in_block, in_line


def edit_start_state(file_path, old_string, replace_all):
    # 편집이 시작되는 지점의 주석 상태를 파일 원본에서 확정해 돌려준다.
    # 조각만 보면 블록 주석 내부인지 알 수 없어, 주석 본문 편집이 코드 변경으로 오인된다.
    # 확정하지 못하면 (False, False)를 돌려 조각만 보던 기존 판정을 그대로 따른다.
    if not old_string:
        return False, False

    try:
        with open(file_path, encoding="utf-8") as f:
            content = f.read()
    except Exception:
        return False, False

    # 줄바꿈 방식이 달라 위치를 놓치는 일이 없도록 양쪽을 맞춘다. 주석 상태와는 무관한 차이다.
    content = content.replace("\r\n", "\n")
    needle = old_string.replace("\r\n", "\n")

    state = None
    at = content.find(needle)
    while at >= 0:
        here = scan(content[:at])[1:]
        if state is None:
            state = here
        elif state != here:
            # replace_all이 상태가 엇갈리는 지점들을 함께 잡으면 판정을 포기한다.
            return False, False
        if not replace_all:
            break
        at = content.find(needle, at + len(needle))

    if state is None:
        return False, False
    return state


def main():
    # stdin은 항상 UTF-8 페이로드. Windows 로케일(cp949)에 영향받지 않도록 직접 디코드.
    try:
        data = json.loads(sys.stdin.buffer.read().decode("utf-8"))
    except Exception:
        passthrough()

    tool_input = data.get("tool_input") or {}
    file_path = tool_input.get("file_path") or ""
    if not file_path:
        passthrough()

    norm = file_path.replace("\\", "/").lower()

    # 게임 소스만 게이트한다: 경로에 /source/ 포함 + C++/빌드 확장자.
    # .claude/, Content/, Snapshots/ 등은 여기서 걸러져 통과한다.
    ext = norm.rsplit(".", 1)[-1] if "." in norm else ""
    is_source = "/source/" in norm and ext in SOURCE_EXTS
    if not is_source:
        passthrough()

    # 주석만 바뀌는 편집은 worklog 없이 통과시킨다.
    # Edit만 가른다 — Write는 편집 전 내용이 페이로드에 없어 비교할 수 없다.
    if (data.get("tool_name") or "") == "Edit":
        old_string = tool_input.get("old_string") or ""
        new_string = tool_input.get("new_string") or ""
        in_block, in_line = edit_start_state(file_path, old_string, bool(tool_input.get("replace_all")))
        if scan(old_string, in_block, in_line)[0] == scan(new_string, in_block, in_line)[0]:
            decide("allow", "주석만 바뀌는 편집이라 worklog 게이트를 통과시킵니다(사소 작업 예외).")

    # 편집 파일의 basename(확장자 제거)을 오늘자 worklog 본문과 매칭한다.
    # 확장자를 떼므로 WxFoo.h / WxFoo.cpp 짝이 한 번에 커버된다.
    base = os.path.basename(norm)                                # 소문자, 매칭용
    stem = base.rsplit(".", 1)[0] if "." in base else base       # 예: wxfoo

    project_dir = os.environ.get("CLAUDE_PROJECT_DIR", os.getcwd())
    today = datetime.date.today().strftime("%Y-%m-%d")
    pattern = os.path.join(project_dir, ".claude", "worklog", today + "-*.md")

    matched = False
    if stem:
        # 토큰 경계: wxfoo 가 wxfoobar 에 오매칭되지 않도록(언더스코어도 단어 문자로 취급).
        token = re.compile(r"(?<![a-z0-9_])" + re.escape(stem) + r"(?![a-z0-9_])")
        for wl in glob.glob(pattern):
            try:
                with open(wl, encoding="utf-8") as f:
                    if token.search(f.read().lower()):
                        matched = True
                        break
            except Exception:
                continue

    if matched:
        passthrough()

    disp = os.path.basename(file_path.replace("\\", "/"))        # 원본 케이스, 표시용
    reason = (
        "오늘자 worklog의 수정 범위에 '" + disp + "'가 없습니다(" + today + "). "
        "비사소 작업이면 거절(deny)하고 worklog의 「수정 범위」에 이 파일을 적으세요"
        "(필요시 .claude/worklog/" + today + "-제목.md 새로 작성). "
        "사소 예외(오타·로그 문구·포매팅 등)면 승인(allow)하세요."
    )
    decide("ask", reason)


main()
