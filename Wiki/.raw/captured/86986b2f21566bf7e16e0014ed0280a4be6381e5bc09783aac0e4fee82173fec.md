# AnimNotify 표시 이름 정리

상태: 완료 · 체크리스트 2/2 통과
다음 행동: 변경 시 기록된 테스트 범위와 제약을 참고한다.

- 사용자 합의: 2026-09-25, `종류: 대표 값 하나`의 짧은 라벨로 17종 수정 요청.
- 범위: WxCombat 15종, WxAI ReportNoise, WxInventory UseItem. 표시 함수 및 신규 override 선언만 변경.
- 규칙: Row·에셋 이름 보존, 클래스명 끝 `_C` 제거, 미설정 `None`, 스냅 비활성 `Off`. 수치 없는 고정 표식은 Recovery/Combo Window/Use Item.
- 검증: diff 공백 검사, 17종 표시 함수 외 실행 코드 불변 비교, Wiki lint 및 링크 검사 통과. 에디터 타임라인 시각 검증 미실시.
- Editor Development 재검증 성공: 빌드 로그 `Saved/Logs/BuildDoctor/build_2026-09-25_034044_655_34628.log`, `Result: Succeeded`, 종료 코드 0. 최초 실행에서는 별도 작업 중인 AI 헤더와 UHT 생성 매크로 행 번호 불일치로 실패했으며 Notify 17종은 컴파일됐다. 다른 세션 빌드 종료 후 재실행에서 최신 상태를 확인했다.
- 재실행: `& 'C:/Wx/.agents/skills/build-doctor/scripts/Invoke-WxEditorBuild.ps1' -ProjectRoot 'C:/Wx'`.
- 제출 범위: 본 세션 Notify 코드 24개와 관련 문서 7개만 단일 커밋. 사용자가 본 세션 작업물만 제출하도록 명시했으며 기존 스테이징 항목은 유지한다.
- 기존 어빌리티·에셋 변경은 이번 수정에 포함하지 않는다. 현재 작업 트리 전체 빌드임을 구분한다.
- 재사용 지식: 편집기 표시 규칙, 구현 근거.


## 테스트 체크리스트

| 항목 | 확인 방법 | 담당 | 결과 | 근거 |
| --- | --- | --- | --- | --- |
| 17종 라벨 값 | 에디터: 몽타주 타임라인에서 노티파이 17종이 `종류: 대표 값` 짧은 라벨로 보이고 값이 설정과 같다 | 사람 | 통과 | 이우성 2026-09-25 |
| 라벨 가독성 | 에디터: 타임라인에서 라벨이 겹치거나 잘리지 않고 읽힌다 | 사람 | 통과 | 이우성 2026-09-25 |

## 사용자 테스트 결과 · 2026-09-25T17:14:48.161Z

<!-- test-feedback:request-44ffe9fd-cc0f-4922-b2b6-2989e4e0a289:submitted -->
- 전달한 사람: 이우성

> 통과 · 17종 라벨 값
> 통과 · 라벨 가독성


## AI 완료 정리 · 2026-09-25T17:14:48.166Z

<!-- test-feedback:request-44ffe9fd-cc0f-4922-b2b6-2989e4e0a289-cleanup:1 -->
- 전달한 사람: 이우성
- 처리 AI: Codex
- 처리 결과: 정리 완료

AI 요약:

> AnimNotify 17종 라벨 값과 가독성의 사람 확인 범위를 Wiki에 반영했습니다. 게임 코드·에셋·작업 기록·접수 JSON·작업 상태는 변경하지 않았습니다.
> 기존 다른 작업의 변경과 관련 문서의 줄바꿈 경고는 그대로 보존했습니다.

> 변경: .wiki/raw/notes/2026-09-26-animnotify-label-acceptance.md에 사람 확인 결과와 검증 범위를 수집했습니다.

> 변경: .wiki/wiki/references/editor-tools.md에 17종 값 일치·가독성 확인과 적용 범위를 반영했습니다.

> 변경: Wiki 루트·원자료 색인과 log.md를 갱신했습니다.

> 근거: AGENTS.md, 작업 절차, animnotify-labels.md, Wiki config.md·schema.md, wiki 스킬과 기존 라벨 원자료·편집기 기사를 읽었습니다.

> 근거: Get-FileHash 결과 작업 기록 SHA-256이 접수 해시 d40b92a81c52df76f669259b5eb2b146776be098471a0f1099b0f7fbfdc3ffe3과 일치했습니다.

> 근거: 순정 llm-wiki lint --local --json 통과: 오류·경고 0건.

> 근거: git diff --check 통과, 관련 문서 로컬 링크 33개 확인.

> 근거: Export-Wiki.ps1 실행으로 Wiki·Workflow 뷰어 각각 64개 문서를 갱신했습니다. 빌드·에디터 테스트는 재실행하지 않았습니다.
