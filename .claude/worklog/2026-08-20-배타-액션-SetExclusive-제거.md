# 배타 액션 SetExclusive() 를 제거하고 두 줄을 각 생성자에 풀어쓰기

## 계획

### 목표

같은 날 도입한 `UWxAbilityBase::SetExclusive()` 가 감추는 것은 두 줄뿐인데 대신 순서 조건(`SetAssetTags` 뒤에 불러야 하고, 어기면 마커만 조용히 빠짐)을 만들었다. 호출을 지우고 마커·차단을 각 생성자에 직접 쓴다. CDO 에 실리는 태그 집합은 그대로다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Public/AbilitySystem/Ability/WxAbilityBase.h` | `SetExclusive()` 선언·주석 제거 | 수정 |
| `Plugins/WxCombat/.../Private/AbilitySystem/Ability/WxAbilityBase.cpp` | `SetExclusive()` 정의 제거 | 수정 |
| `WxAbility_{Attack,Dodge,Guard,Ultimate}.cpp` (WxCombat) | 호출 → 마커·차단 두 줄 (자기 애셋 태그 있음) | 수정 |
| `WxAbility_{Pattern,Skill}.cpp` (WxCombat) | 호출 → 마커·차단 두 줄 (자기 애셋 태그 없음) | 수정 |
| `Source/WxGame/.../WxAbility_{Interact,UseItem}.cpp` | 동일 | 수정 |
| `Plugins/WxCore/.../Public/WxGameplayTags.h` | 태그 주석에서 `SetExclusive` 언급 정정 | 수정 |

### 접근 방식

- **자기 애셋 태그가 있는 6종은 마커를 그 컨테이너에 함께 담는다**: 이미 만들고 있는 컨테이너에 한 줄 더하므로 `Get→Add→Set` 왕복도 순서 조건도 남지 않는다. 차단 줄은 기존 취소 줄 옆에 붙여 세 축이 한자리에서 보이게 한다.
- **Pattern·Skill 은 마커만 담아 세팅한다**: 슬롯 태그가 BP 소관이라 C++ 가 세팅할 것이 마커뿐이다. `SetAssetTags` 가 컨테이너를 통째로 덮는다는 사실이 호출 자리에 드러나, BP 가 애셋 태그를 편집하면 이 마커가 닿지 않는다는 함정도 그 자리에서 읽힌다.
- **주석 이관**: 함수 주석이 안고 있던 지식 둘을 앵커로 되돌린다 — 차단에 self-exception 이 없어 자기 재발동까지 막힌다는 점은 그 성질에 기대는 Guard 로, BP 애셋 태그 덮어쓰기 함정은 Pattern·Skill 로. 전체 지도(표식·잠금·취소 표)는 이미 태그 주석에 있어 그대로 둔다.
- **예외 6종은 손대지 않는다**: 반응형 4종·Sprint·LockOn 은 지금도 필요한 줄만 직접 쓰고 있다. 이 변경으로 배타 관련 12종이 모두 같은 표기가 된다.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Ability/WxAbilityBase.h/.cpp` | `SetExclusive()` 선언·정의 제거 | 수정 |
| `WxAbility_{Attack,Dodge,Guard,Ultimate}.cpp` (WxCombat) | 마커를 기존 애셋 태그 컨테이너에 합치고 차단 줄을 직접 표기 | 수정 |
| `WxAbility_{Pattern,Skill}.cpp` (WxCombat) | 마커만 담은 컨테이너를 세팅 + 차단 줄 | 수정 |
| `Source/WxGame/.../WxAbility_{Interact,UseItem}.cpp` | 동일 | 수정 |
| `Plugins/WxCore/.../Public/WxGameplayTags.h` | 태그 주석의 `SetExclusive` 언급 정정 | 수정 |

### 구현·결정과 그 이유

- **순서 조건이 사라진 것이 가장 큰 소득**: 마커를 어빌리티가 이미 만들고 있던 컨테이너에 한 줄 더하니, 덮어쓰는 API 뒤에 불러야 한다는 조건 자체가 없어졌다. 함수가 있을 땐 그 함정을 설명하는 주석이 필요했고, 어기면 마커만 조용히 빠졌다.
- **표기가 하나로 모였다**: 반응형 4종은 원래부터 차단 줄을 직접 썼으므로, 이제 마커를 다루는 12종이 모두 같은 세 컨테이너 표기로 읽힐다. 생성자만 보면 표식·잠금·취소가 각각 있는지 바로 보이고, 헤더로 건너갈 일이 없다.
- **Pattern·Skill 은 마커만 담아 세팅한다**: `Get→Add→Set` 왜복은 생성자 시점에 부모 컨테이너가 비어 있어 무의미하다. 대신 `SetAssetTags` 가 통째로 덮는다는 사실이 호출 자리에 드러나 BP 함정 주석이 그 옆에 살아남는다.
- **Interact 의 주석을 둘로 쪼개다**: 마커와 차단이 서로 다른 자리로 갈라졌기 때문에, "남의 잠금에 걸려 표시 게이트에 반영된다"는 마커 쪽 설명과 "거는 쪽으로는 무효다"는 차단 쪽 설명을 각자의 줄 위로 나눠 붙였다.

### 계획 대비 달라진 점
- 계획대로.

### 검증
- WxEditor(Development) 빌드 성공. 경고는 이번 변경과 무관한 기존 deprecation 하나뿐이다.
- 태그 집합이 불변이라 에셋 재저장·PIE 재측은 하지 않았다.

### 후속 과제
- **다시 묶지 말 것**: 같은 날 통합(`SetExclusive`) → 부류 enum 시도·철회 → 이번 해체로 세 번 오갔다. 결론은 둘이다 — 엔진이 이미 세 컨테이너로 제공하는 것을 한 겹 우회해봐야 순서 조건이나 간접층만 늘고, 배타 여부는 생성자 세 줄을 보는 편이 빠르다. 재도입 제안 금지.
