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

	/** 그로기 상태. DP가 MaxDP에 도달 시 부여 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Groggy);

	/** 락온 상태. 락온 어빌리티 활성 중 부여, Look 입력 억제 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_LockOn);

	/** 인식 상태. 적 AI가 플레이어를 인식하면 서버에서 부여, 추적 종료 시 제거. 네임플레이트 표시 조건으로 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Recognized);

	/** 무적 상태. WxAnimNotifyState_Invincible이 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invincible);
	
	/** 가드 판정 활성 상태 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Guard);

	/** 퍼펙트 가드 판정 구간. ANS_PerfectGuard가 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_PerfectGuard);

	/** 피격 반응 상태. WxAbility_HitReact가 ActivationOwnedTags로 활성 중에만 자동 부여 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_HitReact);

	/** 슈퍼 아머 상태. 소유 중인 어빌리티는 HitReact로 캔슬되지 않음 (HitReact의 ActivationBlockedTags) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_SuperArmor);

	// ── Event ─────────────────────────────────────────────────────────────

	/** 피격 이벤트 부모 카테고리. ExecCalc 필터링용. Guard 어빌리티가 자식 태그를 모두 수신하기 위해 부모로도 구독 (직접 dispatch 금지) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact);

	/** 일반 피격 이벤트. HitReact 어빌리티가 기본 HitReactMontage 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Normal);

	/** 넉백 피격 이벤트. HitReact 어빌리티가 Knockback 몽타주 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Knockback);

	/** 넉다운 피격 이벤트. HitReact 어빌리티가 Knockdown 몽타주 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Knockdown);

	/** 넉업 피격 이벤트. HitReact 어빌리티가 Knockup 몽타주 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Knockup);

	/** 패리 피격 이벤트. 공격이 퍼펙트 가드로 막힌 공격자에게 송출, HitReact 어빌리티가 ParryReactMontage 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Parry);

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

	/** 데미지 Cue. 임팩트 위치에 스파크/사운드 재생, 데미지 플로터 출력 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Damage);

	/** 퍼펙트 가드 성공 Cue. 임팩트 위치에 스파크/사운드 재생 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_PerfectGuard);

	/** ATK 버프 지속 Cue */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_BuffATK);

	/** Exceed 버프 지속 Cue. 활성 동안 캐릭터의 무기에 Niagara 이펙트를 부착 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Exceed);

	/** 화상 지속 Cue */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Burn);

	/** 역경직(히트 스톱) Cue. WxWeaponBase로 공격 적중 시 공격자·피격자의 애니메이션 일시 정지 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_HitStop);

	/** 변신 Cue. GE 유효 기간 동안 캐릭터 외형을 교체한다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Metamorphose);

	// ── Damage ────────────────────────────────────────────────────────────

	/** 치명타 판정 결과 태그 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Critical);

	/** 가드 불가 공격 태그. 가드·퍼펙트 가드를 무시하고 풀 대미지 적용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Unblockable);

	/** 패리 피격 유발 공격 태그. 이 공격이 퍼펙트 가드로 막히면 공격자에게 Event.HitReact.Parry 송출 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_ParryHitReact);

	// ── Ability ───────────────────────────────────────────────────────────

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dodge);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Sprint);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Guard);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_1);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_2);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_3);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_4);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Ultimate);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Interact);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_1);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_2);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_3);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_4);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_5);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_6);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_7);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_8);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_9);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_10);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Pattern_Phase);

	// ── SetByCaller ──────────────────────────────────────────────────────

	/** 지속시간 Duration SetByCaller 키. Cooldown/NoCooldown/InfiniteMP/DrainDP, Groggy 등 Duration 모디파이어에서 공용으로 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Duration);

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

	/** 원시 대미지 SetByCaller 키. 양수일 때 ATK/DEF/Coeff/크리를 우회하고 평탄 값으로 대미지 처리 (환경 대미지). 가드/퍼펙트 가드/HitReact는 정상 동작 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_RawDamage);

	// ── Input ──────────────────────────────────────────────────────────────

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
