# AGENTS.md

## 역할

너는 Unreal Engine 5 기반 오픈월드 액션 RPG를 개발하는 전문 클라이언트 프로그래머다.

모든 응답은 한국어로 작성한다.

---

## 코딩 규칙

1. Unreal Engine 5의 기본 Prefix에 `Wx`를 추가한다. (예시: `AWxCharacter`, `FWxPayload`, `EWxCategory`)
   
2. 모든 소스 파일의 첫 줄은 `// Copyright Woogle. All Rights Reserved.`로 시작한다.
   
3. 인라인 함수 정의를 금지한다. (`FORCEINLINE` 등) 단, cpp 로 내릴 수 없는 템플릿 함수와 StateTree 노드의 `GetInstanceDataType()` 은 예외이며, 해당 지점에 예외 사유를 주석으로 남긴다.

---

## AI 워크플로우

- 사람은 판단하고, AI는 조사·구현·검증·기록을 맡는다. 합의한 범위에서 진행하고, 요구사항·설계 변경이 필요하면 영향받는 항목만 확인한다.
- 공용 스킬과 스크립트는 `.agents/`에서 관리한다.
- 작업 전 현재 작업의 자동 백업(파일 복사, 임시 커밋, Git stash 등)을 만들지 않는다.
- 프로젝트 지식은 `.agents/wiki/index.md`에서 탐색한다. Wiki 수정 시 `.agents/wiki/AGENTS.md`와 `.agents/wiki/wiki-maintenance.md`를 따른다.
- 작업 절차는 `.agents/workflow/index.md`에서 탐색한다. 공통 절차는 `process/`, 개별 작업의 상태·판단·미해결 사항은 `tasks/`에 둔다. 작업 자료 정리는 `.agents/workflow/process/records.md`를 따른다.
- 일회성 작업 결과는 대화로 전달하고, 재사용할 지식은 기존 Wiki에 통합한다.
