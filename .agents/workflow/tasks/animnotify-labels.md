# AnimNotify 표시 이름 정리

- 상태: 구현·검증 완료
- 사용자 합의: 2026-09-25, `종류: 대표 값 하나`의 짧은 라벨로 17종 수정 요청.
- 범위: WxCombat 15종, WxAI ReportNoise, WxInventory UseItem. 표시 함수 및 신규 override 선언만 변경.
- 규칙: Row·에셋 이름 보존, 클래스명 끝 `_C` 제거, 미설정 `None`, 스냅 비활성 `Off`. 수치 없는 고정 표식은 Recovery/Combo Window/Use Item.
- 검증: diff 공백 검사, 17종 표시 함수 외 실행 코드 불변 비교, Wiki lint 및 링크 검사 통과. 에디터 타임라인 시각 검증 미실시.
- Editor Development 재검증 성공: [빌드 로그](../../../Saved/Logs/BuildDoctor/build_2026-09-25_034044_655_34628.log), `Result: Succeeded`, 종료 코드 0. 최초 실행에서는 별도 작업 중인 AI 헤더와 UHT 생성 매크로 행 번호 불일치로 실패했으며 Notify 17종은 컴파일됐다. 다른 세션 빌드 종료 후 재실행에서 최신 상태를 확인했다.
- 재실행: `& 'C:/Wx/.agents/skills/build-doctor/scripts/Invoke-WxEditorBuild.ps1' -ProjectRoot 'C:/Wx'`.
- 제출 범위: 본 세션 Notify 코드 24개와 관련 문서 7개만 단일 커밋. 사용자가 본 세션 작업물만 제출하도록 명시했으며 기존 스테이징 항목은 유지한다.
- 기존 어빌리티·에셋 변경은 이번 수정에 포함하지 않는다. 현재 작업 트리 전체 빌드임을 구분한다.
- 재사용 지식: [편집기 표시 규칙](../../../.wiki/wiki/references/editor-tools.md), [구현 근거](../../../.wiki/raw/notes/2026-09-25-animnotify-labels.md).
