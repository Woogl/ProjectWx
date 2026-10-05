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

## [2026-10-02] lint | 시험 실행 지적 반영
- 점검 스크립트가 git을 `-c core.quotepath=false`로 부르고, 얕은 클론이면 멈추고, 찾을 수 없는 커밋을 '커밋을 찾을 수 없다'로 보고하게 고쳤다(21b1e95bf). `wiki-ingest` 8절에 `git fetch --unshallow` 단계를 넣었다.
- [Claude 활용](concepts/Claude-활용.md): 풀린 미결(`pwsh`)을 구현 절로 옮기고, 10-02 폴더 이름 결정을 결정 절에 넣고, `.agents/skills/` 출처 커밋을 21b1e95bf로 갱신했다.
- 연결 후보 1건은 그대로 둔다.

## [2026-10-02] lint | Claude 활용 문서의 정본 중복 제거
- 사용자 요청에 따라 [Claude 활용](concepts/Claude-활용.md)의 스킬 목록·설정값·도구 로딩 방식·루틴 상태·실행 환경 세부사항을 정본 위치 안내로 대체했다.
- 활용 원칙과 날짜가 있는 결정 이유는 보존하고, 운영 절차의 반복과 현재 상태를 나열한 미결 항목은 제거했다.
- `index.md`의 소개를 문서 범위에 맞췄다.

## [2026-10-02] lint | SSoT 중복 관리 점검
- 범위: 위키 개념·엔티티와 관련 출처, 위키 운영 규칙·스킬, README, 빌드·프로젝트 생성 도구의 운영 정보.
- 주요 발견: 주간 일정과 위키 작업 절차의 중복, 엔진 버전·탐색·빌드 실행 로직의 분산, AI 검토 규칙의 중복, 기획 작업 도구의 수동 목록, Object_Design 원문 사본.
- 자원 규칙은 현광 문서에 이미 모순이 기록되어 있으나, 스탯 문서와 현광 문서가 같은 일반 규칙을 각각 확정 결정으로 적고 있다. 원문에도 궁극기1 MP 3과 과거 궁극기 UP 규칙이 함께 있어 추가 결정 근거가 필요하다.
- 기계 점검: 낡은 자료·사라진 자료·낡은 코드 출처·없는 코드 이름·깨진 링크·고아 문서·출처 짝·색인 누락 각 0, 연결 후보 1(현광의 피격 경직), 미적재 자료 1(Docs/Programmer/CodeReview_2026-10-02.md).
- 점검 기록만 추가했다. 발견 항목의 문서·설정·코드·원문은 수정하지 않았다.

## [2026-10-02] lint | 승인된 SSoT 중복 정리
- 위키 원칙·형식은 `Wiki/AGENTS.md`, 실행 절차는 각 스킬, 일정·활성 상태는 claude.ai 루틴 설정으로 정본을 나눴다.
- 조작키는 플레이어 캐릭터, AI 검토 규칙은 Claude 활용, 자원 공통 규칙은 스탯과 피해 계산 문서로 연결했다. 기획 작업 도구의 BT·노티파이 목록은 구현 위치 안내로 바꿨다.
- `Docs/SystemDesign/Object_Design.md` 사본을 LevelDesign 정본 링크로 대체하고 관련 요약·개념 문서를 갱신했다.
- 09-19 자원 규칙은 과거 전제와 당시 구현을 구분했다. 현재 공통 코스트 규칙을 새로 확정하지 않았다. 피해 계산은 HEAD 코드의 정본과 기획 차이를 연결했다.
- 엔진 버전은 `.uproject`의 `EngineAssociation`, 설치 경로 탐색은 `BatchFiles/Get-WxEngineRoot.ps1`로 통합했다. 빌드 공용 사전 검사·프로젝트 생성·리디렉트 검사에서 이를 사용한다. 동시에 반영된 에디터 실행 공용화 구조를 보존했다.
- 검증: PowerShell 구문 검사, PowerShell 5.1·현재 셸의 실제 설치 탐색, 공용 사전 검사(읽기 전용), 다른 버전·공백 경로·미등록 버전·버전 누락 검증 통과. 실제 빌드·에디터 종료·리디렉트 재저장은 실행하지 않았다.

## [2026-10-05] ingest | Docs/Meeting/어빌리티 규칙 브리핑.md, Docs/Programmer/PCG 개발 방향성.md (무인)
- 다시 적재: `Docs/Meeting/어빌리티 규칙 브리핑.md` (f21dbb0d9). 강제 그룹의 영어 이름이 Override에서 Reaction으로 바뀐 것뿐이라 [요약](sources/어빌리티-규칙-브리핑.md)과 [어빌리티 규칙](concepts/어빌리티-규칙.md)의 그룹 절에 영어 이름을 넣었다.
- 새로 적재: `Docs/Programmer/PCG 개발 방향성.md` (29facaef6). [요약](sources/PCG-개발-방향성.md)을 쓰고 [기획 작업 도구](concepts/기획-작업-도구.md)의 PCG 기획·구현·결정·미결·출처를 고쳤다. 색인에 `Docs/Programmer` 분류를 만들었다.
- 함께 적은 어긋남(미결): 07-19 '프로그래머가 PCG 제공' 대 10-05 '아트가 바이옴 PCG 배치'(역할 분담에 아트가 없다), 09-26 'PCG는 파라미터만 제공하고 마무리' 대 10-05 숲 바이옴 PCG 추가.
- 구현 절은 HEAD 29facaef6에서 git으로만 확인했다(`Content/LevelDesign/PCG/Forest`·`Road`·`Maps`). PCG 그래프 내부는 에디터 없이 확인하지 못했다.

## [2026-10-05] lint | 주간 루틴 점검 (무인)
- 환경: 얕은 클론을 `git fetch --unshallow origin main`으로 풀고, PowerShell 7.4.6 linux-x64를 `/tmp`에 받아 점검 스크립트를 돌렸다.
- 기계 점검(적재 뒤): 낡은 자료 0, 사라진 자료 0, 낡은 코드 출처 24, 없는 코드 이름 0, 깨진 링크 0, 고아 문서 0, 출처·반영 짝 0, 연결 후보 1, 색인 누락 0, 미적재 자료 0. 적재 전에는 낡은 자료 1, 미적재 자료 1이었다.
  - 연결 후보: [현광](entities/현광.md)의 '피격 경직'은 지난 점검대로 그대로 둔다.
  - 낡은 코드 출처 24건은 10-01~10-05 커밋(주석 정리, 콤보 GAS 입력 태스크 전환 4cc8cfb39, 노티파이 분기점 리팩터링 8ed34595d, 피격 취소·넉백 06ede3999, 처형 점유 e76ff6bb0, 패시브 다중 타격 1508c1632, 장치 복원 3c4fb1adc·e0accaab6, 미니언 속성 행 46eb978ce)이다. 주석만 바뀐 출처는 커밋만 갱신하면 된다.
- 의미 점검(낡은 코드 출처가 걸린 문서·이번에 고친 문서와 링크된 문서): 구현 절이 코드와 다르거나 빠진 것.
  - [체크포인트와 리스폰](concepts/체크포인트와-리스폰.md): 구현 절 '체크포인트는 PlayerStart 기반'과 달리 HEAD의 체크포인트는 장치(`BP_CheckPoint`·`ST_CheckPoint`)이고 `WxStateTreeTask_SaveCheckpoint`가 RespawnPoint 위치를 `UWxCheckpointSaveGame`에 저장한다. `PlayerStart`는 Source에 0건이다. 체크포인트 구현이 [장치와 배치물](concepts/장치와-배치물.md)과 이 문서 어디에도 없다.
  - [피격 경직](concepts/피격-경직.md): 04-25 '넉백은 루트모션, 강도 수치 없음'과 달리 코드는 루트모션을 끄고 `KnockbackDistance`(HitReact 300·GuardReact 100)를 ConstantForce로 밀고 넉업은 `KnockupZVelocity` 750이다. HitReact가 `UseItem`도 취소한다(06ede3999). 패턴을 취소하는 것은 그로기·사망뿐이라 스킬 피격도 패턴을 끊지 않는 것으로 보인다(추정, 기획은 평타만 예외).
  - [그로기와 처형](concepts/그로기와-처형.md): 처형이 `State.FinisherReserved`로 한 대상을 한 처형만 점유한다(e76ff6bb0). 미결 '그로기 중 피격 반응'은 코드가 WX 문서 쪽(그로기 중 넉백류를 Normal로 바꿈)이다.
  - [아이템과 회복](concepts/아이템과-회복.md): 에셋 이름은 `GA_UseItem`이 아니라 `GA_Shared_UseItem`이다. 10-02 코드 리뷰의 '회복 전 피격 시 소비 안 함' 결정이 결정 절에 없다. C++ 기본 `MaxCharges`는 3이라 기획(4개)과 다르다(에셋 값 미확인). [피격 경직](concepts/피격-경직.md)과 링크가 없다.
  - [현광](entities/현광.md): 도플갱어 미러링이 마스터의 종료·콤보 단계까지 따른다(4cc8cfb39). 분신은 `SummonedMinion`, 도플갱어는 `Minion` 속성 행을 쓴다(46eb978ce, 이진 에셋이라 추정 포함).
  - [기획 작업 도구](concepts/기획-작업-도구.md): 46eb978ce가 '고치면 안 된다'던 `ABS_Shared_Enemy`에서 속성 행 지정을 뺐다. RandomChoice '균등 확률' 서술은 `WxBTDecorator_RandomWeight` 이후 낡았다.
  - [초반 구간과 퀘스트](concepts/초반-구간과-퀘스트.md): 29facaef6이 PCG·Water 플러그인을 켜고 숲 바이옴 데모 레벨을 넣었으니 구현과 [기획 작업 도구](concepts/기획-작업-도구.md) 연결을 더할 만하다.
  - [적 몬스터](concepts/적-몬스터.md)·[커스터](entities/커스터.md): 큐는 `GC_AttackTelegraph_Red/Blue/Purple`이고 기획의 가드 불가 '노란빛' 큐는 없다.
  - [스탯과 피해 계산](concepts/스탯과-피해-계산.md): 패시브 MP·UP 수급이 1508c1632 뒤 대상 수에 비례한다.
  - [장치와 배치물](concepts/장치와-배치물.md): 에셋 목록에 버튼(`BP_ButtonDevice`·`ST_Button`)이 없다. 복원·레이트조인 때 일회성 연출을 생략하는 규칙이 구현 절에 없다.
  - 풀린 미결은 없다. 가드 미결이 [어빌리티 규칙](concepts/어빌리티-규칙.md)과 [게임 개요와 전투 방향](concepts/게임-개요와-전투-방향.md)에 중복돼 있다.
- 무인 실행이라 점검 결과로는 아무 문서도 고치지 않았다(적재로 고친 문서만 바뀌었다).

## [2026-10-05] lint | 구현 절 최신화 (사용자 요청)
- 적재: 미적재 자료 0건, 낡은 자료 0건이라 새 요약은 없다.
- 낡은 코드 출처 24건이 걸린 문서의 구현 절을 HEAD fea47be89 코드·에셋 목록으로 다시 확인하고 출처 커밋을 갱신했다. 10-05 주간 점검의 의미 점검 항목도 함께 반영했다.
- 고친 문서: [피격 경직](concepts/피격-경직.md), [그로기와 처형](concepts/그로기와-처형.md), [어빌리티 규칙](concepts/어빌리티-규칙.md), [게임 개요와 전투 방향](concepts/게임-개요와-전투-방향.md), [체크포인트와 리스폰](concepts/체크포인트와-리스폰.md), [장치와 배치물](concepts/장치와-배치물.md), [아이템과 회복](concepts/아이템과-회복.md), [스탯과 피해 계산](concepts/스탯과-피해-계산.md), [적 몬스터](concepts/적-몬스터.md), [기획 작업 도구](concepts/기획-작업-도구.md), [초반 구간과 퀘스트](concepts/초반-구간과-퀘스트.md), [플레이어 캐릭터](concepts/플레이어-캐릭터.md), [현광](entities/현광.md), [커스터](entities/커스터.md), [2026.08.22-회의](sources/2026.08.22-회의.md)(반영한 문서에 체크포인트 추가)
- 위키와 코드가 달랐던 주요 항목(코드 기준으로 고쳤다):
  - 체크포인트는 PlayerStart 기반이 아니라 장치(`BP_CheckPoint`·`ST_CheckPoint`)다. 휴식은 HP만 채우고 MP는 회복하지 않는다. 저장은 부활 위치뿐이고 장치·처치 기록은 디스크에 저장하지 않는다(추정이 아니라 확인).
  - 현광 궁극기1 코스트는 MP 3이 아니라 UP 100이고, 분신 협공은 UP +33.4다. 09-19 회의자료와 다르며 근거는 커밋 1b9a2df58 메시지뿐이다.
  - 잠입 모드(07-31)는 코드에 앉기 토글만 있고 뒤잡·AI 감지와 이어져 있지 않다.
  - 넉백은 루트모션 애니가 아니라 거리 수치(HitReact 300·GuardReact 100)로 민다(04-25 결정과 다름). PC 스킬·궁극기 피격은 반응 몽타주가 패턴 몽타주를 밀어내 패턴을 끊고, 평타 피격은 '피해만'이 아니라 가산 피격 연출이 난다.
  - 앞잡 반경은 3m가 아니라 150cm, '가장 가까운 적' 규칙 없음, 그로기 중 피해 30% 증가 없음.
  - 포션 최대 충전 수는 4가 아니라 3이다. 패시브 수급은 MP·UP가 아니라 UP만이다.
  - 처형 점유(`State.FinisherReserved`)·짝 연출 중 사망 지연·그로기 유발 타격의 반응 제거·가드 재진입 시 퍼펙트 가드 창, 콤보의 한 활성화 진행(`WaitInputPress`), RandomWeight 가중치, 공격 예고 큐 4색(Yellow 포함, 판정과 연결 안 됨)을 구현 절에 넣었다.
- 결정 추가: 2026-10-02 회복 전 피격 시 포션 회복·차감 모두 안 함(사용자 결정, 10-02 코드 리뷰 R6), 2026-10-02 일반 피격이 아이템 사용 취소(R5), 2026-08-22 체크포인트를 자체 완결 장치로 분류.
- 정리: 가드 폐지 미결은 [게임 개요와 전투 방향](concepts/게임-개요와-전투-방향.md)에서 관리하고 [어빌리티 규칙](concepts/어빌리티-규칙.md)은 링크만 둔다. [피격 경직](concepts/피격-경직.md)↔[아이템과 회복](concepts/아이템과-회복.md), [초반 구간과 퀘스트](concepts/초반-구간과-퀘스트.md)→[기획 작업 도구](concepts/기획-작업-도구.md) 연결을 더했다.
- 기계 점검(수정 뒤): 연결 후보 1(현광의 '피격 경직', 지난 점검대로 둔다) 말고 모두 0.
