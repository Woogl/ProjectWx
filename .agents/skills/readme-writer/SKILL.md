---
name: readme-writer
description: WX Wiki의 모듈 설명을 작성·갱신한다. 기존 README 갱신 요청도 처리하며 모듈 README에는 Wiki 안내 링크만 유지한다.
argument-hint: "[모듈명...|all]"
user-invocable: true
allowed-tools: Read, Grep, Glob, Bash, Write, Agent
---

# Wiki 모듈 설명 갱신

호환성을 위해 readme-writer 이름을 유지한다. 본문 출력은 `.agents/wiki/modules/<Module>.md`이며 기존 모듈 README에는 해당 Wiki 페이지의 상대경로 링크만 둔다.

먼저 저장소의 `AGENTS.md`, `.agents/wiki/AGENTS.md`, `.agents/wiki/maintenance.md`를 읽는다. Wiki 목차와 `sources.json`에서 상태·근거를 확인한다. 원자료·리뷰·사용자의 작업 트리 변경을 보존한다.

## 대상 선택

- `.uproject`와 `Plugins/*/*.uplugin`, `Source/*/*.Build.cs`에서 모듈을 발견한다. 기본은 런타임 모듈이며 에디터 모듈은 명시 요청 시 포함할 수 있다.
- 인자가 없으면 `BatchFiles/CheckWiki.ps1` 결과와 Wiki 등록 목록을 비교해 미작성·needs-review·근거 변경 모듈을 고른다. `all`은 모든 기본 대상, 모듈명 인자는 해당 모듈을 지정한다.
- 페이지의 `baseline_commit` 이후 모듈 및 원자료의 Git diff와 현재 status를 확인한다. SHA를 찾지 못하면 재검토한다. Wiki·안내 README 변경 자체는 소스 변경으로 세지 않는다.
- WxCore·Config·기획 등 모듈 밖 참조 변경도 확인한다. 점검 스크립트는 미등록 의존의 의미적 영향을 발견하지 못한다. 관련 주장을 읽어 의존 목록을 보완한다.
- Wiki가 없는 모듈의 수기 README는 이관 입력으로 보존하며 먼저 읽는다. 본문 검증과 링크 전환이 끝나기 전에 원문을 제거하지 않는다.

## 작성

모듈의 descriptor·Build.cs·공개 헤더와 주요 cpp를 읽어 책임, 경계, 진입점, 확장 절차, 권한·수명 규칙을 정리한다. 타입 전수 목록이나 헤더 주석 복사를 만들지 않는다. 소스 경로는 페이지 기준 실제 상대경로 링크로 쓴다.

기존 구조를 존중하되 새 페이지는 다음을 포함한다.

- 제목, 상태, 검증 범위, 확인일과 기준 커밋.
- 책임과 경계, 핵심 진입점, 확장 포인트, 읽기 순서, 관련 모듈·시스템 링크.
- 원자료 근거와 미검증 범위. BP/WBP·DataTable 내부를 보지 않았다면 명시한다.

전체를 다시 읽지 않은 단순 이관은 `needs-review`를 유지하고 과거 provenance를 보존한다. 읽은 범위만 `current`로 표시하며 추론·개선 제안·의도와 실제 구현을 구분한다. 코딩 규칙은 현재 프로젝트 지침에서 확인하고 과거 문서의 규칙을 현행 지침으로 단정하지 않는다.

## 근거와 마무리

`sources.json`에 읽은 근거와 보수적인 watch_roots를 기록한다. 모듈 내부 소스·descriptor, 사용한 공용 정의·설정·기획을 포함한다. 공용 해시를 바꿀 때에는 다른 소비 페이지를 재검토하거나 needs-review로 표시한다. 수정 중인 파일은 HEAD만으로 재현되지 않으므로 작업 트리 관찰과 해시를 기록한다.

목차·로그와 기존 README의 안내 링크를 맞추고 점검 스크립트를 실행한다. 점검 결과를 없애려고 미확인 원자료의 해시만 바꾸지 않는다. 생성/갱신 페이지, 남은 재검토, 에셋 등 검증 한계를 한국어로 보고한다. 커밋·푸시는 이 스킬에 포함하지 않는다.
