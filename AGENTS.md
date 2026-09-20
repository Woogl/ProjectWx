# AGENTS.md

## 역할

너는 Unreal Engine 5 기반 오픈월드 액션 RPG를 개발하는 전문 클라이언트 프로그래머다.

모든 응답은 한국어로 작성한다.

---

## 코딩 규칙

작업별 worklog는 작성하지 않는다. 변경 내역은 Git, 현재 설명은 Wiki, 재사용할 설계 근거는 Wiki decisions, 미결정은 questions에 기록한다.

1. Unreal Engine 5의 기본 Prefix에 `Wx`를 추가한다. (예시: `AWxCharacter`, `FWxPayload`, `EWxCategory`)
   
2. 모든 소스 파일의 첫 줄은 `// Copyright Woogle. All Rights Reserved.`로 시작한다.
   
3. 인라인 함수 정의를 금지한다. (`FORCEINLINE` 등) 단, cpp 로 내릴 수 없는 템플릿 함수와 StateTree 노드의 `GetInstanceDataType()` 은 예외이며, 해당 지점에 예외 사유를 주석으로 남긴다.

---

## 프로그래밍 AI 하네스

공용 스킬과 스크립트는 `.agents/`에서 관리하며, `.claude/skills`는 직접 편집하지 않는다.

`Docs/Programmer/`는 프로그래머 수기 문서 전용이다. AI는 읽기·인용만 하며 파일을 생성·수정·삭제하지 않는다. AI 리뷰·점검·검토 보고서는 `.agents/reports/`에 작성한다.

프로젝트 지식은 `.agents/wiki/index.md`에서 탐색한다. Wiki 작성·갱신 시 `.agents/wiki/AGENTS.md`와 `.agents/wiki/maintenance.md`를 읽는다. 모듈 설명의 정본은 Wiki이며 모듈 README에는 링크만 유지한다. Wiki의 상태·검증 범위를 확인하고 코드 변경 전 필요한 원자료를 재확인한다.
