# WX GAS 구조 학습 정리

## 공부 목적

현재 WX는 GAS 기반으로 개발되고 있다.

이 문서는 PC와 적이 어떤 방식으로 관리되고, 어떤 방식으로 액션이 실행되는지 이해하기 위해 작성한다. 일반적인 언리얼 GAS 설명이 아니라, 현재 WX 프로젝트에서 사용하는 구조를 기준으로 정리한다.

---

## 목차

0. BP와 인스턴스
1. 컴포넌트
2. 캐릭터 BP
3. Character Movement Component
4. ABP
5. State Machine
6. ASC
7. ABS
8. AttributeSet
9. GA
10. GE
11. AM
12. AS
13. PC와 적의 액션 실행 흐름
14. 전체 관계 요약

---

## 전체 흐름 요약

WX의 액션 실행 흐름은 크게 다음과 같이 볼 수 있다.

```text
입력 또는 AI 판단
→ Character BP에서 관리하는 입력 / BT 흐름
→ ASC에 GA 실행 요청
→ GA 실행
→ GA에서 코스트 / 쿨타임 GE 처리
→ GA에서 AM 재생
→ AM의 Notify 타이밍에 판정 발생
→ AM이 가진 데미지 정보 또는 데미지 테이블 기준으로 데미지 처리
→ 대상 Attribute 값 변경
→ ABP가 상태 값을 바탕으로 최종 애니메이션 출력
```

핵심 관계는 다음과 같다.

```text
BP = 설계도
Instance = 설계도로부터 생성된 실제 객체
Component = Actor에 붙는 기능 부품
Character BP = 캐릭터의 기본 구성과 참조 정보
BT = 적 AI의 판단 흐름, WX에서는 사용할 BT를 BP에서 관리
ASC = GAS 기능을 실제로 실행하고 관리하는 컴포넌트
ABS = 캐릭터에게 줄 GA와 Attribute 데이터를 관리하는 묶음
AttributeSet = 런타임 스탯 값을 저장하는 구조
GA = 스킬 / 액션의 실행 규칙과 흐름
GE = GA에서 코스트 / 쿨타임 / 효과 처리를 위해 사용하는 클래스
ABP = 최종 애니메이션 포즈 계산
AM = 행동 애니메이션, Notify, 데미지 데이터 관리
AS = 실제 애니메이션 원본 데이터
```

---

## 0. BP와 인스턴스

BP는 오브젝트나 액터의 설계도이다.

레벨에 배치되거나 런타임에 Spawn되면 BP로부터 실제 인스턴스가 생성된다. 같은 BP를 사용해 여러 캐릭터를 배치하더라도, 각각은 독립된 인스턴스로 존재한다.

예시:

```text
BP_Enemy = 설계도

Enemy_01 = BP_Enemy로부터 생성된 인스턴스
Enemy_02 = BP_Enemy로부터 생성된 인스턴스
Enemy_03 = BP_Enemy로부터 생성된 인스턴스
```

각 인스턴스는 자기만의 변수, 컴포넌트, 상태 값을 가진다. 따라서 같은 BP에서 만들어졌더라도 HP, 위치, Velocity, IsFalling, 보유 GA, 쿨타임 상태는 각자 따로 관리된다.

---

## 1. 컴포넌트

컴포넌트는 Actor에 붙는 기능 부품이다.

정확히는 특정 기능과 데이터를 담당하는 Actor의 하위 객체라고 볼 수 있다.

예시:

```text
Character BP
- Capsule Component
  → 충돌 범위 담당

- Skeletal Mesh Component
  → 캐릭터 모델과 애니메이션 출력 담당

- Character Movement Component
  → 이동, 점프, 낙하, 속도, 가속도 담당

- Ability System Component
  → GA, GE, Gameplay Tag, Attribute 관리 담당
```

컴포넌트도 인스턴스로 작동한다. 같은 `BP_Enemy`로 여러 적이 생성되면 각 적은 자기만의 Movement Component와 ASC를 가진다.

```text
Enemy_01
- CharacterMovement_01
- ASC_01

Enemy_02
- CharacterMovement_02
- ASC_02
```

즉 한 적의 `IsFalling`이나 쿨타임 상태가 다른 적에게 공유되지 않는다.

---

## 2. 캐릭터 BP

캐릭터 BP는 레벨에 배치될 캐릭터의 기본 구성과 참조 정보를 가진다.

WX에서는 캐릭터 BP에서 다음 요소를 포함하거나 관리한다.

- Skeletal Mesh
- Capsule Component
- Character Movement Component
- ASC
- ABP 설정
- ABS 참조
- 사용할 BT 정보
- 입력 처리 또는 액션 실행 요청

PC와 적은 액션을 시작하는 출발점이 다르다.

PC는 플레이어 입력이 출발점이다.

```text
플레이어 입력
→ Character BP 또는 입력 시스템
→ ASC에 GA 실행 요청
```

적은 AI 판단이 출발점이다. WX에서는 사용할 BT를 BP에서 관리한다.

```text
Character BP에서 사용할 BT 관리
→ BT 흐름에 따라 AI 판단
→ ASC에 GA 실행 요청
```

정리하면 캐릭터 BP는 캐릭터의 몸체와 참조 정보를 관리하고, 실제 GAS 실행은 ASC가 담당한다.

---

## 3. Character Movement Component

Character Movement Component는 캐릭터의 이동 관련 상태를 계산하고 관리하는 컴포넌트이다.

대표적으로 다음 값을 제공한다.

- Velocity
- Acceleration
- IsFalling
- IsCrouching
- Movement Mode
- Ground Speed 계산에 필요한 이동 속도

ABP는 Character Movement Component에서 값을 읽어 애니메이션 전환 조건으로 사용한다.

예시:

```text
IsFalling == true
→ ABP State Machine에서 Jump 또는 Fall Loop 상태로 전환

GroundSpeed > 0
→ Locomotion에서 Idle이 아니라 Walk / Run 애니메이션 출력
```

중요한 점은 Character Movement Component가 실제 이동 상태를 계산하고, ABP는 그 결과를 읽어 애니메이션을 바꾼다는 점이다.

---

## 4. ABP

ABP는 Animation Blueprint이다.

Skeletal Mesh의 최종 애니메이션 포즈를 계산하는 애니메이션용 Blueprint라고 이해하면 된다. 단순히 애니메이션 하나를 재생하는 에셋이 아니라, 현재 캐릭터 상태를 보고 어떤 애니메이션을 어떤 방식으로 섞어서 출력할지 결정한다.

ABP는 다음을 담당한다.

- State Machine 관리
- BlendSpace 재생
- Montage Slot 처리
- 이동, 점프, 낙하, 착지 상태 전환
- 캐릭터 변수 기반 애니메이션 전환
- 매 프레임 최종 Pose 출력

### ABP와 AnimInstance

ABP는 설계도에 가깝고, AnimInstance는 실행 중 생성되는 실제 인스턴스이다.

```text
ABP = 애니메이션 설계도
AnimInstance = 실행 중 Skeletal Mesh Component에 생성된 실제 애니메이션 인스턴스
```

같은 ABP를 쓰더라도 캐릭터마다 AnimInstance는 따로 생성된다. 따라서 각 캐릭터의 애니메이션 상태도 독립적으로 작동한다.

---

## 5. State Machine

State Machine은 ABP 내부에서 애니메이션 상태를 관리하는 구조이다.

예시 상태:

```text
Locomotion
Jump
Fall Loop
Land
```

각 상태는 특정 애니메이션 또는 BlendSpace를 재생한다. 상태 사이에는 전환 규칙이 존재한다.

예시:

```text
Locomotion → Jump
조건: IsFalling == true AND Velocity.Z > 100

Jump → Fall Loop
조건: Jump 애니메이션이 끝났거나 자동 전환 조건 만족

Fall Loop → Land
조건: IsFalling == false

Land → Locomotion
조건: Land 애니메이션 종료
```

State Machine은 실제 이동을 만드는 기능이 아니다. 실제 이동 상태는 Character Movement Component가 계산하고, State Machine은 그 값을 읽어 애니메이션 상태를 전환한다.

---

## 6. ASC

ASC는 Ability System Component이다.

WX에서 ASC는 GAS 기능을 실제로 실행하고 관리하는 컴포넌트이다. GA를 보유하고 실행하며, GE 적용, Gameplay Tag, Attribute 변경 흐름을 관리한다.

ASC는 다음을 담당한다.

- GA 보유
- GA 실행
- GE 적용
- Gameplay Tag 관리
- AttributeSet과 연동
- 코스트와 쿨타임 처리 흐름 관리
- 버프, 디버프 상태 관리

정리:

```text
ASC = GA, GE, Attribute, Gameplay Tag를 실제로 관리하는 컴포넌트
```

ASC는 클래스이면서, 캐릭터 인스턴스에 붙으면 실제 컴포넌트 인스턴스로 작동한다.

```text
UAbilitySystemComponent = ASC의 C++ 클래스
BP_PlayerCharacter 안의 ASC = 이 캐릭터가 ASC를 가진다는 구성 정보
PlayerCharacter_01의 ASC = 실제 실행 중인 ASC 인스턴스
```

---

## 7. ABS

ABS는 Ability Set이다.

WX에서는 ABS가 캐릭터에게 부여할 GA와 Attribute 데이터를 함께 관리한다. 특히 Attribute는 테이블 기반으로 조절할 수 있다.

ABS가 관리하는 것:

- 캐릭터가 사용할 GA 목록
- 캐릭터의 Attribute 데이터
- Attribute 테이블
- 필요 시 GE 참조
- 입력 태그 또는 어빌리티 태그

예시:

```text
ABS_Player
- GA_LightAttack
- GA_HeavyAttack
- GA_Dodge
- GA_Guard
- GA_Skill01
- DT_PlayerAttribute
```

중요한 구분:

```text
ABS = 어떤 능력과 Attribute 데이터를 캐릭터에게 줄지 정하는 묶음
ASC = ABS를 통해 받은 GA와 Attribute 흐름을 실제로 관리하고 실행하는 컴포넌트
```

따라서 `ASC는 ABS에 있는 기능을 실행시키기 위한 컴포넌트인가?`라고 보면 거의 맞지만, 더 정확히는 다음과 같다.

```text
ABS = 지급 목록 / 세팅 묶음
ASC = 지급받은 능력을 실제로 보유하고 실행하는 컴포넌트
```

---

## 8. AttributeSet

AttributeSet은 GAS에서 스탯 값을 저장하는 구조이다.

예시 Attribute:

- HP
- MaxHP
- MP
- Stamina
- AttackPower
- Defense
- MoveSpeed
- CriticalRate

WX에서는 Attribute의 기준 데이터와 조절값을 ABS 안의 테이블로 관리한다.

흐름:

```text
캐릭터 생성
→ ABS 확인
→ ABS 안의 Attribute 테이블 확인
→ 초기 Attribute 값 세팅
→ ASC가 런타임 Attribute 변경 흐름 관리
```

정리:

```text
ABS = Attribute 기준 데이터 관리
Data Table = Attribute 값을 조절하는 데이터
AttributeSet = 런타임에서 실제 스탯 값을 저장하는 구조
ASC = Attribute 변경 흐름을 관리하는 컴포넌트
```

따라서 WX 기준으로는 “ABS에서 Attribute를 관리한다”고 정리할 수 있다. 다만 런타임에서 실제 값이 존재하고 바뀌는 장소는 AttributeSet으로 이해하면 된다.

---

## 9. GA

GA는 Gameplay Ability이다.

일반적으로 스킬이나 액션이라고 생각되는 행동을 관리한다.

예시:

- 기본 공격
- 강공격
- 회피
- 방어
- 점프 공격
- 스킬
- 궁극기
- 피격 반응

WX에서 GA는 다음을 담당한다.

- 발동 조건 검사
- 코스트 확인
- 쿨타임 확인
- 코스트 GE 처리
- 쿨타임 GE 처리
- Gameplay Tag 조건 확인
- 실행 중 차단할 GA 관리
- 실행 중 허용할 GA 관리
- AM 재생
- 액션 종료 또는 취소 처리

WX에서는 GE를 클래스화하여 GA가 코스트와 쿨타임 등을 관리한다. 또한 테이블을 통해 코스트와 쿨타임 값을 조절할 수 있다.

정리:

```text
GA = 행동의 규칙과 실행 흐름을 관리한다
GA = 코스트 / 쿨타임 GE를 사용해 발동 비용과 재사용 제한을 처리한다
```

예시 흐름:

```text
GA_Attack 실행
→ 공격 가능한 상태인지 확인
→ 테이블 값을 기준으로 코스트 확인
→ 테이블 값을 기준으로 쿨타임 확인
→ 코스트 GE 처리
→ 쿨타임 GE 처리
→ AM_Attack 재생
→ AM의 Notify 타이밍에 타격 판정
→ AM의 데미지 테이블 기준으로 데미지 처리
→ 공격 종료
```

---

## 10. GE

GE는 Gameplay Effect이다.

WX에서는 GE를 클래스화해서 GA에서 코스트, 쿨타임 등을 관리하는 데 사용한다. 코스트와 쿨타임 수치는 테이블로 조절할 수 있다.

GE가 사용되는 예:

- 코스트 소모
- 쿨타임 부여
- 버프
- 디버프
- 스탯 증가
- 스탯 감소
- Gameplay Tag 부여

정리:

```text
GE = GA에서 코스트 / 쿨타임 / 효과 적용을 처리하기 위해 사용하는 클래스
```

WX 기준 예시:

```text
GA_Skill
→ 테이블에서 코스트 값 확인
→ Cost GE 처리
→ 테이블에서 쿨타임 값 확인
→ Cooldown GE 처리
→ AM_Skill 재생
```

주의할 점:

```text
일반 GAS에서는 데미지도 GE로 처리하는 경우가 많다.
WX에서는 데미지를 AM과 데미지 테이블에서 관리한다.
```

따라서 WX 문서에서는 데미지 설명을 GE 중심이 아니라 AM 중심으로 정리한다.

---

## 11. AM

AM은 Animation Montage이다.

WX에서는 GA에서 공격, 회피, 스킬 같은 행동 애니메이션을 재생할 때 사용한다. 또한 AM에서 데미지를 관리하며, 데미지 테이블로 수치를 조절할 수 있다.

AM은 다음을 담당한다.

- 여러 AS를 하나의 행동 단위로 묶기
- Section을 통한 애니메이션 구간 관리
- Slot을 통한 재생 위치 관리
- Notify를 통한 특정 타이밍 이벤트 발생
- Root Motion 애니메이션 재생
- GA와 연동되는 액션 애니메이션 재생
- 데미지 데이터 관리
- 데미지 테이블을 통한 데미지 값 조절

예시:

```text
AM_Attack
- Section: Attack_01
- Section: Attack_02
- Section: Attack_03
- Notify: HitCheck
- NotifyState: WeaponTraceOn / WeaponTraceOff
- DamageTable: DT_AttackDamage
```

흐름:

```text
GA_Attack
→ AM_Attack 재생
→ AM_Attack의 Notify 타이밍에 HitCheck 발생
→ AM에 연결된 데미지 테이블 확인
→ 대상에게 데미지 적용
```

정리:

```text
AM = 행동 애니메이션의 재생, 타이밍, 데미지 데이터를 관리하는 에셋
```

단, AM이 데미지를 관리하더라도 발동 가능 여부, 코스트, 쿨타임, 태그 차단 같은 규칙은 GA와 ASC 쪽 책임으로 구분한다.

---

## 12. AS

AS는 Animation Sequence이다.

실제 애니메이션 원본 데이터이다. 캐릭터 뼈의 위치, 회전, 스케일 키프레임을 가지고 있다.

AS는 다음에서 사용된다.

- AM의 재료
- ABP State Machine의 상태 애니메이션
- BlendSpace의 입력 애니메이션

정리:

```text
AS = 실제 애니메이션 원본 데이터
AM = AS를 행동 단위로 묶고 제어하는 에셋
ABP = AS, BlendSpace, AM Slot 등을 조합해 최종 포즈를 만드는 시스템
```

예시:

```text
AS_Attack_01
AS_Attack_02
AS_Attack_03
→ AM_AttackCombo를 구성하는 재료
```

---

## 13. PC와 적의 액션 실행 흐름

PC와 적은 둘 다 GAS를 사용할 수 있다. 차이는 액션을 시작시키는 출발점이다.

### PC

PC는 플레이어 입력이 출발점이다.

```text
플레이어 입력
→ Input Action
→ Character BP 또는 Player Controller
→ ASC에 GA 실행 요청
→ GA 실행
→ 코스트 / 쿨타임 GE 처리
→ AM 재생
→ Notify 타이밍에 판정
→ AM의 데미지 테이블 기준으로 데미지 처리
```

### 적

적은 AI 판단이 출발점이다. WX에서는 사용할 BT를 BP에서 관리한다.

```text
Character BP에서 사용할 BT 관리
→ BT 흐름에 따라 행동 판단
→ ASC에 GA 실행 요청
→ GA 실행
→ 코스트 / 쿨타임 GE 처리
→ AM 재생
→ Notify 타이밍에 판정
→ AM의 데미지 테이블 기준으로 데미지 처리
```

GA가 실행된 뒤의 구조는 PC와 적이 비슷하다.

```text
GA 실행
→ GE로 코스트 / 쿨타임 처리
→ AM 재생
→ AM Notify 타이밍
→ AM의 데미지 테이블 기준으로 데미지 처리
→ Attribute 변경
```

---

## 14. 전체 관계 요약

WX의 구조를 GAS 기준으로 보면 다음처럼 이해할 수 있다.

```text
캐릭터 BP
= 캐릭터의 몸체, 컴포넌트, 참조 정보를 관리한다.

BT
= 적 AI의 판단 흐름이다. WX에서는 사용할 BT를 BP에서 관리한다.

ASC
= GAS 실행의 중심 컴포넌트이다.

ABS
= 캐릭터에게 줄 GA와 Attribute 데이터를 관리한다.

AttributeSet
= 런타임에서 HP, MP 같은 실제 스탯 값을 저장한다.

GA
= 스킬과 액션의 실행 흐름을 관리한다.

GE
= GA에서 코스트, 쿨타임, 효과 처리를 위해 사용하는 클래스이다.

ABP
= 캐릭터 상태를 보고 최종 애니메이션 포즈를 만든다.

AM
= GA에서 실행되는 행동 애니메이션, Notify 타이밍, 데미지 데이터를 담당한다.

AS
= 실제 애니메이션 원본 데이터이다.
```

가장 중요한 구분은 다음이다.

```text
이동 상태 계산 = Character Movement Component
애니메이션 상태 결정 = ABP / State Machine
GAS 실행 중심 = ASC
캐릭터 능력과 Attribute 데이터 관리 = ABS
런타임 스탯 저장 = AttributeSet
스킬 실행 규칙 = GA
코스트 / 쿨타임 처리 = GA + GE
행동 애니메이션 재생 = AM
데미지 데이터 관리 = AM + 데미지 테이블
원본 애니메이션 데이터 = AS
적 AI 판단 흐름 = BP에서 관리하는 BT
```

---

## 한 줄 요약

```text
WX에서 Character BP는 캐릭터의 몸체와 참조 정보를 관리하고,
ASC는 GAS 실행을 담당하며,
ABS는 GA와 Attribute 데이터를 제공하고,
GA는 액션의 규칙과 흐름을 처리하고,
GE는 코스트와 쿨타임 같은 효과 처리를 담당하고,
AM은 애니메이션 타이밍과 데미지 데이터를 관리하고,
ABP는 최종적으로 보이는 애니메이션을 출력한다.
```
