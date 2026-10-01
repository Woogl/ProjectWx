# Copyright Woogle. All Rights Reserved.
"""Snapshot working files and verify comment-only edits without changing source files."""

import argparse
import bisect
import hashlib
import json
import re
import sys
from pathlib import Path


def code_signature(data):
    # Keep literal contents opaque, including raw strings containing comment markers.
    source = data.decode("utf-8")
    chars, offsets = [], []
    index = 0
    while index < len(source):
        splice = re.match(r"\\(?:\r\n|\n|\r)", source[index:index + 3])
        if splice:
            index += len(splice.group())
        else:
            chars.append(source[index])
            offsets.append(index)
            index += 1
    offsets.append(len(source))
    logical = "".join(chars)
    parts = [source[:offsets[0]]]
    index = 0
    while index < len(logical):
        start = index
        if logical.startswith("//", index):
            end = re.search(r"[\r\n]", logical[index + 2:])
            index = index + 2 + end.start() if end else len(logical)
            continue
        if logical.startswith("/*", index):
            end = logical.find("*/", index + 2)
            if end < 0:
                raise ValueError("unterminated block comment")
            index = end + 2
            comment = source[offsets[start]:offsets[index]]
            # A comment separates preprocessing tokens: +/**/+ must not become ++.
            previous = logical[start - 1] if start else " "
            following = logical[index] if index < len(logical) else " "
            if not previous.isspace() and not following.isspace():
                parts.append(" ")
            parts.extend(re.findall(r"\r\n|\r|\n", comment))
            continue
        raw = re.match(r'(?:u8|u|U|L)?R"([^\s()\\]{0,16})\(', logical[index:])
        if raw:
            # Raw literal contents are not subject to backslash-newline splicing.
            terminator = ")" + raw.group(1) + '"'
            original_start = offsets[index]
            body_start = offsets[index + raw.end()]
            end = source.find(terminator, body_start)
            if end < 0:
                raise ValueError("unterminated raw string")
            original_end = end + len(terminator)
            index = bisect.bisect_left(offsets, original_end)
            parts.append("\x00" + json.dumps(source[original_start:original_end]) + "\x00")
            parts.append(source[original_end:offsets[index]])
            continue
        if logical[index] in "\"'":
            quote = logical[index]
            index += 1
            while index < len(logical) and logical[index] != quote:
                if logical[index] in "\r\n":
                    raise ValueError("unterminated quoted literal")
                index += 2 if logical[index] == "\\" else 1
            if index >= len(logical):
                raise ValueError("unterminated quoted literal")
            index += 1
            parts.append("\x00" + json.dumps(source[offsets[start]:offsets[index]]) + "\x00")
            continue
        number = re.match(r"(?:[0-9]|\.[0-9])(?:[eEpP][+-]|[A-Za-z0-9_.]|'[A-Za-z0-9_])*", logical[index:])
        identifier = re.match(r"[A-Za-z_][A-Za-z0-9_]*", logical[index:])
        index += len(number.group() if number else identifier.group()) if number or identifier else 1
        parts.append(source[offsets[start]:offsets[index]])

    signature = []
    continued = False
    for line in "".join(parts).splitlines(keepends=True):
        # Empty lines in a continued macro can terminate the directive.
        if line.strip() or continued:
            signature.append(line)
        continued = line.rstrip("\r\n").endswith("\\")
    return signature


def selected_paths(root, files_from):
    paths = json.loads(Path(files_from).read_text(encoding="utf-8-sig"))
    if not isinstance(paths, list) or not paths or not all(isinstance(p, str) for p in paths):
        raise ValueError("files-from must contain a nonempty JSON array of paths")
    result = []
    for name in paths:
        path = (root / name).resolve()
        relative = path.relative_to(root).as_posix()
        if path.suffix.lower() not in (".h", ".cpp") or not path.is_file():
            raise ValueError("expected an existing .h/.cpp file: " + relative)
        if relative not in result:
            result.append(relative)
    return result


def snapshot(root, destination, files_from):
    paths = selected_paths(root, files_from)
    if any(destination == (root / path) or destination in (root / path).parents for path in paths):
        raise ValueError("snapshot directory must not contain the target files")
    destination.mkdir(parents=True, exist_ok=False)
    entries = []
    for index, relative in enumerate(paths):
        data = (root / relative).read_bytes()
        backup = str(index) + ".original"
        (destination / backup).write_bytes(data)
        entries.append({"path": relative, "backup": backup, "sha256": hashlib.sha256(data).hexdigest()})
    manifest = {"version": 1, "root": str(root), "files": entries}
    (destination / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    print("COMMENT_SNAPSHOT=" + str(destination))
    print("COMMENT_SNAPSHOT_FILES=" + str(len(entries)))


def verify(destination, files_from=None):
    manifest = json.loads((destination / "manifest.json").read_text(encoding="utf-8"))
    if manifest["version"] != 1:
        raise ValueError("unsupported snapshot version")
    root = Path(manifest["root"]).resolve()
    selected = set(selected_paths(root, files_from)) if files_from else None
    entries = manifest["files"]
    if selected and not selected.issubset({entry["path"] for entry in entries}):
        raise ValueError("selected file has no original snapshot")
    failures, checked = [], 0
    for entry in entries:
        if selected and entry["path"] not in selected:
            continue
        checked += 1
        try:
            path = (root / entry["path"]).resolve()
            path.relative_to(root)
            backup = (destination / entry["backup"]).resolve()
            backup.relative_to(destination)
            before = backup.read_bytes()
            if hashlib.sha256(before).hexdigest() != entry["sha256"]:
                raise ValueError("snapshot was modified")
            after = path.read_bytes()
            if before != after:
                if before.splitlines()[0:1] != after.splitlines()[0:1]:
                    raise ValueError("first line changed (copyright/encoding)")
                if code_signature(before) != code_signature(after):
                    raise ValueError("code or code-line whitespace changed")
        except (ValueError, OSError, UnicodeError) as error:
            failures.append(entry["path"] + ": " + str(error))
    for failure in failures:
        print("COMMENT_VERIFY_FAILURE=" + failure)
    print("COMMENT_VERIFY_FILES=" + str(checked))
    print("COMMENT_VERIFY_RESULT=" + ("failed" if failures else "success"))
    return 1 if failures else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("snapshot", "verify"))
    parser.add_argument("--snapshot", required=True, type=Path)
    parser.add_argument("--root", type=Path)
    parser.add_argument("--files-from")
    args = parser.parse_args()
    try:
        destination = args.snapshot.resolve()
        if args.mode == "snapshot":
            if args.root is None or args.files_from is None:
                parser.error("snapshot requires --root and --files-from")
            snapshot(args.root.resolve(), destination, args.files_from)
            return 0
        return verify(destination, args.files_from)
    except (ValueError, OSError, KeyError, UnicodeError) as error:
        print("COMMENT_VERIFY_ERROR=" + str(error), file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
