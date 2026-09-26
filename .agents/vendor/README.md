# Vendored code

외부 도구를 원본 그대로 넣어 둔 곳입니다. 저장소 코딩 규칙(첫 줄 저작권 표기 등)을 적용하지 않고 파일을 고치지 않습니다.

- `claude-obsidian/`: [claude-obsidian](https://github.com/AgriciDaniel/claude-obsidian) v2.2.0(커밋 `32ac5a0`, MIT)에서 실행에 필요한 부분만 옮겼습니다(`claude_obsidian/`·`scripts/`·`skills/`·`templates/`·`config/`·`agents/`·`docs/`·`.claude-plugin/plugin.json`과 LICENSE·ATTRIBUTION.md·WIKI.md·README.md). Wiki 정기 갱신 Routine이 이 사본을 씁니다. 클라우드 세션은 실행 중에 외부에서 받은 코드를 읽거나 실행하는 것을 거부합니다(2026-09-26 첫 실행).
- 버전 올리기: 사람이 새 태그를 받아 위 목록의 파일로 이 폴더를 통째로 바꾸고, `Wiki/README.md`의 버전 줄을 고쳐 함께 커밋합니다.
