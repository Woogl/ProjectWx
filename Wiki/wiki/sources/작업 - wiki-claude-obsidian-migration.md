---
type: source
title: "작업 - wiki-claude-obsidian-migration"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - "source"
  - "작업-기록"
  - "Wiki"
  - "claude-obsidian"
summary: "옛 LLM Wiki(.wiki, llm-wiki)를 claude-obsidian vault Wiki/로 바꾸고, 쓰기는 매일 Routine 정기 갱신과 대시보드에서 고른 AI의 즉시 갱신(WSL 래퍼)으로만 하게 한 2026-09-26 작업 기록으로, 체크리스트 22/22 통과로 완료됐다."
source_type: task-record
source_id: src-62727f69018c0cf3ae01
sha256: 092b806675eae7750252d2eeee68ac261f9abf6b40446a16d1eb227f37a7c7c1
authority: primary
independence_key: ".agents/workflow/tasks/wiki-claude-obsidian-migration.md"
review_state: active
refresh_due: 2027-03-25
original_paths:
  - ".agents/workflow/tasks/wiki-claude-obsidian-migration.md"
raw_copy: ".raw/captured/092b806675eae7750252d2eeee68ac261f9abf6b40446a16d1eb227f37a7c7c1.md"
claim_ids:
  - clm-5b90e96024-c1
  - clm-5b90e96024-c2
  - clm-5b90e96024-c3
  - clm-5b90e96024-c4
  - clm-5b90e96024-c5
  - clm-5b90e96024-c6
key_claims:
  - "사용자는 2026-09-26 Wiki 플러그인을 claude-obsidian으로 바꾸고, Wiki 쓰기는 정기 갱신으로만 하며, Wiki는 Obsidian으로 보고, 기존 내용은 새로 수집하기로 결정했다."
  - "사용자는 claude-obsidian 버전을 사람이 수동으로 올리기로 했고(D8), 플러그인을 순정 그대로 쓰라는 지시에 따라 저장소의 claude-obsidian 사본은 지워지고 Routine 환경의 설정 스크립트가 태그 고정으로 순정 플러그인을 설치하게 되었다."
  - "사용자 결정(D9·D11)으로 대시보드 Wiki 갱신은 클라우드 Routine이 아니라 대시보드에서 고른 AI가 이 PC에서 하고, Routine은 매일 정기 갱신에만 쓴다."
  - "claude-obsidian 2.2.0은 Windows 네이티브에서 vault 쓰기를 UNSUPPORTED_PLATFORM으로 거부하고, WSL 기본 /mnt/c 마운트에서는 쓰기가 RESULT_DRIFT로 되돌려지며, metadata 옵션 마운트에서는 쓰기가 통과함이 실측됐다."
  - "claude-obsidian 레저는 쓰기마다 active 원자료의 사본 바이트를 다시 해시하고 확정 주장은 refresh_due가 지나지 않은 active 원자료가 받쳐야 하므로, Wiki/.raw는 커밋해야 한다."
  - "wiki-claude-obsidian-migration 작업의 사람 항목(Wiki 갱신 버튼, Obsidian으로 읽기, Routine 순정 플러그인 설치)은 이우성이 2026-09-26 통과로 확인했고, 사람 코드 리뷰는 2026-09-27 사용자 결정으로 이번에만 AI 코드 리뷰로 대신했다."
---

# 작업 - wiki-claude-obsidian-migration

- 원본: `.agents/workflow/tasks/wiki-claude-obsidian-migration.md`
- 원자료 사본: `.raw/captured/092b806675eae7750252d2eeee68ac261f9abf6b40446a16d1eb227f37a7c7c1.md` (수집 2026-09-26, 재확인 기한 2027-03-25)

## 개요

AI 게임 개발 워크플로우에 claude-obsidian과 llm-wiki 중 무엇이 맞는지 묻는 사용자 질문에서 시작해, 옛 `.wiki/`(llm-wiki)를 지우고 이 vault(`Wiki/`)로 옮긴 2026-09-26 작업 기록이다. 조사와 첫 결정은 클라우드 세션에서, 검토·구현은 로컬 세션에서 했다. 상태는 완료(체크리스트 22/22 통과)다. Wiki 갱신 절차의 현재 정본은 저장소의 `Wiki/README.md`이고, 이 페이지는 결정 이력과 근거를 모은다.

## 요청

> 사용자 2026-09-26: "claude obsidian이 더 나아보이네요. 워크플로우도 claude obsidian 기반으로 옮길 수 있을까요? 웹페이지에서 버튼 누르면 터미널로 AI 실행되면 될거 같아서요."

> 사용자 2026-09-26: "그런데 작업은 새 세션에서 옮겨서 할거에요. 여기는 클라우드라서, 더 안정적인 세션에서 할게요"

## 사람의 결정 원문

- D1 Wiki 쓰기 주체: "Wiki 쓰기를 정기 갱신으로만 하기로 했습니다. Obsidian의 기능들을 써보고 싶습니다."
- D2 Wiki 화면: "위키 뷰어는 옵시디언 그대로 쓰면 되겠고, 웹 기반 워크플로우만 있으면 되겠네요"
- D3 기존 내용: "새로 수집합시다."
- D4 갱신 시작 방식: "둘 다"(대시보드 버튼과 예약 실행). D5 실행 위치: "B"(클라우드 Routine). D6 주기: "매일 새벽". D7 갱신 결과 병합: "자동 병합".
- D8 버전: "claude-obsidian 버전은 항상 최신으로 유지했으면 해요" → 이어서 "claude-obsidian 버전 업데이트는 수동으로 사람이 할게요"
- Q1(새 폴더 `Wiki/`에 만들고 검증 뒤 `.wiki/` 삭제)·Q3(첫 수집은 기획서 Markdown과 완료된 작업 기록)·Q4(main 직접 푸시)·Q5(어빌리티·이펙트·캐릭터 목록은 vault 밖 `Saved/AbilitySystemLists/`): "나머지는 추천대로 진행하세요."
- Q2 사람의 노트 편집: "웹브라우저에서 사람이 테스트 체크리스트를 갱신하거나, 추가 의견을 작성할 수 있었으면 해요" → Obsidian 노트는 사람이 고치지 않고, 고칠 내용은 웹 워크플로우로 받아 작업 기록에 남긴다고 해석했다.
- Q6 웹에서 받는 의견 범위: "지금 그대로 둡시다. 필요하면 나중에 추가할게요."
- D9 대시보드 Wiki 갱신: ""Wiki 갱신" 버튼이 클로드 Routine을 실행하는 것이 아니라, 선택된 AI 서비스로 처리했으면 해요. Dailiy 갱신은 클로드 Routine으로 처리하겠지만, 필요에 따라서는 즉시 갱신하는 것이 필요할 수도 있으니까요." · WSL 전제 확인에 "네 설치해주세요"
- D10 다른 PC의 준비물: "네, 다른 사람도 워크플로우에서 위키 갱신 버튼 눌렀을 때 설치 안되어있으면 자동 설치되게 합시다."
- D11 즉시 갱신도 Routine으로 하자는 AI 추천(철회): "우리의 워크플로우를 claude 전용으로 고정시키고 싶지 않아요" · "Routine 링크는 순수하게 데일리 갱신 용도로만 씁시다"
- D12 이 PC의 claude-obsidian 쓰기: "혹시 WSL 없이 위키 갱신하는 스킬을 만들 수는 없을까요? 절차가 너부 복잡해지는 것 같아서요." → 래퍼 안과 사용자 쪽 부담을 설명한 뒤 "네 진행합시다"
- 순정 지시: "claude-obsidian 플러그인은 순정 그대로 쓰고 싶어요. 프로젝트 워크플로우는 이 플러그인을 간접적으로 활용할 뿐이구요." · "가급적이면 플러그인 순정을 따랐으면 해요"
- 구현 중 지시: "기존 워크플로우의 핵심 규칙만 지키면 됩니다. 모든 것을 온전히 옮기지는 않아도 됩니다." · "옛 문서 호환도 신경쓰지 않아도 됩니다. 레거시는 곧 지울거라서요"
- 위치 결정: `Saved/Wiki`는 `Saved/Workflow`로 옮겼다. vault 위치는 "vault 유지합시다"로 `Wiki/`에 두었고, `.agents/workflow`와 `Docs/`도 옮기지 않았다("네, 계속 작업합시다"). `OpenWiki.bat`은 "네, 지워주세요."로 지웠다.
- 구현 승인: 이우성 2026-09-26. 계획은 위 답변대로 고쳐 적은 것이라 2026-09-27 통폐합에서 그 답변을 승인 근거로 적었다.

## 구현 경과

- 첫 Routine 방식: 대시보드 버튼이 Routine `/fire`를 부르고 Routine이 claude-obsidian을 받아 쓰게 했으나, 클라우드 자동 모드 권한 검사가 받은 스킬 문서 읽기를 거부해 첫 실행이 멈췄다. 한때 v2.2.0 실행 부분을 `.agents/vendor/`에 넣었다가(101개 → 58개로 축소), 순정 지시에 따라 지우고 클라우드 환경 `Wiki`의 설정 스크립트가 태그 고정(`AgriciDaniel/claude-obsidian#v2.2.0`)으로 순정 플러그인을 설치하게 바꿨다. 이 환경에서 세션이 플러그인 설치본의 스킬·CLI만 쓰는 것을 실측했다.
- 첫 수집: 두 번째 Routine 실행(13분)이 vault를 init하고 원자료 109건(기획서 29, 완료 작업 기록 14, 옛 결정 노트 66)을 수집해 원자료 페이지 109장·주제 페이지 18장을 만들었다(커밋 `20b0751`~`33cb7ea`, lint 전 항목 0). 한 묶음에서 검증 실패가 파이프에 가려져 사본만 든 커밋이 먼저 올라간 일이 있었다.
- 줄바꿈: 이 PC(`core.autocrlf=true`)가 `.raw/captured/`를 CRLF로 꺼내 해시 불일치 109건이 나서 루트 `.gitattributes`에 `Wiki/.raw/** -text`를, Obsidian 재저장 표시를 막으려고 `Wiki/** text=auto eol=lf`를 더했다.
- 즉시 갱신(D9~D12): 대시보드 Wiki 갱신은 고른 AI가 origin/main의 LF sparse 작업 트리 `Saved/Workflow/wiki-update-tree`에서 README 절차를 따르고, claude-obsidian 명령만 `Wiki-Obsidian.cjs` 래퍼가 WSL(Ubuntu, root, 저장소 드라이브 metadata 마운트)에서 실행한다. 준비물이 없으면 버튼이 설치를 시작한다. Routine `/fire` 호출 코드와 토큰 파일은 지웠다.
- 처리할 AI 고르기: 사용자 요청("모든 작업에 대해 처리할 AI를 최상단 드롭다운으로 선택하게 해주세요." → "좌측 탭 아래쪽으로 합시다. 항상 보이게요.")으로 왼쪽 메뉴 아래 드롭다운 하나가 모든 전달·터미널·Wiki 갱신에 쓰인다.
- 워크플로우 쪽 변경: 완료 뒤 AI의 Wiki 정리를 없앴고, 대시보드는 Workflow 화면만 둔다.
- AI 자기 평가: 이 PC 갱신에서 플러그인 지원 경로를 돌아가는 장치(설치 창·재부팅 판정·작업 트리·경로 변환)가 쌓였고, 쓰기 시험을 설계 단계에서 먼저 하지 않은 것은 AI의 잘못이라고 기록했다.
- 종합 점검([[작업 - workflow-inspection]])이 이 전환의 회귀(대시보드 데이터 이스케이프, `f9bf3de97`)를 찾아 고쳤다.

## 확인한 사실 (claude-obsidian v2.2.0·Routine)

- claude-obsidian은 Windows 네이티브(Git Bash 포함)에서 읽기·미리보기만 되고 vault 쓰기는 `UNSUPPORTED_PLATFORM`으로 거부한다. WSL 기본 `/mnt/c`에서는 `init` 적용이 `RESULT_DRIFT`로 되돌려졌고(0600이 0777로 읽힘), metadata 마운트와 WSL 파일 시스템에서는 통과했다(실측).
- vault 밖 파일은 바로 수집할 수 없어 `inbox/`에 넣고 capture하면 `.raw/captured/<sha256><확장자>` 사본이 생긴다. 내용 추출은 없고 `.md .txt .json .csv .yaml .yml .html`만 텍스트로 다룬다.
- 레저는 쓰기마다 active 원자료의 사본 바이트를 다시 해시하므로 `.raw/`는 커밋해야 한다. 확정 주장은 `refresh_due`가 지나지 않은 원자료가 받쳐야 하고 기본 기한은 없다.
- 실행 사이의 사람 편집은 감지하지 않는다. 충돌 확인은 계획→적용 사이뿐이라(Q2 이유 정정), 사람이 고친 페이지는 다음 갱신이 통째로 바꿀 수 있다.
- 하위 폴더 vault는 저장소 루트에서 찾지 못해 `--vault Wiki`가 필요하고, `checkpoint`는 vault가 Git 최상위여야 해서 쓸 수 없다. `.vault-meta/`는 무시 대상이다.
- 클라우드 세션은 저장소 설정의 플러그인을 설치하지 않지만 환경 설정 스크립트로 설치한 플러그인은 세션에 실렸다(실측). Routine은 PR 자동 병합 수단이 없고, 기존 Routine은 main에 직접 푸시해 왔다. 여러 머신이 한 vault에 쓰는 수렴 방식은 플러그인에 정해져 있지 않다(#154).

## 검증 범위

- AI 항목 19개 통과(도구 실행): 워크플로우 Node 테스트, Wiki 갱신 서버 경로(결함 2건 주입 검출), 문서 링크, 목록 생성 위치, 하네스, 원자료 사본 줄바꿈, 레거시 제거 검색, WSL 쓰기 시험, 래퍼 lint(131쪽·링크 1630개 이슈 0), 첫 설치 흐름(시험 배포판), Windows Codex 시범 갱신(푸시 주소를 막은 사본에서 수집·lint·커밋), 이 PC 실제 갱신(바뀐 것 없음), 저장소 사본 제거 등.
- 사람 항목 3개 통과(이우성 2026-09-26): Wiki 갱신 버튼(Claude로 약 2분, 바뀐 것 없어 커밋 없음), Obsidian으로 읽기, Routine 순정 플러그인 설치.
- 2026-09-27 통폐합에서 사람 코드 리뷰는 사용자 결정으로 이번에만 AI 코드 리뷰로 대신했고(다음에는 사람이 리뷰), 다음 날 예약 실행 확인은 확인 대기 기록 `workflow-wrapup-checks.md`로 옮겼다.
- 문서 갱신이나 lint 통과는 게임 동작 검증이 아니며, 이 작업은 게임 코드·빌드와 무관하다.

## 미결정·남은 것

- 추적 중인 `.obsidian/app.json`·`appearance.json`·`graph.json`은 Obsidian이 설정을 저장할 때 다시 바뀔 수 있다고 기록했다.
- Ubuntu의 Linux 사용자 `woogle`은 옛 첫 실행 흐름에서 만든 것으로, 새 방식에는 필요 없지만 남겼다.
- 헤더·회의록 수집은 운영해 본 뒤 더할 수 있다고 남겼다(Q3).

## 관련 주제

- [[Wiki 운영]]
- [[작업 절차(Workflow)]]
- [[작업 - workflow-inspection]]
- [[작업 - harness-legacy-cleanup]]
- [[작업 - wiki-regeneration]]

## 핵심 주장

- 사용자는 2026-09-26 Wiki 플러그인을 claude-obsidian으로 바꾸고, Wiki 쓰기는 정기 갱신으로만 하며, Wiki는 Obsidian으로 보고, 기존 내용은 새로 수집하기로 결정했다. ^c1
- 사용자는 claude-obsidian 버전을 사람이 수동으로 올리기로 했고(D8), 플러그인을 순정 그대로 쓰라는 지시에 따라 저장소의 claude-obsidian 사본은 지워지고 Routine 환경의 설정 스크립트가 태그 고정으로 순정 플러그인을 설치하게 되었다. ^c2
- 사용자 결정(D9·D11)으로 대시보드 Wiki 갱신은 클라우드 Routine이 아니라 대시보드에서 고른 AI가 이 PC에서 하고, Routine은 매일 정기 갱신에만 쓴다. ^c3
- claude-obsidian 2.2.0은 Windows 네이티브에서 vault 쓰기를 UNSUPPORTED_PLATFORM으로 거부하고, WSL 기본 /mnt/c 마운트에서는 쓰기가 RESULT_DRIFT로 되돌려지며, metadata 옵션 마운트에서는 쓰기가 통과함이 실측됐다. ^c4
- claude-obsidian 레저는 쓰기마다 active 원자료의 사본 바이트를 다시 해시하고 확정 주장은 refresh_due가 지나지 않은 active 원자료가 받쳐야 하므로, Wiki/.raw는 커밋해야 한다. ^c5
- wiki-claude-obsidian-migration 작업의 사람 항목(Wiki 갱신 버튼, Obsidian으로 읽기, Routine 순정 플러그인 설치)은 이우성이 2026-09-26 통과로 확인했고, 사람 코드 리뷰는 2026-09-27 사용자 결정으로 이번에만 AI 코드 리뷰로 대신했다. ^c6
