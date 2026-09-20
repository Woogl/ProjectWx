# 작업 가이드 작성

상태: current · 범위: 작업 절차 안내 · 기준: 2026-09-20 요청한 단계 순서와 기존 운영 규약

[완료](../workflow/completion.md)한 작업을 바탕으로 기획자와 AI가 같은 작업을 수행하는 방법을 정리합니다. 대상 독자, 준비 사항, 수행 순서, 결과 확인 방법과 주의점을 설명합니다. 작업 방법 가이드는 추후 추가할 예정이며, 재사용할 절차가 없다면 새 문서를 만들 필요가 없습니다. 모듈·시스템의 구현 설명은 해당 기술 문서에서 관리합니다.

작성 전 [작성 규칙](../AGENTS.md)과 [운영 절차](../maintenance.md)를 읽습니다. 상태·검증 범위·기준을 표시하고 코드·기획 원자료를 연결합니다. 본문과 `sources.json`의 상태·확인 범위를 맞춥니다.

새 문서는 [작업 가이드](work-guides.md)에 연결하고 Wiki 변경 이력을 짧게 갱신합니다. 저장소 루트에서 `powershell -NoProfile -ExecutionPolicy Bypass -File .agents/scripts/CheckWikiLinks.ps1`을 실행하고 로컬 링크 오류를 확인하면 문서 정리를 마칩니다.

[위키 목차](../index.md)
