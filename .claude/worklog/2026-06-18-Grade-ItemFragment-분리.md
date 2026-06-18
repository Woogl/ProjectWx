# Grade를 ItemFragment로 분리 + 등급 색상 Fragment 이관

## 계획

### 목표
아이템 등급을 `UWxItemDefinition`의 직접 필드에서 `UWxItemFragment_Grade`로 분리하고, DeveloperSettings의 등급별 색상 맵을 이 Fragment로 이관한다. Fragment가 `Grade` + `Color`(등급별 기본 색 시드 + 기획자 오버라이드)를 함께 들어, 등급 데이터가 한 Fragment에 응집되고 향후 확장(드롭 가중치 등)이 쉬워진다.

### 수정 범위
| 파일 | 수정할 내용 | 구분 |
|---|---|---|
| `WxInventory/.../Items/WxItemFragment.h/.cpp` | `EWxItemGrade` enum 이관 + `UWxItemFragment_Grade`(Grade/Color/`GetDefaultColorForGrade`/ctor 시드/`PostEditChangeProperty` 재시드) | 수정 |
| `WxInventory/.../Items/WxItemDefinition.h/.cpp` | `EWxItemGrade` enum·`Grade` 필드·ctor 이니셜라이저 제거(Category 유지) | 수정 |
| `WxInventory/.../System/WxInventoryDeveloperSettings.h/.cpp` | 삭제(등급 색상만 보유) | 삭제 |
| `WxGame/MVVM/WxViewModel_Item.h/.cpp` | DeveloperSettings include 제거, Grade/GradeColor를 Fragment 조회로; `.h` enum include 교체 | 수정 |
| `WxInventory/README.md` | DeveloperSettings 색상 언급을 Grade Fragment로 갱신 | 수정 |

### 접근 방식
- **등급 데이터를 Fragment에 응집**: 다른 Fragment(Charges/Equippable)와 동일하게 `FindFragmentByClass<UWxItemFragment_Grade>()`로 소비. 헬퍼 신설 없이 기존 패턴 유지. Fragment 부재 = Common 등급/Common 색 폴백.
- **색상 시드 UX**: `GetDefaultColorForGrade` 정적 팔레트(DeveloperSettings에서 verbatim 이관) + ctor 시드 + `PostEditChangeProperty`에서 Grade 변경 시 Color 재시드. "등급 기본 색 자동 채움 + 기획자 오버라이드" 충족.
- **enum 이관**: `EWxItemGrade`를 Fragment.h로 옮겨 Grade 개념과 colocate. reflected 이름 불변이라 BP/에셋 무해. DeveloperSettings 삭제로 ViewModel만 include 교체.

---

## 완료

### 수정한 파일
| 파일 | 수정한 내용 | 구분 |
|---|---|---|
| `WxInventory/.../Items/WxItemFragment.h/.cpp` | `EWxItemGrade` enum 이관; `UWxItemFragment_Grade`(Grade/Color + 정적 `GetDefaultColorForGrade` 팔레트 + 생성자 시드 + `PostEditChangeProperty` 재시드) 추가 | 수정 |
| `WxInventory/.../Items/WxItemDefinition.h/.cpp` | `EWxItemGrade`·`Grade` 필드·생성자 이니셜라이저 제거(Category 유지) | 수정 |
| `WxInventory/.../System/WxInventoryDeveloperSettings.h/.cpp` | 삭제(등급 색상만 보유, 다른 사용처·.ini 없음) | 삭제 |
| `WxGame/MVVM/WxViewModel_Item.h/.cpp` | DeveloperSettings include 제거; Grade/GradeColor를 `FindFragmentByClass<UWxItemFragment_Grade>()` 조회로(부재 시 Common 폴백); `.h`에 `WxItemFragment.h` include 추가·주석 갱신 | 수정 |
| `WxInventory/README.md` | DeveloperSettings 등급 색상 언급 2곳을 `UWxItemFragment_Grade`로 갱신 | 수정 |

### 구현·결정과 그 이유
- **등급 데이터를 Fragment에 응집**: Grade와 표시 Color를 한 Fragment(`UWxItemFragment_Grade`)에 모아, 향후 등급 관련 데이터(드롭 가중치/VFX 등)를 필드 추가로 확장할 수 있게 했다. 소비는 다른 Fragment와 동일하게 `FindFragmentByClass`로 통일(헬퍼 신설 없음).
- **기본 색 팔레트는 프로그래머 전용·C++ 생성자 시드(사용자 지시)**: 등급별 기본색을 정적 `GetDefaultColorForGrade`(에디터 비노출)에 두고 생성자가 그 값으로 `Color`를 시드한다. 생성자는 CDO 기본 등급(Common)만 보므로, 에디터에서 Grade 변경 시 `PostEditChangeProperty`가 해당 등급 기본값으로 `Color`를 재시드해 "등급별 기본색 자동 채움"을 완성한다. `Color`는 `EditDefaultsOnly`라 기획자가 아이템별 오버라이드 가능.
- **DeveloperSettings 삭제**: 전역 등급 색상 맵만 들고 있던 클래스라, 색상이 Fragment로 가면서 완전히 비어 삭제했다. 소비처(ViewModel)는 Fragment 조회로 전환.
- **enum 이관**: `EWxItemGrade`를 Grade 개념과 colocate하기 위해 `WxItemFragment.h`로 옮겼다. reflected 이름 불변이라 BP/에셋 무해. ViewModel.h는 enum을 위해 `WxItemFragment.h`를 추가 include(기존 Definition.h 유지로 전이 인클루드 안전).

### 계획 대비 달라진 점
- 색 팔레트의 소유를 "프로그래머 전용·생성자 시드"로 명확히 함(구현 중 사용자 지시 반영). 계획의 `GetDefaultColorForGrade`+생성자+`PostEditChangeProperty` 구조가 이를 그대로 충족.

### 후속 과제
- **기존 ItemDefinition 에셋에 Grade Fragment 추가(사용자, 데이터)**: `Grade` 필드 제거로 기존 정의 에셋은 등급을 잃는다. 각 에셋의 `Fragments`에 `Grade` Fragment를 추가하고 등급을 지정(Color 자동 시드)해야 한다. Content에 ItemDefinition 데이터 에셋이 거의/전혀 없어 보여 부담은 작을 것.
- **검증 미완**: 컴파일만 확인. 에디터에서 Grade 드롭다운→Color 자동 시드·오버라이드 유지, WBP_ItemSlot 등급 색 표시 확인 필요.
