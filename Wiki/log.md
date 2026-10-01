# Wiki Log

위키 작업 기록이다.
새 항목은 맨 아래에 덧붙인다.

## [2026-10-01] init | 위키 생성
- `AGENTS.md`·`index.md`·`log.md`를 만들었다.

## [2026-10-01] ingest | Docs/Meeting/ai_game_dev.md, Docs/Meeting/LLM_lecture.md 외 저장소 설정
- 새 페이지 `Claude-활용.md`를 만들었다(원본 전체는 페이지의 출처 절).
- `index.md`에 개발 환경 분류를 만들었다.

## [2026-10-01] ingest | Docs/ 기획·회의 문서 일괄 적재
- 새 페이지 17개: `게임-개요와-전투-방향.md`, `세계관과-시나리오.md`, `어빌리티-규칙.md`, `플레이어-캐릭터.md`, `현광.md`, `스탯과-피해-계산.md`, `피격-경직.md`, `그로기와-처형.md`, `아이템과-회복.md`, `적-몬스터.md`, `보스.md`, `네임플레이트.md`, `초반-구간과-퀘스트.md`, `장치와-배치물.md`, `체크포인트와-리스폰.md`, `개발-진행과-작업-규칙.md`, `기획-작업-도구.md`
- 고친 페이지: `Claude-활용.md`(팀 AI 규칙 추가), `index.md`(분류 5개로 재편)
- 적재한 원본은 각 페이지의 출처 절에 있다. 구현 절의 코드 사실은 HEAD 47abf38a7 기준으로 확인했다.
- 적재하지 않은 원본:
  - 강의·학습 자료라 프로젝트 결정이 아니다:
    - `Docs/Meeting/LLM 기본 개념 (standalone).html`
    - `Docs/Meeting/PCG_강의노트.html`
    - `Docs/Meeting/World Partition 핵심 개념 (standalone).html`
    - `Docs/Meeting/World Partition 핵심 개념 대본.pdf`
    - `Docs/Meeting/게임 프로그래밍 언어 (오프라인).html`
    - `Docs/Meeting/언리얼_게임_프로그래밍_언어.md`
    - `Docs/Meeting/언리얼 데이터 관리 수단.html`
    - `Docs/Meeting/데이터_관리_수단.md`
    - `Docs/Meeting/GAS공부_이창영.md`
    - `Docs/Meeting/State Machine, Behavior Tree, State Tree 비교 분석.md`
    - `Docs/Meeting/Game_Logic_Control_Paradigms.pptx`
    - `Docs/Meeting/이창영 7.5 BT 학습.pptx`
    - `Docs/Meeting/어크리메이크나침반.pptx`
  - 중복이다: `Docs/SystemDesign/Object_Design.md`(`Docs/LevelDesign/Object_Design.md`와 같은 내용)
  - 읽지 않은 이진 파일이다: `Docs/CombatDesign/WX_main_character.docx`(더 새 md 규격서가 있음), `Docs/LevelDesign/새 동선 맵_1F.svg`(지도 그림)

## [2026-10-01] ingest | 위키를 요약·엔티티·개념 구조로 재편
- 사용자 결정: 적재는 자료를 읽고 요약 문서를 쓴 뒤 색인·엔티티·개념 문서를 함께 갱신하는 것이다. `AGENTS.md`의 구성·문서 형식·적재 절차를 이에 맞춰 바꿨다.
- `/wiki-ingest` 스킬을 만들었다(`.agents/skills/wiki-ingest/SKILL.md`).
- 앞서 적재한 자료 62건의 요약 문서를 `요약/`에 썼다. 각 요약의 '반영한 문서' 절은 역링크로 채웠다.
- 주제 문서 16개를 `개념/`으로, `현광.md`를 `엔티티/`로 옮겼다. `보스.md`는 `엔티티/커스터.md`로 바꾸고, 보스 공통 BT는 `개념/적-몬스터.md`로 합쳤다.
- 개념·엔티티 문서의 출처를 자료 경로에서 요약 문서 링크로 바꿨다.
- `index.md`를 개념·엔티티·요약 세 갈래로 다시 만들었다.

## [2026-10-01] ingest | 사용자 결정: /wiki-query 스킬
- `/wiki-query` 스킬을 만들었다(`.agents/skills/wiki-query/SKILL.md`). `AGENTS.md`의 질문 절이 이 스킬을 가리킨다.
- 고친 문서: `개념/Claude-활용.md`(스킬 표, 위키 절, 결정 절)

## [2026-10-01] lint | /wiki-lint 스킬을 만들고 첫 점검
- `/wiki-lint` 스킬과 기계 점검 스크립트를 만들었다(`.agents/skills/wiki-lint/`). `AGENTS.md`의 점검 절을 스킬 항목에 맞췄고, `/wiki-ingest`의 점검 단계도 이 스크립트를 쓴다.
- 첫 기계 점검(HEAD 47abf38a7): 연결 후보 1건 말고는 0건이다. 연결 후보는 `엔티티/현광.md`의 '피격 경직 중' 표현이라 고치지 않았다.
- 고친 문서: `개념/Claude-활용.md`(스킬 표, 위키 절, 결정 절)

## [2026-10-01] ingest | 사용자 결정: 주간 정기 실행
- `AGENTS.md`의 원칙을 '사용자 요청과 주간 정기 실행에서만 고친다'로 바꿨다.
- 클라우드 Routine 「Wiki 주간 적재·점검 + 푸시」(trig_01Kt2pQAqqAtJQRj5X9Rqrrt)를 매주 월요일 07:00(KST)로 켰다. 옛 「Wiki 정기 갱신 + 푸시」를 고친 것이고, 첫 실행은 10-05다.
- `wiki-ingest` 8절과 `wiki-lint` 6절에 무인 실행 절차를 넣었고, 점검 스크립트를 Linux의 `pwsh`에서도 돌게 고쳤다.
- 고친 문서: `개념/Claude-활용.md`(클라우드 루틴 절, 결정 절, 미결 절)

## [2026-10-02] lint | 푸시 뒤 점검
- 위키·스킬 커밋(9529b18ca, dbcd7fdb9) 때문에 `개념/Claude-활용.md`의 코드 출처 2건(`AGENTS.md`, `.agents/skills/`)이 낡은 것으로 잡혔다. 내용은 이미 반영돼 있어 커밋만 갱신했다.
- 연결 후보 1건(`엔티티/현광.md`의 '피격 경직 중')은 그대로 둔다.

## [2026-10-02] ingest | 사용자 결정: 폴더 이름을 영어로
- Karpathy LLM Wiki 원문의 색인 분류(entities, concepts, sources)를 따라 `요약/` → `sources/`, `엔티티/` → `entities/`, `개념/` → `concepts/`로 바꿨다. 문서 파일 이름은 그대로다.
- 모든 링크, `AGENTS.md`의 구성·양식, `wiki-ingest`·`wiki-query`·`wiki-lint`의 경로를 함께 고쳤다. 이 항목 위의 지난 기록은 옛 폴더 이름 그대로 둔다.

## [2026-10-02] lint | 주간 루틴 적재·점검 (무인)
- 적재: 미적재 자료 0건, 낡은 자료 0건이라 건너뛰었다(HEAD 237056575).
- 점검 환경: 클라우드에 `pwsh`가 없어 PowerShell 7.4.6 linux-x64 tar.gz를 `/tmp`에 받아 돌렸다.
  - 새 클론이 얕은 클론(shallow)이라 처음 실행에서 요약 커밋 대부분이 `bad revision`으로 나고 낡은 자료가 0건으로 잘못 나왔다. `git fetch --unshallow origin main` 뒤 다시 돌렸다.
  - git 기본값 `core.quotepath=true` 때문에 한글 경로가 8진수로 이스케이프돼 미적재 자료가 57건으로 잘못 나왔다. 로컬 설정 `core.quotepath false` 뒤 다시 돌렸다.
- 기계 점검(위 두 조치 뒤): 낡은 자료 0, 사라진 자료 0, 낡은 코드 출처 1, 없는 코드 이름 0, 깨진 링크 0, 고아 문서 0, 출처·반영 짝 0, 연결 후보 1, 색인 누락 0, 미적재 자료 0.
  - 낡은 코드 출처: [Claude 활용](concepts/Claude-활용.md)의 `.agents/skills/` (dbcd7fdb9 뒤 237056575). 폴더 이름 영어화로 경로 문자열만 바뀌었고, 문서 본문은 폴더 이름을 인용하지 않아 내용은 맞다. 커밋만 갱신하면 된다.
  - 연결 후보: [현광](entities/현광.md)의 '피격 경직 중'은 지난 점검대로 그대로 둔다.
- 의미 점검([Claude 활용](concepts/Claude-활용.md)과 링크로 이어진 [개발 진행과 작업 규칙](concepts/개발-진행과-작업-규칙.md)·[기획 작업 도구](concepts/기획-작업-도구.md)):
  - 모순 없음. 기획자 AI 활용 규칙(07-31)은 두 문서가 같다.
  - 풀린 미결: [Claude 활용](concepts/Claude-활용.md)의 '`pwsh`가 있는지 확인하지 않았다' 항목은 이번 실행에서 풀렸다(없지만 받아서 쓸 수 있다).
  - 빠진 결정: [Claude 활용](concepts/Claude-활용.md) 결정 절에 10-02 폴더 이름 영어화(sources·entities·concepts) 사용자 결정이 없다.
  - 제안: `wiki-ingest` 8절 점검 스크립트 항목에 `git fetch --unshallow`와 `git config core.quotepath false`를 넣거나, 스크립트가 git을 `-c core.quotepath=false`로 부르게 고친다.
- 무인 실행이라 아무 문서도 고치지 않았다.
