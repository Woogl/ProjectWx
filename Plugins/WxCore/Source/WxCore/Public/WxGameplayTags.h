// Copyright Woogle. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/** 태그 추가 시 이 파일과 WxGameplayTags.cpp에만 작성. */
namespace WxGameplayTags
{
	// ── State ──────────────────────────────────────────────────────────────
	
	/** 대화 세션 컴포넌트가 시작·종료에 맞춰 폰 ASC에 loose 태그로 발행. WxAbility_Interact가 ActivationBlockedTags로 사용해 대화 중 프롬프트 표시·상호작용을 닫는다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dialogue);

	// ── Effect ──────────────────────────────────────────────────────────────
	
	// GE가 부여하는 태그. 애셋 태그로도 사용한다.

	/** WxEffect_Invincible이 부여하며, 구간을 연 쪽(노티파이 구간·컷신 태스크·처형의 활성 구간)이 수명을 쥔다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Invincible);

	/** 가드 어빌리티가 WxEffect_GuardReduction으로 부여하고 종료에서 걷는다. SP 고갈로 가드가 깨질 때는 리액션 어빌리티가 가드를 끊어 같은 경로로 걷힌다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_GuardReduction);

	/** 가드 몽타주의 노티파이 구간이 WxEffect_PerfectGuard로 부여하고 구간 끝에서 걷어낸다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_PerfectGuard);

	/** SP를 소모하면 WxEffect_Exhaust가 일정 시간 부여한다. SP 자연 회복의 억제 조건 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Exhausted);

	/** 궁극기가 WxEffect_SuperArmor로 활성 구간만큼 부여한다. 대미지는 그대로 들어오고 경직(HitReact)만 막힌다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SuperArmor);

	/** WxEffect_HitStop이 적중마다 무기·투사체의 HitStopDuration만큼 부여한다. 있는 동안 WxHitStopComponent가 몽타주를 얼린다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_HitStop);

	// ── Movement ──────────────────────────────────────────────────────────────
	
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_InAir);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Sprint);

	// ── HitReact ────────────────────────────────────────────────────────────

	/** 대미지 테이블이 저작하는 피격 반응 종류. Event.Hit의 TargetTags 페이로드로 전달한다. */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact_Normal);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact_KnockBack);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact_KnockDown);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact_KnockUp);

	// ── Event ──────────────────────────────────────────────────────────────
	
	/**
	 * 피격 이벤트. 대미지 파이프라인이 서버에서 피격자 ASC에 히트마다 한 번 보낸다.
	 * 공격이 요청한 반응 종류는 TargetTags의 HitReact.* 페이로드로 전달한다.
	 * 패리 반동과 가드 브레이크는 전투 시스템이 생성하는 별도 이벤트 자식으로 유지한다.
	 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit);
	
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit_Parry);

	/**
	 * 이 히트로 SP가 바닥나 가드가 깨졌다. 대미지 파이프라인이 서버에서 판정해 반응 태그 대신 이것을 보낸다.
	 * 클라의 복제 SP는 트리거보다 늦게 도착하므로 판정을 클라에서 다시 하면 서버와 갈린다.
	 *
	 * 파이프라인이 만들어 내는 결과값이라 공격의 반응 종류로 저작하면 안 된다 — 가드하지 않은 대상에게는 받아 줄 어빌리티가 없어 반응 없이 지나간다.
	 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Hit_GuardBreak);

	/**
	 * 적중 이벤트의 공격자 몫. 대미지 파이프라인이 서버에서 공격자 ASC에 히트마다 한 번 보낸다.
	 * EventMagnitude는 최종 대미지이고, ContextHandle에 그 히트를 낸 어빌리티가 실려 있다.
	 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_DamageDealt);

	/** 무적 구간에서 대미지를 회피했을 때 발송 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_DodgeSuccess);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_PerfectGuard);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Interact);

	/** 발동 장치가 연결 장치의 트리에 보내는 기본 이벤트. 목적지가 여럿인 장치는 버튼마다 다른 태그를 저작한다. */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Device_Triggered);

	/** 적 상호작용이 서버에서 플레이어 ASC에 보내는 처형 트리거. 앞잡·뒤잡은 어빌리티 하나가 받아, 페이로드 TargetTags(대상 소유 태그)의 Ability.Groggy 유무로 연출을 가른다. */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Finisher);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Death);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Groggy);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ragdoll);
	
	/** 소비 아이템의 스택 차감과 Usable 프래그먼트의 사용 효과 적용을 실행한다. */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_UseItem);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_ApplyFinisherDamage);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_SpawnProjectile);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_SpawnMinion);

	/** 주인이 관리 중인 소환수의 특정 어빌리티를 페이로드와 함께 발동한다. */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_CommandMinionAbility);

	// ── Device ──────────────────────────────────────────────────────────────

	// 장치의 State Tree 상태값이다.
	// 코드에서 읽거나 쓰는 값은 아니지만 태그는 여기서 정의한다.
	
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Button_Idle);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Button_Pressed);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Button_Locked);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Door_Close);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Door_Open);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Elevator_Inactive);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Elevator_1F);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Elevator_2F);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_TreasureChest_Closed);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_TreasureChest_Open);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_CheckPoint_Unlit);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_CheckPoint_Lit);
	
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Piston_On);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Device_Piston_Off);

	// ── GameplayCue ──────────────────────────────────────────────────────────────
	
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_DamageFloater);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_PerfectGuard);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_GhostTrail);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Exceed);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Red);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Yellow);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Blue);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Purple);
	
	// ── Damage ──────────────────────────────────────────────────────────────

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Critical);

	/** 대미지 ExecCalc가 판정해 스펙에 붙이는 결과 — 가드 히트의 SP 차감이 이 히트로 0에 닿았다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_GuardBreak);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_CanCritical);

	/** 가드로 막을 수 있는 공격. 이 태그가 없으면 일반 가드도 퍼펙트 가드도 뚫는다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_CanGuard);

	/** 패리가 성립하는 공격. 이 공격이 퍼펙트 가드로 막히면 공격자가 GP를 반사받고 Event.Hit.Parry로 역경직에 걸린다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_CanParry);
	
	// ── Ability ──────────────────────────────────────────────────────────────
	
	/**
	 * 어빌리티는 자신을 가리키는 식별 태그 Ability.X를 정확히 하나 갖고, AssetTags와 ActivationOwnedTags 양쪽에 넣는다.
	 * 곧 "Ability.X = 그 어빌리티가 지금 활성화 중이다"가 성립한다.
	*/

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability);

	/** 플레이어 캐릭터 전용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Light);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Heavy);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Air);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_DodgeCounter);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dodge);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Sprint);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Guard);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_GuardReact);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_1);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_2);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_3);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Skill_4);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Ultimate);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Interact);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_UseItem);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Finisher);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_LockOn);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Passive);

	/** 플레이어 캐릭터, 적 공용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_HitReact);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Groggy);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Death);

	/** 밖에서 주입된 일회성 몽타주 연출 중. 적의 처형 어포던스가 이걸로 닫힌다 — 처형 당하기는 기상까지가 그 구간이다. */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_PlayMontageOnce);

	/** 적 캐릭터 전용 */
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

	// ── Cooldown ──────────────────────────────────────────────────────────────

	/**
	 * 어빌리티별 쿨다운 GE가 부여하는 태그. 순정 CheckCooldown·쿨다운 조회 API가 이 태그로 쿨다운을 식별한다.
	 * 이름은 위 Ability.X 식별 태그를 따른다 — 어빌리티가 지정한 UWxEffect_Cooldown 파생 GE가 짝이 되는 태그를 부여한다.
	 * 적은 쿨다운을 쓰지 않는다 — 패턴 간격은 BT가 잡는다.
	 */

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Dodge);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Skill_1);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Ultimate);


	// ── SetByCaller ──────────────────────────────────────────────────────────────

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Magnitude);
	
	/** 지속시간 Duration SetByCaller 키. NoCooldown/InfiniteMP/HitStop의 DurationMagnitude에서 공용으로 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Duration);

	/** 공격력 계수 SetByCaller 키. WxExecCalc_Damage가 ATK 어트리뷰트에 곱하는 배율 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Coeff_ATK);

	/** 이동 속도 배율 SetByCaller 키. WxEffect_MoveSpeedScale이 SPD 어트리뷰트에 곱하는 배율 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MoveSpeedScale);
	
	// ── UI ──────────────────────────────────────────────────────────────

	/** HUD 레이어 (플레이어 체력 바 등) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game);

	/** 게임 메뉴 레이어 (아이템 획득 알림 등, 메뉴 아래) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Menu);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal);

	/** CommonUI 액션: 인벤토리 토글. HUD가 FUIActionTag로 변환해 RegisterUIActionBinding으로 수신, 키 매핑은 CommonUI Input Settings에서 지정 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_Inventory);

	/** CommonUI 액션: 메인 메뉴 토글. HUD가 FUIActionTag로 변환해 RegisterUIActionBinding으로 수신, 키 매핑은 CommonUI Input Settings에서 지정 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_MainMenu);

	/** CommonUI 액션: 자유 커서 홀드. HUD가 Pressed/Released로 나눠 수신해 입력 모드를 전환한다. 키 매핑은 CommonUI Input Settings에서 지정 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_FreeCursor);
}
