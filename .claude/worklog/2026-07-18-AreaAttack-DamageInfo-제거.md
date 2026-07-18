# UWxAnimNotify_AreaAttack — DamageInfo 제거, DamageDataRow 단일화

## 계획

### 목표
`UWxAnimNotify_AreaAttack`의 인라인 대미지 소스 `FWxDamageInfo DamageInfo`를 제거하고, 대미지 소스를 `DamageDataRow` 하나로 통일한다. `WxAnimNotify_FinisherDamage`·`WxProjectileBase`가 이미 걸어간 테이블 행 단일화 방향을 따른다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Public/AnimNotify/WxAnimNotify_AreaAttack.h` | `DamageInfo` 멤버 제거, `CanEditChange` 오버라이드 선언 제거, `DamageDataRow`/클래스 독 코멘트 갱신 | 수정 |
| `Plugins/WxCombat/.../Private/AnimNotify/WxAnimNotify_AreaAttack.cpp` | `CanEditChange` 정의 제거, `ResolveDamageInfo()`를 행 없으면 기본값 반환 형태로 재작성 | 수정 |

### 접근 방식
- **FinisherDamage 패턴 재사용**: 멤버는 `DamageDataRow`만 남기고, `ResolveDamageInfo()`는 기본 `FWxDamageInfo`를 만들어 행이 있으면 `ApplyTableRow`, 없으면 그대로 반환. 인라인 필드 게이팅 전용이던 `CanEditChange`는 존재 이유가 사라져 제거. `#include "WxDamageInfo.h"`는 반환형 때문에 유지.
- **동작 변화**: 행 미설정/누락 시 기존엔 인라인 `DamageInfo`로 폴백했으나, 이후 기본값(빈 스펙 → 무피해) 반환. Finisher/Projectile 이관과 동일 계약.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `Plugins/WxCombat/.../Public/AnimNotify/WxAnimNotify_AreaAttack.h` | `DamageInfo` 멤버·`CanEditChange` 선언 제거, `DamageDataRow`/클래스 독 코멘트 갱신 | 수정 |
| `Plugins/WxCombat/.../Private/AnimNotify/WxAnimNotify_AreaAttack.cpp` | `CanEditChange` 정의 제거, 리졸버를 `GetDamageInfo()`로 개명하고 행 없으면 기본값 반환 형태로 재작성 | 수정 |
| `WxAnimNotify_FinisherDamage.{h,cpp}`, `WxAnimNotifyState_WeaponAttack.{h,cpp}` | 동일 패턴의 `ResolveDamageInfo` → `GetDamageInfo` 개명(본문 지역변수 포함) | 수정 |

### 구현·결정과 그 이유
- **행 단일화**: 대미지 소스가 인라인 값/테이블 행 두 갈래라 `CanEditChange`로 한쪽을 죽여 에디터 상태를 관리해야 했다. 소스를 행 하나로 줄이면 이 게이팅 자체가 불필요해져 오버라이드까지 함께 제거했다. Finisher/Projectile 이관과 동일한 형태로 맞춰 코드 패턴을 통일.
- **`ResolveDamageInfo` → `GetDamageInfo`**: 두 소스 중 하나를 고르는 폴백 분기가 사라져 더 이상 "resolve"할 게 없다. 이름을 단순 게터로 낮춰 의미를 맞춤. FinisherDamage·WeaponAttack의 동명 함수도 이미 폴백 없는 같은 형태라 함께 개명해 코드베이스 어휘를 통일.
- **`WxDamageInfo.h` include 유지**: `ResolveDamageInfo()` 반환형이 `FWxDamageInfo`라 헤더 의존이 남는다.

### 계획 대비 달라진 점
- 계획대로.

### 후속 과제
- 없음. (WxEditor Development 빌드 성공으로 컴파일 검증 완료)
