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

4. 게임 컨텐츠는 `WxGame` 모듈에 Lyra `LyraGame`처럼 기능 폴더로 나눠 둔다. 플러그인은 게임 타입을 모르는 범용 도구와, `WxGame`을 의존하는 GameFeature 컨텐츠에만 쓴다.

---

## AI 워크플로우

- 공용 스킬과 스크립트는 `.agents/`에서 관리한다.
- 프로젝트 지식(기획·구현·결정)은 `Wiki/index.md`부터 찾고, 위키를 고칠 때는 `Wiki/AGENTS.md`를 따른다.
- 어빌리티(GA_·몽타주)·이펙트(GE)·캐릭터 구성(WxAbilitySet·AttributeSet 초기값) 조사는 `.agents/scripts/Export-AbilitySystemLists.ps1`을 실행한 뒤 `Saved/AbilitySystemLists/`의 `ability-list.md`·`effect-list.md`·`character-list.md`부터 본다.
