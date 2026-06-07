# WxCombat — Gameplay Tag & 어빌리티 규칙

태그 정의는 `WxCore/WxGameplayTags.h`. 이 문서는 사용 규칙.
**원칙: 태그는 출처가 아니라 의미로 가른다 — 소비자는 조건만 조회한다.**

## 네임스페이스

- `Ability.*` — 어빌리티 정체성(= 캔슬 대상 표식). AssetTag, 활성 시 오너 컨테이너.
- `State.*` — 액터 조건/모드. 디커플드 시스템이 조회.
- `ANS.*` — 노티파이 윈도우/플러밍 신호, 전투 내부 소비. (WeaponCollision·ComboWindow·CancelWindow)

## 규칙

1. **캔슬 대상이면 `Ability.X` AssetTag, 아니면 AssetTag 없이 `State.*`.** AssetTag 없음 = 광역 캔슬 면역. (면역 4종: LockOn·HitReact·Groggy·Death)
2. **광역 `CancelAbilitiesWithTag(Ability)` 유지.** 새 캔슬 대상은 AssetTag만 붙이면 자동 포함. 선택적 끊기는 구체 태그(예: HitReact는 Attack·Skill만).
3. **활성 신호가 필요하면 AssetTag를 OwnedTag로 재사용,** 소비처 없으면 발행 안 함. (Attack 자기차단, Pattern→HitReact, Dodge→LockOn)
4. **몽타주 구간 = `ANS.*`(윈도우/플러밍) 또는 `State.*`(액터 조건).** State.Guard처럼 어빌리티 수명과 다른 구간일 수 있음.
5. **다출처/외부 조건 = `State.X` 하나.** 출처가 발행, 정체성으로 대체 금지. (SuperArmor·Invincible·Dead·Groggy·Aerial)
6. **부모 `Ability`는 Cancel/Block(AssetTag 매칭)에서만.** ActivationBlocked/Required·HasMatchingGameplayTag(오너 컨테이너)엔 금지.

## 멀티(4인) 주의

- 복제는 소비자 머신 기준 태그별 결정(`AddLooseGameplayTag`는 기본 로컬). 서버 데미지를 막는 조건은 anim 틱 의존 회피.
- Loose/OwnedTag는 비정상 종료(EndAbility·NotifyEnd 누락·파괴)에서도 정리 — 가능하면 수명 자동 묶음(OwnedTag/GE Duration) 우선.
