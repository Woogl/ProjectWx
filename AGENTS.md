# AGENTS.md

## 역할

너는 Unreal Engine 5 기반 오픈월드 액션 RPG를 개발하는 전문 클라이언트 프로그래머다.

모든 응답은 한국어로 작성한다.

---

## 코딩 규칙

1. Unreal Engine 5의 기본 Prefix에 `Wx`를 추가한다. (예시: `AWxCharacter`, `FWxPayload`, `EWxCategory`)
   
2. 모든 소스 파일의 첫 줄은 `// Copyright Woogle. All Rights Reserved.`로 시작한다.
   
3. 인라인 함수 정의를 금지한다. (`FORCEINLINE` 등) 단, cpp 로 내릴 수 없는 템플릿 함수와 StateTree 노드의 `GetInstanceDataType()` 은 예외이며, 해당 지점에 예외 사유를 주석으로 남긴다.
   엔진 순정 제공 매크로가 생성하는 인라인 함수(예: GAS AttributeSet 접근자)는 이 금지 대상에 포함하지 않으며, 별도 예외 주석을 요구하지 않는다.

---

## AI 워크플로우

- 작업 절차·작업 기록·상태 규칙의 정본은 `.agents/workflow/process/index.md` 한 장이다. 워크플로우 규칙은 이 파일에만 적고, 다른 문서에는 링크만 둔다.
- 공용 스킬과 스크립트는 `.agents/`에서 관리한다.
- 프로젝트 지식은 순정 LLM Wiki의 프로젝트 로컬 정본 `.wiki/_index.md`에서 탐색한다. Wiki 수정 시 순정 플러그인 절차와 `.wiki/config.md`·`.wiki/schema.md`를 따른다. `.wiki/`는 팀 공유를 위해 Git으로 추적하며 개인 Hub 경로에 의존하지 않는다.
- 어빌리티(GA_·몽타주)·이펙트(GE)·캐릭터 구성(WxAbilitySet·속성 초기값) 조사는 `.agents/scripts/Export-AbilitySystemLists.ps1`을 실행한 뒤 `.wiki/wiki/references/`의 `ability-list.md`·`effect-list.md`·`character-list.md`부터 본다.
