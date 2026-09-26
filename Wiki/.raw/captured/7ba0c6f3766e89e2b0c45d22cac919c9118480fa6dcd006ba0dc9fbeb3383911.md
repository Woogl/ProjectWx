---
title: "Ability Resolver의 WxUI 소유와 이전 경로 호환"
source: "MANUAL"
type: notes
ingested: 2026-09-23
tags: [wx, ui, static-review]
summary: "Ability Resolver를 WxUI로 이동하고 기존 WxGame 클래스 경로를 ClassRedirect로 유지했다."
---

# Ability Resolver의 WxUI 소유와 이전 경로 호환

2026-09-23 사용자 요청에 따른 미커밋 작업 트리 정적 확인이다.

- `Plugins/WxUI/Source/WxUI/Public/MVVM/WxViewModelResolver_Ability.h`: `WXUI_API`를 사용하는 Resolver 선언과 AbilityTags 속성.
- `Plugins/WxUI/Source/WxUI/Private/MVVM/WxViewModelResolver_Ability.cpp`: 위젯 소유 PC의 Pawn에서 IAbilitySystemInterface로 ASC를 얻는다. ASC가 없거나 AbilityTags가 비면 nullptr, 그 외에는 AbilitySystem VM의 슬롯 캐시를 사용한다.
- `Plugins/WxUI/Source/WxUI/WxUI.Build.cs`: GameplayAbilities, GameplayTags, ModelViewViewModel 의존성이 이미 등록되어 추가 의존성이 필요하지 않았다.
- `Config/DefaultEngine.ini`의 CoreRedirects: `/Script/WxGame.WxViewModelResolver_Ability` → `/Script/WxUI.WxViewModelResolver_Ability`.

클래스 이동으로 동작은 바꾸지 않았다. 기존 WBP의 로드와 표시 동작은 이 정적 조사에서 확인하지 않았다. 빌드 결과는 [작업 기록](../../../.agents/workflow/tasks/ability-resolver-to-wxui.md)에서 관리한다.
