---
type: concept
title: "Wiki 운영"
created: 2026-09-26
updated: 2026-09-26
status: developing
tags:
  - concept
summary: "팀 Wiki의 운영 규칙과 전환 이력"
sources:
  - "[[결정 노트 - 2026-09-22-current-workflow]]"
  - "[[결정 노트 - 2026-09-22-verified-stock-rule]]"
  - "[[결정 노트 - 2026-09-25-workflow-simplification]]"
  - "[[결정 노트 - 2026-09-26-refresh-commit-trace]]"
  - "[[결정 노트 - 2026-09-26-workflow-image-removal]]"
  - "[[결정 노트 - 2026-09-26-workflow-legacy-removal]]"
  - "[[결정 노트 - 2026-09-26-workflow-process-diagram]]"
  - "[[결정 노트 - 2026-09-26-workflow-ssot-copies]]"
  - "[[작업 - wiki-regeneration]]"
  - "[[작업 - workflow-review]]"
  - "[[작업 - harness-legacy-cleanup]]"
  - "[[작업 - wiki-claude-obsidian-migration]]"
  - "[[작업 - workflow-final-fixes]]"
  - "[[작업 - workflow-inspection]]"
---

# Wiki 운영

팀 Wiki의 운영 규칙과 전환 이력에 관한 원자료 요약을 모은 주제 페이지입니다. 문장마다 끝의 링크가 출처이며, 절 이름으로 기획 요구사항·사람의 확정 결정·코드 구현 관찰·검증 범위·미결정을 구분합니다. 구현 관찰은 원자료 작성 시점의 코드 기준이고, 문서 갱신이나 Wiki lint 통과는 게임 동작 검증이 아닙니다. 이 페이지는 결정 이력과 근거를 모으며, 지금의 Wiki 갱신 절차는 저장소의 `Wiki/README.md`를 봅니다.

## 요구사항

- 사용자는 2026-09-26 "claude obsidian이 더 나아보이네요. 워크플로우도 claude obsidian 기반으로 옮길 수 있을까요? 웹페이지에서 버튼 누르면 터미널로 AI 실행되면 될거 같아서요."라며 Wiki 플러그인 전환을 요청했다. ([[작업 - wiki-claude-obsidian-migration]])

## 확정 결정

- 옛 .wiki 재생성 작업은 문서 작성일과 인간 검증일을 구분하고 C++ 정적 확인이 빌드·실행·바이너리 에셋 검증을 대신하지 않는다는 기준을 따랐다. ([[작업 - wiki-regeneration]])
- 사용자는 2026-09-22 옛 LLM Wiki 기사의 verified를 WX 전용 인간 확인 방침 대신 순정 규칙(편찬·재확인 때 그날 날짜 기록)으로 되돌리도록 결정했다. ([[결정 노트 - 2026-09-22-verified-stock-rule]])
- 옛 Wiki schema.md는 verified를 편찬 없는 구조 이관에는 넣지 않도록 규정했다. ([[결정 노트 - 2026-09-22-verified-stock-rule]])
- 사용자는 2026-09-26 쓰는 곳이 없는 Wiki·Workflow 문서 이미지 기능(PNG 묶기·Markdown 이미지 표시)을 지우기로 최종 결정했고, 앞서 잔재 제거 때 남겨 둔 보류를 뒤집었다. ([[결정 노트 - 2026-09-26-workflow-image-removal]])
- Wiki의 Workflow 문서는 단계 표·구현 승인 규칙·기록 형식·체크리스트 작성 규칙을 다시 적지 않고 작업 절차 정본을 가리키며, 도구 구조·서버 동작·결정 이유만 담는다. ([[결정 노트 - 2026-09-26-workflow-ssot-copies]])
- 사용자는 2026-09-26 Wiki 플러그인을 llm-wiki에서 claude-obsidian으로 바꾸고, Wiki 쓰기는 정기 갱신으로만 하며("Obsidian의 기능들을 써보고 싶습니다"), Wiki는 Obsidian으로 보고 웹에는 워크플로우만 두며, 기존 내용은 새로 수집하기로 했다(D1~D3). ([[작업 - wiki-claude-obsidian-migration]])
- 정기 갱신은 대시보드 버튼과 매일 새벽 예약 둘 다로 하고(D4·D6), 예약 실행은 클라우드 Routine이 맡으며(D5), 결과는 사람 확인 없이 반영하되(D7) 방식은 main 직접 푸시로 정했다(Q4). ([[작업 - wiki-claude-obsidian-migration]])
- 사용자는 claude-obsidian 버전을 사람이 수동으로 올리기로 했고(D8, "claude-obsidian 버전 업데이트는 수동으로 사람이 할게요"), "claude-obsidian 플러그인은 순정 그대로 쓰고 싶어요. 프로젝트 워크플로우는 이 플러그인을 간접적으로 활용할 뿐이구요."라고 지시해 저장소의 claude-obsidian 사본을 지우고 클라우드 환경 `Wiki`의 설정 스크립트가 태그 고정으로 순정 플러그인을 설치하게 했다. ([[작업 - wiki-claude-obsidian-migration]])
- 사용자는 2026-09-26 대시보드 Wiki 갱신을 Routine 호출이 아니라 대시보드에서 고른 AI로 이 PC에서 하게 했고(D9), 준비물이 없으면 버튼이 자동 설치하며(D10), "우리의 워크플로우를 claude 전용으로 고정시키고 싶지 않아요"·"Routine 링크는 순수하게 데일리 갱신 용도로만 씁시다"로 Routine은 매일 정기 갱신에만 쓴다(D11). 이에 따라 Wiki를 쓰는 것은 Routine만이 아니라 Wiki 갱신 두 갈래다. ([[작업 - wiki-claude-obsidian-migration]])
- 이 PC의 claude-obsidian 쓰기는 AI는 Windows 그대로 두고 claude-obsidian 명령만 래퍼(`Wiki-Obsidian.cjs`)가 WSL에서 실행하는 안으로 정했다(D12, "네 진행합시다"). WSL 없이 레저를 직접 고치는 스킬은 순정 규칙("공유 변경은 트랜잭션 번들로만") 위반이라 기각했다. ([[작업 - wiki-claude-obsidian-migration]])
- 사람은 Obsidian 노트를 고치지 않고 고칠 내용은 웹 워크플로우로 남기며(Q2), 첫 수집 범위는 기획서 Markdown과 완료된 작업 기록(Q3), 어빌리티·이펙트·캐릭터 목록은 vault 밖 `Saved/AbilitySystemLists/`에 둔다(Q5). ([[작업 - wiki-claude-obsidian-migration]])
- vault 위치는 사용자 결정 "vault 유지합시다"로 `Wiki/`에 두었고, 대시보드 생성물 폴더는 `Saved/Wiki`에서 `Saved/Workflow`로 옮겼으며, `Docs/`와 `.agents/workflow`는 옮기지 않았고 `OpenWiki.bat`은 지웠다(2026-09-26). ([[작업 - wiki-claude-obsidian-migration]])
- 2026-09-26 워크플로우 종합 점검에서 사용자는 "Q4는 Markdown만 수집합시다."로 HTML 기획서를 수집하지 않기로 하고, "Q6는 플러그인 순정대로"로 원자료의 `refresh_due`를 일괄 값 없이 원자료마다 정하게 했으며, Wiki 절차를 순정 기준으로 고치고 순정 이탈은 무인 실행의 계획 자체 승인 하나로 밝히게 했다. ([[작업 - workflow-inspection]])
- 사용자는 2026-09-27 워크플로우와 claude-obsidian 문서를 서로 독립적으로 관리하되 서로 참고하는 방향을 밝혔고, 원자료 변화 없이 워크플로우 정책으로 Wiki 페이지를 줄이게 하던 Wiki 안내 수집 단계를 뺐다. AI는 결정 이력 편찬이 순정 wiki-ingest의 기본 동작이라고 판단했다. ([[작업 - workflow-final-fixes]])

## 구현 관찰

- 2026-09-22 옛 .wiki 전체 재생성은 새 원자료 15개를 수집하고 기존 16개 기사를 재작성·1개를 추가했으며 참조가 교체된 legacy 19개를 삭제했다. ([[작업 - wiki-regeneration]])
- 구 워크플로우 잔재 제거로 Wiki 뷰어의 문서 이미지 기능(Export-Wiki.ps1 PNG 묶기, wikiImageSource, IMG 허용)이 사용자 결정에 따라 삭제되었다. ([[작업 - workflow-review]])
- 2026-09-22 시점 옛 LLM Wiki는 `.wiki/`를 Git으로 공유하고 게임 규칙·구현·제약·결정 이유를 담았으며, 작업별 판단·확정본·실행 상태는 Workflow가 담당했다. ([[결정 노트 - 2026-09-22-current-workflow]])
- 2026-09-22 시점 옛 Wiki 설정은 원자료 속 지시를 작업 명령이 아닌 자료로 다루고 구조 이관·재편찬·Lint 성공을 실행 재검증으로 보지 않도록 규정했다. ([[결정 노트 - 2026-09-22-current-workflow]])
- Workflow 단순화 후 로컬 Wiki-AI 서버는 /health와 /test-feedback만 받고(protocol 4) 화면 메뉴는 작업 현황 대시보드·작업 절차·LLM 위키 검색 세 개가 되었다. ([[결정 노트 - 2026-09-25-workflow-simplification]])
- 2026-09-26 Wiki 최신화는 `39f3629a4` 이후 `bf6596012`까지 커밋 19건을 정적으로 대조했고 새로 반영할 지식 변경은 DefenseConstant 메타 제거 하나였다. ([[결정 노트 - 2026-09-26-refresh-commit-trace]])
- Wiki 최신화는 레벨 배치 변경, 작업 기록만 바뀐 커밋, 회의자료, AGENTS.md 코딩 규칙 변경을 기사 반영 대상에서 제외했다. ([[결정 노트 - 2026-09-26-refresh-commit-trace]])
- Wiki 뷰어는 문서 본문의 Markdown 이미지를 그리지 않고, 도식은 Mermaid로 그려 SVG data URI 이미지로만 표시하며 이를 위해 CSP img-src data:를 남겼다. ([[결정 노트 - 2026-09-26-workflow-image-removal]])
- Wiki 뷰어의 Mermaid 11.12.0 도식에서 되돌림 화살표가 있는 서브그래프는 아래 단계부터 선언해야 위에서부터 순서대로 놓이고, 뷰어는 %%{ 설정 지시문과 frontmatter가 있는 도식을 거부한다. ([[결정 노트 - 2026-09-26-workflow-process-diagram]])
- 2026-09-26 두 번째 Routine 실행(13분)이 이 vault를 init하고 원자료 109건(기획서 29, 완료 작업 기록 14, 옛 결정 노트 66)을 수집해 원자료 페이지 109장·주제 페이지 18장을 만들었다(커밋 `20b0751`~`33cb7ea`). 한 묶음에서 검증 실패가 파이프에 가려져 사본만 든 커밋이 먼저 올라간 일이 있었다. ([[작업 - wiki-claude-obsidian-migration]])
- 이 PC(`core.autocrlf=true`)가 원자료 사본을 CRLF로 꺼내 해시 불일치 109건이 나서, 루트 `.gitattributes`에 `Wiki/.raw/** -text`와 `Wiki/** text=auto eol=lf`를 더했다. ([[작업 - wiki-claude-obsidian-migration]])
- 대시보드 Wiki 갱신은 고른 AI가 origin/main의 LF sparse 작업 트리 `Saved/Workflow/wiki-update-tree`에서 일하고, claude-obsidian 명령만 `Wiki-Obsidian.cjs`가 WSL(Ubuntu, root)에서 저장소 드라이브를 metadata 옵션으로 붙여 실행한다. 사용자 작업 트리에서 갱신하면 푸시하지 않은 사용자 커밋이 섞이거나 pull이 실패할 수 있어 작업 트리를 따로 두었다. ([[작업 - wiki-claude-obsidian-migration]])
- claude-obsidian 2.2.0은 Windows 네이티브에서 vault 쓰기를 `UNSUPPORTED_PLATFORM`으로 거부하고, WSL 기본 `/mnt/c`에서는 `init` 적용이 `RESULT_DRIFT`로 되돌려지며(0600이 0777로 읽힘), metadata 마운트와 WSL 파일 시스템에서는 통과한다(2026-09-26 실측). ([[작업 - wiki-claude-obsidian-migration]])
- claude-obsidian 레저는 쓰기마다 active 원자료의 사본 바이트를 다시 해시하므로 `.raw/`를 커밋하고, 확정 주장은 `refresh_due`가 지나지 않은 active 원자료가 받쳐야 한다. 실행 사이의 사람 편집은 감지하지 않아 사람이 고친 페이지는 다음 갱신이 통째로 바꿀 수 있다. ([[작업 - wiki-claude-obsidian-migration]])
- 클라우드 세션은 저장소 설정의 플러그인을 설치하지 않지만, 환경 설정 스크립트로 설치한 플러그인은 세션에 실렸고 세션은 설치본의 스킬·CLI만 썼다(2026-09-26 실측). ([[작업 - wiki-claude-obsidian-migration]])
- 2026-09-26 종합 점검 D1은 원자료 112건의 `refresh_due`가 모두 2026-10-26이라 2026-10-27부터 lint 오류 343건이 나고 레저 쓰기가 막히는 결함이었고, 대시보드 Wiki 갱신(`47f79230b`)이 원자료 109건을 자료별 기한으로 한 트랜잭션에서 재확인해 해소했다. ([[작업 - workflow-inspection]])
- 종합 점검 반영으로 두 스킬(comment-cleanup·module-review)이 작업 중인 AI에게 Wiki 직접 수정을 지시하던 문장이 지워졌다. ([[작업 - workflow-inspection]])
- 2026-09-27 하네스 정리로 옛 Wiki 시절 이름의 워크플로우 스크립트가 `Workflow-*`·`Export-WorkflowPage.ps1`·`CheckDocLinks.ps1` 등으로 바뀌었고, 이제 이름에 Wiki가 남은 스크립트는 claude-obsidian 래퍼 `Wiki-Obsidian.cjs`뿐이다. ([[작업 - harness-legacy-cleanup]])

## 검증 범위

- 옛 .wiki 재생성은 lint·링크 검사·뷰어 테스트·Edge 렌더링으로만 검증되었고 빌드와 게임 실행은 하지 않았다. ([[작업 - wiki-regeneration]])
- 옛 Wiki의 verified 날짜는 편찬·재확인 날짜이며 빌드·게임 실행 검증일을 뜻하지 않는다. ([[결정 노트 - 2026-09-22-verified-stock-rule]])
- 2026-09-26 최신화의 커밋 대조는 정적 조사이며 빌드·PIE·에셋 편집기 검증은 하지 않았다. ([[결정 노트 - 2026-09-26-refresh-commit-trace]])
- 문서 이미지 기능 제거는 Export-Wiki 재생성, CheckWikiLinks 0건, 뷰어 자동 테스트, 헤드리스 Edge 렌더로 확인했고 사람의 화면 확인은 노트에 없다. ([[결정 노트 - 2026-09-26-workflow-image-removal]])
- claude-obsidian 전환의 사람 항목 「Wiki 갱신 버튼」「Obsidian으로 읽기」「Routine 순정 플러그인 설치」는 이우성이 2026-09-26 통과로 확인했다. 버튼 확인 실행은 새 원자료가 없어 커밋이 없었으므로 그 실행으로 쓰기 단계는 확인되지 않았고, 쓰기는 AI의 Windows Codex 시범 갱신(푸시 주소를 막은 사본)과 WSL 쓰기 시험으로 확인했다. ([[작업 - wiki-claude-obsidian-migration]])
- 전환 작업의 사람 코드 리뷰는 2026-09-27 사용자 결정으로 이번에만 AI 코드 리뷰로 대신했고, 다음 날 예약 실행과 대시보드 Wiki 갱신의 실제 확인은 확인 대기 기록 `workflow-wrapup-checks.md`로 옮겨졌다. ([[작업 - wiki-claude-obsidian-migration]], [[작업 - workflow-inspection]])

## 미결정·충돌

- 구 워크플로우 잔재 제거 노트는 문서 PNG 묶기 기능을 사용자 미결로 남겼지만 같은 날 이미지 기능 제거 결정으로 뒤집혔다. ([[결정 노트 - 2026-09-26-workflow-legacy-removal]])
- 완료된 작업 기록만 수집하므로 확인 대기 기록(워크플로우 결정 기록 포함)은 사람 항목이 끝날 때까지 Wiki에 들어오지 않는다. 확인 대기 기록까지 잠정 수집하는 안은 사본이 쌓이고 검증 안 된 내용이 들어가 권하지 않았다(2026-09-27). ([[작업 - workflow-final-fixes]])
- 여러 머신이 한 vault에 쓰는 수렴 방식은 claude-obsidian에 정해져 있지 않아(#154), 푸시가 거절되면 합치지 않고 최신 main에서 다시 한다. ([[작업 - wiki-claude-obsidian-migration]])
- 추적 중인 `.obsidian/app.json`·`appearance.json`·`graph.json`은 Obsidian이 설정을 저장할 때 다시 바뀔 수 있다고 전환 기록에 남아 있다. ([[작업 - wiki-claude-obsidian-migration]])

## 원자료

- [[결정 노트 - 2026-09-22-current-workflow]] — 2026-09-22 시점 AGENTS.md·옛 LLM Wiki 설정·Workflow 절차와 실행 스크립트의 계약을 발췌한 정적 조사 노트로 사람 판단·AI 실행 경계를 기록한다
- [[결정 노트 - 2026-09-22-verified-stock-rule]] — 옛 LLM Wiki 기사의 verified 필드를 WX 전용 인간 확인 방침에서 순정 규칙(편찬·재확인 날짜 기록)으로 되돌린 2026-09-22 사용자 결정 기록
- [[결정 노트 - 2026-09-25-workflow-simplification]] — 사용자 승인으로 Workflow를 정하기·만들기·확인하기 3단계와 상태 3개로 줄이고 웹 새 작업 경로를 폐지하며 AI·사람 담당 테스트 체크리스트를 도입한 기록
- [[결정 노트 - 2026-09-26-refresh-commit-trace]] — 직전 Wiki 최신화 39f3629a4 이후 bf6596012까지 19건 커밋을 정적 대조해 새로 반영할 것은 DefenseConstant 입력 제한 메타 제거 하나였음을 기록
- [[결정 노트 - 2026-09-26-workflow-image-removal]] — 구 워크플로우 그림 삭제 뒤 쓰는 곳이 없던 문서 이미지 기능(PNG 묶기·Markdown 이미지 표시)을 사용자 결정으로 없애고 도식은 Mermaid만 쓰게 한 노트.
- [[결정 노트 - 2026-09-26-workflow-legacy-removal]] — 사용자 요청으로 옛 웹 작업 경로 스크립트·테스트, 한 장으로 합친 옛 절차 문서와 4단계 그림, 옛 결과 형식 표시와 대시보드 CSS를 지운 노트.
- [[결정 노트 - 2026-09-26-workflow-process-diagram]] — 작업 절차 도식을 정하기·만들기·확인하기 단계 상자 배치로 바꾸고 서버·화면 코드와 대조한 노트. 추가 요청 권한과 완료 뒤 재활성 두 가지가 미결이다.
- [[결정 노트 - 2026-09-26-workflow-ssot-copies]] — 워크플로우 SSoT를 엄격히 지키기로 한 사용자 결정에 따라 처리 프롬프트와 Wiki의 규칙 사본을 정리하고 정본을 가리키게 한 노트.
- [[작업 - wiki-regeneration]] — 옛 .wiki LLM Wiki를 현재 코드·설정·기획 기준으로 전면 재생성하고 대체된 legacy 문서를 정리한 2026-09-22 완료 작업 기록
- [[작업 - workflow-review]] — AI 작업 워크플로우를 점검하고 작업 절차 한 장·세 단계·테스트 체크리스트·웹 새 작업과 이어하기로 단순화한 2026-09-22~26 완료 작업 기록
- [[작업 - harness-legacy-cleanup]] — 하네스·워크플로우·Wiki의 옛 흔적과 로컬 Saved 잔여 파일을 정리하고 워크플로우 스크립트 이름을 바꾼 2026-09-27 완료 작업 기록
- [[작업 - wiki-claude-obsidian-migration]] — 옛 LLM Wiki를 claude-obsidian vault로 바꾸고 정기·즉시 Wiki 갱신을 만든 2026-09-26 완료 작업 기록
- [[작업 - workflow-final-fixes]] — 최종 마무리 코드 리뷰 지적 수정과 워크플로우·Wiki 독립 관리 결정을 담은 2026-09-27 완료 작업 기록
- [[작업 - workflow-inspection]] — 순정 우선·장치 축소·SSOT 기준의 워크플로우 종합 점검과 Wiki 절차 순정화·기한 결정을 담은 2026-09-26 완료 작업 기록
