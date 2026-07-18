# GetDamageInfo 반복 제거 — FWxDamageInfo::FromDataRow 정적 팩토리

## 계획

### 목표
노티파이 3종의 `GetDamageInfo()`가 복제하던 "행 핸들 → FWxDamageInfo" 관용구를 `FWxDamageInfo::FromDataRow` 정적 팩토리로 올려 per-class 래퍼를 제거한다. 호스트를 `FWxDamageInfo`로 두는 이유: 짝이 되는 `ApplyTableRow`를 이미 소유해 같은 책임의 연장이기 때문.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxDamageInfo.{h,cpp}` | `static FromDataRow(const FDataTableRowHandle&, const TCHAR*)` 추가, 헤더에 `FDataTableRowHandle` 전방 선언 | 수정 |
| `WxAnimNotify_AreaAttack.{h,cpp}` | private `GetDamageInfo()` 제거, 호출부를 `FromDataRow` 직접 호출로, 미사용 `WxDamageTableRow.h` include 제거 | 수정 |
| `WxAnimNotify_FinisherDamage.{h,cpp}` | 동일 | 수정 |
| `WxAnimNotifyState_WeaponAttack.{h,cpp}` | 동일 | 수정 |

### 접근 방식
- **2단 API**: `ApplyTableRow`(행→자기 자신, low-level primitive)는 유지하고, 그 위에 `FromDataRow`(핸들→새 인스턴스, 없으면 기본값)를 얹는다. 팩토리 정의는 기존 관용구 그대로.
- **ApplyTableRow 유지**: 팩토리 도입 후 호출부는 `FromDataRow`와 `WxProjectileBase` 둘. Projectile은 행 없으면 스펙 캐싱을 통째로 건너뛰는 early-return이라 "행 존재 여부"를 알아야 해 항상 기본값을 주는 `FromDataRow`로 대체 불가. 그래서 통합하지 않고 primitive로 남긴다.
- **동작 변화 없음**: 3종 노티파이 결과는 이전과 동일. 순수 구조 정리.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `<파일>` | <수정 요약> | <신규·수정·삭제 명시> |

### 구현·결정과 그 이유
- **<결정>**: <왜 이렇게 했는가>

### 계획 대비 달라진 점
- <무엇이, 왜 달라졌는가> (없으면 "계획대로")

### 후속 과제
- <남은 일·미검증 항목> (없으면 "없음")
