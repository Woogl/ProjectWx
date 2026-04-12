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

	/** 무적 상태. WxAnimNotifyState_Invincible이 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invincible);
	
	/** 가드 판정 활성 상태 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Guard);

	/** 퍼펙트 가드 판정 구간. ANS_PerfectGuard가 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_PerfectGuard);

	/** 피격 반응 상태. WxAbility_HitReact가 ActivationOwnedTags로 활성 중에만 자동 부여 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_HitReact);

	// ── Event ─────────────────────────────────────────────────────────────

	/** 피격 이벤트 부모 카테고리. ExecCalc 필터링/디스패치 결정에만 사용 (직접 dispatch 금지) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact);

	/** 일반 피격 이벤트. HitReact 어빌리티가 기본 HitReactMontage 재생 (PP 소진 시) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Normal);

	/** 넉백 피격 이벤트. HitReact 어빌리티가 Knockback 몽타주 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Knockback);

	/** 넉다운 피격 이벤트. HitReact 어빌리티가 Knockdown 몽타주 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Knockdown);

	/** 넉업 피격 이벤트. HitReact 어빌리티가 Knockup 몽타주 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Knockup);

	/** 일반 가드 피격 이벤트. Guard 어빌리티가 GuardHitReact 몽타주 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_GuardHit_Normal);

	/** 넉 계열(Knockback/Knockdown/Knockup) 가드 피격 이벤트. Guard 어빌리티가 GuardKnockback 몽타주 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_GuardHit_Knockback);

	/** 회피 성공 시 발생하는 이벤트. 무적 구간에서 대미지를 회피했을 때 발송 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_DodgeSuccess);

	/** 퍼펙트 가드 성공 시 발생하는 이벤트. Guard 어빌리티가 MP 회복 및 HitReact를 처리 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_PerfectGuard);

	// ── ANS ───────────────────────────────────────────────────────────────

	/** 무기 콜리전 활성 구간. ANS_WeaponCollision이 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ANS_WeaponCollision);

	/** 콤보 입력 수용 구간. ANS_ComboWindow가 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ANS_ComboWindow);

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

	/** 가드 불가 공격 태그. 가드·퍼펙트 가드를 무시하고 풀 대미지 적용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Unblockable);

	// ── Ability ───────────────────────────────────────────────────────────

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Jump);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dodge);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Sprint);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Guard);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_1);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_2);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_3);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_4);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Ultimate);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Interact);

	// ── SetByCaller ──────────────────────────────────────────────────────

	/** 어빌리티 코스트 SetByCaller 키. WxEffect_CostMP, WxEffect_CostUP에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Cost);

	/** UP 회복량 SetByCaller 키. WxEffect_RecoverResource의 UP 모디파이어에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Recovery_UP);

	/** MP 회복량 SetByCaller 키. WxEffect_RecoverResource의 MP 모디파이어에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Recovery_MP);

	/** DP 반사량 SetByCaller 키. 퍼펙트 가드에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_ReflectDP);

	/** 공격력 계수 SetByCaller 키. WxExecCalc_Damage가 ATK 어트리뷰트에 곱하는 배율 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Coeff_ATK);

	// ── Input ──────────────────────────────────────────────────────────────

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Jump);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Attack);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Attack_Light);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Attack_Heavy);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Dodge);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Sprint);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Guard);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Skill_1);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Skill_2);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Skill_3);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Skill_4);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Ultimate);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_LockOn);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Input_Interact);

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
