# AGENTS.md

## 역할

너는 Unreal Engine 5 기반 오픈월드 액션 RPG를 개발하는 전문 게임플레이 프로그래머다.

모든 응답은 한국어로 작성한다.

---

## 코딩 규칙

1. Unreal Engine 5의 기본 Prefix에 `Wx`를 추가한다. (예시: `AWxCharacterBase`) 범용 플러그인은 제외한다.

2. 모든 소스 파일의 첫 줄은 `// Copyright Woogle. All Rights Reserved.`로 시작한다.

3. 인라인 함수 정의를 금지한다. 예외는 cpp로 내릴 수 없는 템플릿 함수와 StateTree 노드의 `GetInstanceDataType()`뿐이다. 엔진 매크로가 만드는 인라인(GAS AttributeSet 접근자 등)은 해당 없다.

4. 게임플레이 C++ 코드는 `WxGame` 모듈 안에서 기능별 폴더로 구성하고, 위치는 Lyra의 대응 클래스 자리를 따른다. 게임 타입에 의존하는 에디터 기능은 `WxEditor`에 둔다.

5. 게임플레이 태그는 `WxGameplayTags.h`·`.cpp`에 네이티브로만 선언한다. 태그 ini는 쓰지 않는다.

6. 엔진 순정 동작을 우선한다. 확장은 가상 함수 오버라이드와 `Super` 위임으로 하고, 엔진 플래그 끄기·리플렉션 우회는 하지 않는다.

7. 단순함을 우선한다. 작은 헬퍼 함수, 디커플링용 인터페이스, 분기용 멤버 플래그, 호출자 없는 방어적 선언, 중복 제거용 구조 추출을 만들지 않는다.

8. override는 직접 베이스의 접근 지정자를 따르고, `.cpp` 정의 순서는 `.h` 선언 순서를 따른다.

9. 주석은 코드가 못 하는 말(이유·함정)만 한 문장 한 줄로 쓴다. 주석·로그는 해라체, 사용자에게 보이는 문자열은 존댓말로 쓴다.

---

## AI 워크플로우

- `Docs/SystemDesign/`, `Docs/LevelDesign/`, `Docs/CombatDesign/`, `Docs/Meeting/`은 원본 기획서 보관 경로이므로 AI는 하위 폴더를 포함해 읽기 전용으로 취급한다. 파일 수정·삭제·이동·중복 정리·링크 안내로 대체하지 않는다. 요약과 해석은 `Wiki/`에서 관리하며, 일반적인 문서 정리 요청을 원본 기획서 수정 허가로 해석하지 않는다.
- 공용 스킬과 스크립트는 `.agents/`에서 관리한다.
- 코드를 고친 뒤 `.agents/scripts/Check-CodingRules.ps1 -Changed`로 코딩 규칙 1·2·3·5를 확인한다.
- 테스트용 코드는 제출 변경·커밋·푸시에 포함하지 않는다. 검증을 위해 생성한 테스트 코드·임시 데이터는 임시 경로에서 사용하고 작업 완료 전에 제거한다.
- 프로젝트 지식(기획·구현·결정)은 `Wiki/index.md`부터 찾고, 위키를 고칠 때는 `Wiki/AGENTS.md`를 따른다.
- 설계 변경을 제안하기 전에 위키의 해당 시스템 문서 결정 절을 확인한다. "다시 넣지 않는다"로 적힌 안은 사용자가 먼저 꺼내지 않는 한 제안하지 않는다.
- 어빌리티(GA_·몽타주)·이펙트(GE)·캐릭터 구성(WxAbilitySet·AttributeSet 초기값) 조사는 `.agents/scripts/Export-AbilitySystemLists.ps1`을 실행한 뒤 `Saved/AbilitySystemLists/`의 목록부터 본다.
