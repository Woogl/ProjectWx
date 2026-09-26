---
type: meta
title: Hot Cache
status: developing
created: 2026-09-26
updated: 2026-09-26
tags:
  - meta
  - hot-cache
---

# Recent Context

## Last Updated

2026-09-26 · ingest-20260926T055046Z-b06

## Key Recent Facts

- [[결정 노트 - 2026-09-22-current-workflow]]: 2026-09-22 시점 AGENTS.md·옛 LLM Wiki 설정·Workflow 절차와 실행 스크립트의 계약을 발췌한 정적 조사 노트로 사람 판단·AI 실행 경계를 기록한다
- [[결정 노트 - 2026-09-22-verified-stock-rule]]: 옛 LLM Wiki 기사의 verified 필드를 WX 전용 인간 확인 방침에서 순정 규칙(편찬·재확인 날짜 기록)으로 되돌린 2026-09-22 사용자 결정 기록
- [[결정 노트 - 2026-09-22-workflow-closure]]: Workflow 실행기에 승인 코드 버전 대조, 테스트 수용과 정리 완료 분리, 승인자 이름 기록을 구현한 2026-09-22 요청·구현·모의 테스트 범위 기록
- [[결정 노트 - 2026-09-22-workflow-dashboard]]: Workflow 작업 현황 대시보드에서 단계별 진행 일감을 한눈에 보도록 한 2026-09-22 사용자 요청과 workflow.js 구현·모의 DOM 검증 범위 기록
- [[결정 노트 - 2026-09-22-workflow-review]]: 기획자에게 툴 난도가 높다는 이유로 Workflow 기획 단계를 기획서 검토 단계로 바꾸고 Workflow 문서 검색을 제거한 2026-09-22 사용자 결정 기록
- [[결정 노트 - 2026-09-23-ability-resolver-module]]: Ability ViewModel Resolver를 WxGame에서 WxUI로 옮기고 CoreRedirects로 기존 클래스 경로를 호환시킨 2026-09-23 정적 확인 기록
- [[결정 노트 - 2026-09-23-apply-damage-unification]]: bool ApplyDamage와 ApplyDamageWithResult를 FWxDamageResult 반환 ApplyDamage 하나로 합친 2026-09-23 사용자 요청과 호출처 검색 기록
- [[결정 노트 - 2026-09-23-boss-battle-three-layer]]: 보스 표시를 UWxBattleSubsystem·WxGame 리졸버·WxUI Character VM 세 층으로 재구성하고 보스 식별을 IdentityTags로 바꾼 2026-09-23 결정
- [[결정 노트 - 2026-09-23-damage-context-cleanup]]: Damage EffectContext에서 소비자가 없는 테이블 참조 저장·복제를 없애고 중복 피해 수치를 FWxDamageResult로 합친 2026-09-23 사용처 조사와 변경 기록
- [[결정 노트 - 2026-09-23-damage-forward-flow]]: Hit Wrapper GE와 전용 EffectContext를 없애고 ApplyDamage 판정에서 Damage GE 컴포넌트 반응으로 결과가 앞으로만 흐르게 한 2026-09-23 결정들
- [[결정 노트 - 2026-09-23-damage-four-arguments]]: 사용자 승인으로 FWxDamageRequest를 없애고 ApplyDamage를 Causer·Target·피해 행·HitResult 네 인자로 되돌려 출처·레벨 추론을 복원한 기록

## Recent Changes

- [[결정 노트 - 2026-09-22-current-workflow]]
- [[결정 노트 - 2026-09-22-verified-stock-rule]]
- [[결정 노트 - 2026-09-22-workflow-closure]]
- [[결정 노트 - 2026-09-22-workflow-dashboard]]
- [[결정 노트 - 2026-09-22-workflow-review]]
- [[결정 노트 - 2026-09-23-ability-resolver-module]]
- [[결정 노트 - 2026-09-23-apply-damage-unification]]
- [[결정 노트 - 2026-09-23-boss-battle-three-layer]]
- [[결정 노트 - 2026-09-23-damage-context-cleanup]]
- [[결정 노트 - 2026-09-23-damage-forward-flow]]
- [[결정 노트 - 2026-09-23-damage-four-arguments]]

## Active Threads

- 정기 갱신 Routine이 매일 06:30(KST) 기획서·완료 작업 기록의 바뀐 것을 수집한다.
- 주제 페이지의 구현 관찰은 원자료 작성 시점 기준이며, 게임 동작 검증이 아니다.
