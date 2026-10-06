# Wiki Index

WX 프로젝트 지식의 색인이다.
운영 규칙은 [AGENTS.md](AGENTS.md), 작업 기록은 [log.md](log.md)에 있다.

## 주제

### 게임 개요
- [게임 개요와 전투 방향](topics/게임-개요와-전투-방향.md) — 학습용 오픈월드 프로젝트, 스텔라 블레이드풍에서 명조풍으로의 전투 전환
- [세계관과 시나리오](topics/세계관과-시나리오.md) — 천상민·지상민 전쟁과 '6개의 키' 초안

### 전투
- [어빌리티 규칙](topics/어빌리티-규칙.md) — 어빌리티 분류, 캔슬·차단, 무적·슈퍼아머, 후딜 캔슬
- [어빌리티 구현 구조](topics/어빌리티-구현-구조.md) — 로직은 C++·GA_는 데이터 전용, 값의 위치(쿨다운 GE_, 이동은 애니 루트모션), 입력 라우팅, 순정 GAS와 다른 곳, 다시 꺼내지 않을 대안
- [플레이어 캐릭터](topics/플레이어-캐릭터.md) — PC 공통 구조·자원·공용 어빌리티와 조작
- [캐릭터 메시와 애니메이션](topics/캐릭터-메시와-애니메이션.md) — 구동 메시와 메타휴먼 표시 메시의 이중 구조, 숨긴 리더의 틱 옵션, 메타휴먼 신세대 경로 전용
- [스탯과 피해 계산](topics/스탯과-피해-계산.md) — 스탯 정의, 피해 공식, 자원 사용 규칙
- [피격 경직](topics/피격-경직.md) — 원인·결과 판정, 경직 흐름, 패링 반응
- [그로기와 처형](topics/그로기와-처형.md) — 그로기, 앞잡(그로기 피니시), 뒤잡
- [아이템과 회복](topics/아이템과-회복.md) — 충전형 회복 포션, 아이템·보상

### 적
- [적 몬스터](topics/적-몬스터.md) — 공통 규격, 잡몹·일반·정예, 보스 공통 BT 기획, 인식과 정찰, State Tree AI 구조, 퍼셉션·리시·정찰 경로 결정
- [네임플레이트](topics/네임플레이트.md) — 적·보스 HP 표시 규칙

### 레벨
- [초반 구간과 퀘스트](topics/초반-구간과-퀘스트.md) — 첫 공성포까지의 흐름, 공성포 진지, 메인 퀘스트
- [퀘스트 시스템](topics/퀘스트-시스템.md) — 퀘스트=순정 StateTree 에셋과 GameState 컴포넌트의 러너, 볼륨 수주·체인, 스텝 Linked Asset과 오버라이드 값, 멀티플레이 보류
- [대화 시스템](topics/대화-시스템.md) — DataTable 노드 그래프 대화, 대상 쪽 정의와 PC 쪽 세션 분리, `State.Dialogue` 신호, 대사 포즈
- [상호작용](topics/상호작용.md) — PC 스캐너→`Event.Interact`→서버 어빌리티 경로, `IWxInteractable` 선택지 계약, 상호작용 대기와 NPC 자격 파생, 엔진 표준 보류
- [장치와 배치물](topics/장치와-배치물.md) — 장치(Device) 조립, 문·엘리베이터·상자·레버 규칙
- [체크포인트와 리스폰](topics/체크포인트와-리스폰.md) — 부활, 적 리젠, 체크포인트

### UI
- [UI 설계 원칙](topics/UI-설계-원칙.md) — UI 작업 때 먼저 읽는 지금 유효한 규칙: 의존·소유·수명, 위젯과 바인딩, 입력과 명령, 화면 흐름
- [UI 구조](topics/UI-구조.md) — UI 레이어와 화면 push, 입력 모드, 일시정지 재평가, Popup 명명, 월드 위젯, 기대는 CommonUI 엔진 동작
- [UI 뷰모델](topics/UI-뷰모델.md) — MVVM 층과 의존 방향, 플레이어 공유 VM의 Global Collection 등록, 리졸버와 Manual 주입, 명령·변환 함수 규칙

### 공통 구조
- [게임 프레임워크 구조](topics/게임-프레임워크-구조.md) — WxGame 단일 모듈과 에디터 모듈·CommonInput 사전 로드, GameMode BP·컨트롤러/GameState 컴포넌트 구성, 폰 없는 프론트엔드, 폰 초기화 시점, 세이브 범위
- [State Tree 작성 규칙](topics/State-Tree-작성-규칙.md) — AI·장치·퀘스트 공통의 완료 판정 정책과 머무는 상태, 단방향 바인딩, 컴포넌트 이름 지정, UOL 액터 지정, 기대는 엔진 동작

### 개발 환경
- [개발 진행과 작업 규칙](topics/개발-진행과-작업-규칙.md) — 팀 역할, 작업 규칙, 이정표
- [기획 작업 도구](topics/기획-작업-도구.md) — 공용 에셋·PCG 사용 방침(아트 자동 배치 한정), 숲 바이옴 RVT·HLOD와 도로 결정, 도구 구현의 정본 위치
- [Claude 활용](topics/Claude-활용.md) — AI 활용 원칙·결정 이유와 운영 정보의 정본 위치

## 엔티티
- [현광](entities/현광.md) — 첫 정식 PC, 분신·변신 킷과 구현 상태
- [커스터](entities/커스터.md) — 첫 보스, 창을 쓰는 학습형 보스

## 요약
### Docs/CombatDesign
- [보스 공통 BT 기획 초안](summaries/boss_common_bt_draft.md) — 기획서, 초안
- [엘리트급 몬스터 기획서](summaries/Elite_monster.md) — 기획서, 초안
- [Game Concept: Perfect Frame (가칭)](summaries/game-concept.md) — 기획서, 대체됨
- [네임플레이트 기획서](summaries/Nameplate_System.md) — 기획서, 확정
- [일반급 몬스터 기획서](summaries/normal_monster_BT.md) — 기획서, 초안
- [적 정찰 기능 기획](summaries/Patrol_Design.md) — 기획서, 확정
- [PC 규격 정리](summaries/PC규격서.md) — 사양서, 일부 대체됨
- [리스폰 시스템 기획서](summaries/Respawn_System.md) — 기획서, 일부 대체됨
- [Project WX PC 규격서](summaries/WA_PC_규격서.md) — 사양서, 확정
- [주인공 캐릭터 개별 어빌리티 규격서](summaries/WA_주인공_캐릭터.md) — 기획서, 초안
- [WX AM 기능 정리](summaries/WX_AM_기능정리.md) — 사양서, 대체됨
- [WX BT 기능 정리](summaries/WX_BT_기능정리.md) — 사양서, 대체됨
- [WX 첫 보스 기획서](summaries/WX_첫_보스_기획서.md) — 기획서, 확정
- [WX 피해 계산 규격서](summaries/WX_피해_계산_규격서.md) — 사양서, 확정으로 보인다
- [히트 리액션 시스템](summaries/경직-수정본.md) — 기획서, 일부 대체됨
- [그로기 시스템 초안](summaries/그로기_시스템.md) — 기획서, 초안
- [그로기 피니시 시스템 기획서](summaries/그로기_피니시_시스템_기획서.md) — 기획서, 대체가 요구됨
- [뒤잡 시스템 초안](summaries/뒤잡_시스템.md) — 기획서, 초안
- [레벨 배치용 잡몹](summaries/레벨_배치용_잡몹.md) — 사양서, 확정
- [명조 PC 시스템 역기획서](summaries/명조-PC-시스템-역기획서.md) — 역기획, 참고 자료
- [명조 적 시스템 역기획서](summaries/명조-적-시스템-역기획서.md) — 역기획, 참고 자료
- [현광 스킬 설명서 (약운)](summaries/약운.md) — 기획서, 초안
- [포션 시스템 초안](summaries/에스트병-기획서.md) — 기획서, 초안
- [적 기본 시스템 기획서](summaries/적-규격서.md) — 사양서, 일부 대체됨
- [전투 방향 변경에 따른 시스템 수정 요구사항](summaries/전투-방향-변경에-따른-시스템-수정-요구사항.md) — 사양서, 확정
- [조작키 정리](summaries/조작키.md) — 사양서, 대체됨
- [PC 스탯 명세서 초안](summaries/캐릭터-스탯-명세서.md) — 사양서, 일부 대체됨

### Docs/Programmer
- [PCG 개발 방향성](summaries/PCG-개발-방향성.md) — 기획서, 초안

### Docs/LevelDesign
- [던전 배치물 기획서](summaries/Object_Design.md) — 기획서, 일부 대체됨
- [초반 구간 퀘스트 기획서](summaries/Wx_Quest.md) — 기획서, 초안
- [레버 / 피스톤 기믹 사양서](summaries/레버_증기_사양서.md) — 사양서, 확정으로 보인다

### Docs/SystemDesign
- [핵심 전투 시스템 기획안](summaries/Core_Combat_System.md) — 기획서, 대체됨
- [WX 게임 루프](summaries/WX게임루프.md) — 사양서, 사실상 낡음

### Docs/Meeting
- [2026-08-22 회의](summaries/2026.08.22-회의.md) — 회의록, 확정
- [2026-06-13 회의](summaries/2026-06-13-meeting.md) — 회의록, 확정
- [2026-06-21 회의](summaries/2026-06-21-meeting.md) — 회의록, 확정
- [2026-07-05 회의 안건](summaries/2026-07-05-회의-안건.md) — 회의록, 확정
- [2026-07-12 회의: 프로토타입 완성 일정 제안](summaries/2026-07-12_meeting.md) — 회의록, 확정
- [2026-07-19 회의: 프로토타입 일정과 작업 주의사항](summaries/2026-07-19_meeting.md) — 회의록, 확정
- [2026-07-31 회의 안건](summaries/2026-07-31-회의-안건.md) — 회의록, 확정
- [2026-08-29 구현 작업 공유](summaries/2026-08-29-구현-작업-공유.md) — 회의록, 확정
- [2026-09-12 회의: 전투 구현 상황 공유](summaries/2026-09-12.md) — 회의록, 확정
- [2026-09-19 회의자료](summaries/2026-09-19-회의자료.md) — 회의록, 확정
- [도플갱어 동작 규칙 변경안](summaries/2026-09-26-도플갱어-수정안.md) — 기획서, 미상
- [2026-09-26 회의자료](summaries/2026-09-26-회의자료.md) — 회의록, 확정
- [게임 개발에서 AI 활용](summaries/ai_game_dev.md) — 회의록, 확정
- [기획서 구체화 요청](summaries/design_request.md) — 회의록, 확정
- [히트리액션(피격 경직) 관련 피드백](summaries/hit_react_feedback.md) — 피드백, 반영됨
- [히트리액션(피격 경직) 관련 피드백 2차](summaries/hit_react_feedback_2nd.md) — 피드백, 대부분 반영됨
- [LLM 기본 개념 잡기](summaries/LLM_lecture.md) — 조사, 확정
- [05-02 전달사항](summaries/meeting.md) — 회의록, 확정
- [던전 배치물 기획서 2차 검토 결과](summaries/object_design_feedback.md) — 피드백, 대부분 반영됨
- [Project WX 기능 문의사항 정리 (05-23)](summaries/Project_WX_기능_문의사항_05.23.md) — 회의록, 확정
- [ComboWindow 개선 요청 (06-06)](summaries/Project_WX_기능_문의사항_06.06.md) — 회의록, 확정
- [시나리오 컨셉 제안](summaries/Scenario_Woogle.md) — 기획서, 대체됨
- [시스템 기획서 검토 결과](summaries/system_design_feedback.md) — 피드백, 확정
- [WX 시나리오 구조 개편안](summaries/WX_6keys_vote_system_story.md) — 기획서, 초안
- [공성포 진지 기획서](summaries/WX_공성포진지_기획서.md) — 기획서, 초안
- [WX 오픈월드 초반 구간 개요](summaries/WX_초반구간_개요.md) — 기획서, 초안
- [WX 초반구간 사양서 검토](summaries/WX_초반구간_개요_사양서_검토.md) — 피드백, 확정
- [WX 오픈월드 초반 구간 사양서](summaries/WX_초반구간_사양서.md) — 사양서, 일부 대체됨
- [뒤잡 비교 분석 — 액션게임 레퍼런스 스터디](summaries/암살_뒤잡_비교분석.md) — 조사, 확정
- [어빌리티 시스템 규칙](summaries/어빌리티-규칙-브리핑.md) — 회의록, 확정
