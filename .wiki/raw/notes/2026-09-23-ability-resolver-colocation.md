---
title: "Ability Resolver를 Ability VM 파일에 통합"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, ui, static-review]
summary: "사용자 추가 지시로 WxUI의 Ability VM 헤더와 cpp에 Resolver를 함께 배치했다."
---

# Ability Resolver를 Ability VM 파일에 통합

2026-09-23 사용자 지시: 기존 코드 관례처럼 WxViewModel_Ability에 통합해서 옮긴다.

이는 같은 날의 모듈 이동 원자료에서 설명한 별도 Resolver 파일 배치를 대체한다. `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModel_Ability.h` 끝에 `UWxViewModelResolver_Ability`를 선언하고, `Private/MVVM/WxViewModel_Ability.cpp` 끝에 `CreateInstance`를 구현했다. Attribute·Subtitle VM과 같은 파일 배치 관례다. 클래스 이름, 동작 및 WxGame → WxUI ClassRedirect는 유지한다. 독립 Resolver 파일은 남기지 않는다.

미커밋 코드 정적 확인이며 WBP 로드·표시 실행 검증은 아니다.
