---
title: "장치 상태 태그는 루트 StateTree 에셋에서만 발행"
source: "MANUAL"
type: notes
ingested: 2026-09-24
tags: [wx, world, statetree]
summary: "UWxDeviceStateTreeComponent::FindActiveStateTag가 링크된 StateTree 에셋의 활성 상태 태그를 건너뛰게 해, 서버가 받는 쪽이 찾을 수 없는 태그를 발행하지 않는다. 대기 등록부의 중첩 구조체 이름 변경(FWxWait)은 동작 변화가 없다."
---

# 장치 상태 태그는 루트 StateTree 에셋에서만 발행

2026-09-23 커밋 `67d288fc5`·`6f452b728`(WxWorld 모듈 리뷰와 같은 세션). HEAD `ca84c9aac`에서 코드를 다시 읽어 대조했다.

## 링크된 에셋의 태그를 발행하지 않는다 (`67d288fc5`)

- `FindActiveStateTag`는 활성 프레임을 깊은 쪽부터 훑어 태그가 있는 첫 활성 상태를 고른다. 이 태그를 `PublishState`가 스냅샷으로 발행하고, 복원 수렴·실시간 전이 확인(`FindActiveStateTag() == AuthorityTag`)에도 쓴다.
- 받는 쪽 `EnterState`는 루트 에셋(`StateTreeRef`)에서만 태그로 상태를 찾는다(`HasState`도 루트 에셋 기준). 그런데 링크된 StateTree 에셋의 상태가 더 깊이 활성이면 그 태그가 발행되어, 클라이언트가 찾을 수 없는 태그를 받았다.
- 수정: 프레임의 `StateTree`가 루트 에셋이 아니면 건너뛴다. 링크된 에셋 안의 상태 변화는 장치 상태로 복제되지 않고, 루트 에셋의 활성 상태 중 태그가 있는 가장 깊은 상태의 태그가 발행된다.
- 빌드·인게임 확인 기록은 커밋에 없다. 링크된 StateTree를 쓰는 장치 에셋이 있는지는 확인하지 않았다.

## 대기 등록부 중첩 구조체 이름 (`6f452b728`)

`TWxStateTreeWaitRegistry`의 중첩 구조체 `FWait`를 규칙 1(Wx 접두사)에 맞춰 `FWxWait`로 바꿨다. 동작 변화는 없다.

근거: [장치 StateTree 컴포넌트](../../../Plugins/WxWorld/Source/WxWorld/Private/Device/WxDeviceStateTreeComponent.cpp), [대기 등록부](../../../Plugins/WxWorld/Source/WxWorld/Public/StateTreeTask/WxStateTreeWaitRegistry.h), [WxWorld 리뷰](../../../.agents/workflow/tasks/module_review_WxWorld.md).
