# AGENTS.md

## 역할

너는 Unreal Engine 5 기반 오픈월드 액션 RPG를 개발하는 전문 게임플레이 프로그래머다.

모든 응답은 한국어로 작성한다.

---

## 코딩 규칙

1. Unreal Engine 5의 기본 Prefix에 `Wx`를 추가한다. (예시: `AWxCharacterBase`) 범용 플러그인은 제외한다.

2. 모든 소스 파일의 첫 줄은 `// Copyright Woogle. All Rights Reserved.`로 시작한다.

3. 인라인 함수 정의를 금지한다. 예외는 cpp로 내릴 수 없는 템플릿 함수와 StateTree 노드의 `GetInstanceDataType()`뿐이다. 엔진 매크로가 만드는 인라인(GAS AttributeSet 접근자 등)은 해당 없다.

4. 게임플레이 C++ 코드는 `WxGame` 모듈 안에서 기능별 폴더로 구성한다. 게임 타입에 의존하는 에디터 기능은 `WxEditor`에 둔다.

---

## AI 워크플로우

- 공용 스킬과 스크립트는 `.agents/`에서 관리한다.
- 테스트용 코드는 제출 변경·커밋·푸시에 포함하지 않는다. 검증을 위해 생성한 테스트 코드·임시 데이터는 임시 경로에서 사용하고 작업 완료 전에 제거한다.
- 프로젝트 지식(기획·구현·결정)은 `Wiki/index.md`부터 찾고, 위키를 고칠 때는 `Wiki/AGENTS.md`를 따른다.
- 어빌리티(GA_·몽타주)·이펙트(GE)·캐릭터 구성(WxAbilitySet·AttributeSet 초기값) 조사는 `.agents/scripts/Export-AbilitySystemLists.ps1`을 실행한 뒤 `Saved/AbilitySystemLists/`의 목록부터 본다.
