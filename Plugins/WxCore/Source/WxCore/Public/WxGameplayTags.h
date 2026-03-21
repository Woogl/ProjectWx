// Copyright Woogle. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/**
 * 프로젝트 전체에서 사용하는 Gameplay Tag 선언부.
 * 태그 추가 시 이 파일과 WxGameplayTags.cpp에만 작성.
 *
 * 네이밍: 점(.) 구분자를 언더스코어(_)로 치환하여 변수명 사용.
 * 예) "State.Dead" → State_Dead
 */
namespace WxGameplayTags
{
	// ── State ──────────────────────────────────────────────────────────────

	/** 사망 상태. HandleDeath 시 ASC에 부여되며, 부활 시 제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead);

	/** 공중 상태. Falling/Flying 시 부여, 착지 시 제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Airborne);

	/** 락온 상태. 락온 어빌리티 활성 중 부여, Look 입력 억제 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_LockOn);

	// ── Event ─────────────────────────────────────────────────────────────

	/** 피격 시 발생하는 이벤트. HitReact 어빌리티의 트리거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact);

	// ── ANS ───────────────────────────────────────────────────────────────

	/** 무기 콜리전 활성 구간. ANS_WeaponCollision이 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ANS_WeaponCollision);

	/** 콤보 입력 수용 구간. ANS_ComboWindow가 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ANS_ComboWindow);

	/** 무적 구간. ANS_Invincible이 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ANS_Invincible);

	/** 가드 판정 구간. ANS_Guard가 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ANS_Guard);

	// ── Ability ───────────────────────────────────────────────────────────

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dodge);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Guard);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Jump);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill);

	// ── Cooldown ──────────────────────────────────────────────────────────

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Attack);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Dodge);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Guard);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Jump);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Skill);

	// ── Input ──────────────────────────────────────────────────────────────

	/** 점프 입력 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Jump);

	/** 공격 입력 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Attack);

	/** 회피 입력 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Dodge);

	/** 가드 입력 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Guard);

	/** 락온 입력 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Skill);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_LockOn);

	// ── UI ────────────────────────────────────────────────────────────────

	/** HUD 레이어 (체력 바 등) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game);

	/** 게임 메뉴 레이어 (아이템 획득 알림 등, 메뉴 아래) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu);

	/** 메뉴 레이어 (인벤토리, 설정 등) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Menu);

	/** 모달 레이어 (확인 창 등) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal);
}
