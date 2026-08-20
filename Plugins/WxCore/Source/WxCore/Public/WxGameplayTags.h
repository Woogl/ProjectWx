// Copyright Woogle. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/** 태그 추가 시 이 파일과 WxGameplayTags.cpp에만 작성. */
namespace WxGameplayTags
{
	// ── State ──────────────────────────────────────────────────────────────
	
	/** 로컬 플레이어가 이 액터를 락온 중일 때 로컬로만 부여(복제 안 함). 네임플레이트 표시 조건으로 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_LockedOn);

	/** 적 AI가 플레이어를 인식하면 서버에서 부여, 추적 종료 시 제거. 네임플레이트 표시 조건으로 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_InCombat);

	/** 처형(앞잡·뒤잡) 피대상 표시. 연출 동안 WxAbility_Finisher가 대상 ASC에 권위 발행하며, 대상이 이 태그로 자기 처형 어포던스를 닫는다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_BeingFinished);

	/** WxAnimNotifyState_ComboWindow가 부여/제거 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_ComboWindow);

	/** 대화 세션 컴포넌트가 시작·종료에 맞춰 폰 ASC에 loose 태그로 발행. WxAbility_Interact가 ActivationBlockedTags로 사용해 대화 중 프롬프트 표시·상호작용을 닫는다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dialogue);

	// ── Effect ──────────────────────────────────────────────────────────────
	
	// GE가 부여하는 태그. 애셋 태그로도 사용한다.

	/** WxEffect_Invincible이 부여하며, 노티파이·컷신은 구간 길이를 스펙에 실어 스스로 만료되게 하고 처형은 활성 구간에 묶는다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Invincible);

	/** 가드 어빌리티가 WxEffect_Guard로 부여하되, SP 고갈로 가드가 깨지면 어빌리티가 도는 중에도 뗀다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Guard);

	/** WxAnimNotifyState_PerfectGuard가 WxEffect_PerfectGuard로 구간 길이만큼 부여한다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_PerfectGuard);

	/** SP를 소모하면 WxEffect_Exhaust가 일정 시간 부여한다. SP 자연 회복의 억제 조건 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_Exhausted);

	/** 궁극기가 WxEffect_SuperArmor로 활성 구간만큼 부여한다. 대미지는 그대로 들어오고 경직(HitReact)만 막힌다 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Effect_SuperArmor);

	// ── Movement ──────────────────────────────────────────────────────────────
	
	/** WxCharacterMovementComponent가 낙하 모드 진입·이탈에 맞춰 각 머신에서 부여/제거. 점프 공격의 콤보 세트 진입 조건 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_InAir);
	
	/** WxAbility_Sprint가 활성 중 실제로 이동할 때만 부여. SP 소모 GE의 발동 조건이자 SP 자연 회복의 억제 조건 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Sprint);

	// ── Event ──────────────────────────────────────────────────────────────
	
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Normal);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_KnockBack);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_KnockDown);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_KnockUp);

	/** 공격이 퍼펙트 가드로 막힌 공격자에게 송출 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Parry);

	/** 피니셔(앞잡) 짝 피격 이벤트 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Finisher);

	/** 백스탭(뒤잡) 피격 이벤트 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_HitReact_Backstab);

	/** 무적 구간에서 대미지를 회피했을 때 발송 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_DodgeSuccess);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_PerfectGuard);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_UseItem);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Interact);

	/** 피니셔(앞잡) 발동 이벤트*/
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Finisher);

	/** 백스탭(뒤잡) 발동 이벤트 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Backstab);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Death);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Groggy);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Ragdoll);
	
	// ── Gimmick ──────────────────────────────────────────────────────────────

	// 기믹의 State Tree 상태값이며 곧 세이브 슬롯에 저장된다.
	// 코드에서 읽거나 쓰는 값은 아니지만 태그는 여기서 정의한다.

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_Door_Close);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_Door_Open);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_Elevator_Closed);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_Elevator_AtStart);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_Elevator_AtEnd);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_TreasureChest_Closed);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_TreasureChest_Open);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_CheckPoint_Unlit);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gimmick_CheckPoint_Lit);

	// ── GameplayCue ──────────────────────────────────────────────────────────────
	
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_DamageFloater);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_PerfectGuard);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Exceed);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Burn);

	/** 공격 경고(선딜 표시) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Red);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Yellow);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Blue);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AttackTelegraph_Purple);
	
	// ── Damage ──────────────────────────────────────────────────────────────

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Critical);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_CanCritical);

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Unblockable);

	/** 패리 피격 유발 공격. 이 공격이 퍼펙트 가드로 막히면 공격자에게 Event.HitReact.Parry 송출 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_ParryHitReact);
	
	// ── Ability ──────────────────────────────────────────────────────────────
	
	/**
	 * 어빌리티는 자신을 가리키는 식별 태그 Ability.X를 정확히 하나 갖고, AssetTags와 ActivationOwnedTags 양쪽에 넣는다.
	 * 곧 "Ability.X = 그 어빌리티가 지금 활성화 중이다"가 성립한다.
	 * 어빌리티의 성질을 나타내는 분류 마커는 이 루트가 아니라 Trait.*에 있다.
	*/

	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability);

	/** 플레이어 캐릭터용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Light);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Heavy);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_Air);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack_DodgeCounter);
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
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_UseItem);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Finisher);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_LockOn);
	
	/** 플레이어 캐릭터, 적 공용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_HitReact);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Groggy);
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Death);

	/** 적 캐릭터용 */
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

	// ── Trait ──────────────────────────────────────────────────────────────

	// 어빌리티의 성질을 나타내는 분류 마커. 식별 태그와 달리 ActivationOwnedTags에는 넣지 않는다 — 활성 신호가 아니라 분류다.

	/**
	 * 액션 슬롯을 점유하는 어빌리티.
	 *
	 * 이 태그는 어디에 넣느냐로 뜻이 갈리고, 세 쓰임은 서로 독립이다.
	 * AssetTags에 넣으면 "나는 액션이다" — 남이 건 잠금에 막히고 남의 취소에 끊긴다.
	 * BlockAbilitiesWithTag에 넣으면 "액션을 막는다" — 활성 동안 새 액션의 발동을 막는다.
	 * CancelAbilitiesWithTag에 넣으면 "액션을 끊는다" — 발동 시점에 돌던 액션을 끝낸다.
	 *
	 * 세 줄 모두 어빌리티 생성자가 필요한 것만 직접 선언한다.
	 *
	 *                                      표식  잠금  취소
	 *   Attack, Skill, Pattern, Interact     O    O    -
	 *   Dodge, Guard, Ultimate, UseItem      O    O    O      후딜 캔슬로 들어가 앞 액션을 끊는다
	 *   Death, Groggy, Finisher              -    O    O      반응이라 남의 잠금을 넘어 들어간다
	 *   HitReact                             -    O    ※      마커 대신 Ability.Attack·Skill로 좁혀 건다(적 패턴을 살린다)
	 *   Sprint, LockOn                       -    -    -      액션 슬롯을 쓰지 않는 이동·카메라 보조
	 *
	 * 취소는 후딜 캔슬 창에서 뜻이 생긴다 — 잠금이 풀린 뒤 비집고 들어온 어빌리티가 아직 도는 앞 액션을 끝낸다.
	 * 부류가 취소까지 정하게 만들 수는 없다 — 공격이 이 목록을 차단 우회 판정에도 쓰므로(HasActiveCancelTarget), 액션에 마커를 넣으면 어떤 액션 도중에도 공격이 발동한다.
	 *
	 * 잠금은 후딜 진입(StartRecovery)에 풀리고 EndAbility에서 엔진이 마저 정리한다.
	 * 어빌리티 밖에서도 건다 — 기믹 연출은 ASC에 직접 걸어 그 동안의 액션과 점프를 함께 막는다.
	 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Trait_Ability_Exclusive);

	// 점프는 이 태그의 차단 여부를 보므로 후딜에 열린다 — 캔슬 액션과 같은 취급.

	// ── SetByCaller ──────────────────────────────────────────────────────────────

	/** 지속시간 Duration SetByCaller 키. NoCooldown/InfiniteMP/DrainDP 등 Duration 모디파이어에서 공용으로 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Duration);

	/** UP 회복량 SetByCaller 키. WxEffect_RecoverResource의 UP 모디파이어에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Recovery_UP);

	/** MP 회복량 SetByCaller 키. WxEffect_RecoverResource의 MP 모디파이어에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Recovery_MP);

	/** DP 가산량 SetByCaller 키. WxEffect_AddDP의 DP 모디파이어에서 사용 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_DP);

	/** 공격력 계수 SetByCaller 키. WxExecCalc_Damage가 ATK 어트리뷰트에 곱하는 배율 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Coeff_ATK);

	/** 원시 대미지 SetByCaller 키. 양수일 때 ATK/DEF/Coeff/크리를 우회하고 평탄 값으로 대미지 처리 (환경 대미지). 가드/퍼펙트 가드/HitReact는 정상 동작 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_RawDamage);

	/** 이동 속도 배율 SetByCaller 키. WxEffect_MoveSpeedScale이 SPD 어트리뷰트에 곱하는 배율 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MoveSpeedScale);
	
	// ── UI ──────────────────────────────────────────────────────────────

	/** HUD 레이어 (플레이어 체력 바 등) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Game);

	/** 게임 메뉴 레이어 (아이템 획득 알림 등, 메뉴 아래) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_GameMenu);

	/** 메뉴 레이어 (인벤토리, 설정 등) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Menu);

	/** 모달 레이어 (확인 창 등) */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Layer_Modal);

	/** CommonUI 액션: 인벤토리 토글. HUD가 FUIActionTag로 변환해 RegisterUIActionBinding으로 수신, 키 매핑은 CommonUI Input Settings에서 지정 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_Inventory);

	/** CommonUI 액션: 메인 메뉴 토글. HUD가 FUIActionTag로 변환해 RegisterUIActionBinding으로 수신, 키 매핑은 CommonUI Input Settings에서 지정 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_MainMenu);

	/** CommonUI 액션: 자유 커서 홀드. HUD가 Pressed/Released로 나눠 수신해 입력 모드를 전환한다. 키 매핑은 CommonUI Input Settings에서 지정 */
	WXCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(UI_Action_FreeCursor);
}
