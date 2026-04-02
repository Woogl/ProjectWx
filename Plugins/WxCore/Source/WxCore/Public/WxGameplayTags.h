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
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Aerial);

	/** 그로기 상태. DP가 MaxDP에 도달 시 부여 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Groggy);

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

	/** 퍼펙트 가드 판정 구간. ANS_PerfectGuard가 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ANS_PerfectGuard);

	// ── GameplayCue ──────────────────────────────────────────────────────

	/** 데미지 플로터 출력 Cue */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Damage);

	/** ATK 버프 지속 Cue */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_BuffATK);

	/** 화상 지속 Cue */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Burn);

	/** 역경직(히트 스톱) Cue. WxWeaponBase로 공격 적중 시 공격자·피격자의 애니메이션 일시 정지 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_HitStop);

	// ── Damage ────────────────────────────────────────────────────────────

	/** 치명타 판정 결과 태그 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Critical);
	
	/** 데미지 플로터 출력 억제 태그 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_SuppressFloater);

	// ── Ability ───────────────────────────────────────────────────────────

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Jump);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Light);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Heavy);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dodge);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Sprint);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Guard);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Ultimate);

	// ── Cooldown ──────────────────────────────────────────────────────────

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Jump);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Attack_Light);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Attack_Heavy);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Dodge);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Guard);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Skill);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Ultimate);

	// ── SetByCaller ──────────────────────────────────────────────────────

	/** 어빌리티 코스트 SetByCaller 키. WxEffect_CostMP, WxEffect_CostUP에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Cost);

	/** 자원 회복량 SetByCaller 키. WxEffect_RecoveryMP, WxEffect_RecoveryUP에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Recovery);

	/** DP 반사량 SetByCaller 키. 퍼펙트 가드에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_ReflectDP);

	// ── Input ──────────────────────────────────────────────────────────────

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Jump);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Attack_Light);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Attack_Heavy);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Dodge);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Sprint);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Guard);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Skill);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ultimate);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_LockOn);

	// ── UI ────────────────────────────────────────────────────────────────

	/** HUD 레이어 (플레이어 체력 바 등) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game);

	/** 게임 메뉴 레이어 (아이템 획득 알림 등, 메뉴 아래) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu);

	/** 메뉴 레이어 (인벤토리, 설정 등) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Menu);

	/** 모달 레이어 (확인 창 등) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal);
}
