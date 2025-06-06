
#include <iostream>

#include "boe.global.hpp"

#include <array>
#include <map>

#include "universe/universe.hpp"

#include "boe.fileio.hpp"
#include "boe.graphics.hpp"
#include "boe.graphutil.hpp"
#include "boe.newgraph.hpp"
#include "boe.specials.hpp"
#include "boe.infodlg.hpp"
#include "boe.items.hpp"
#include "boe.actions.hpp"
#include <cstring>
#include <queue>
#include "boe.party.hpp"
#include "boe.monster.hpp"
#include "boe.town.hpp"
#include "boe.combat.hpp"
#include "boe.locutils.hpp"
#include "boe.combat.hpp"
#include "boe.text.hpp"
#include "sounds.hpp"
#include "boe.main.hpp"
#include "utility.hpp"
#include "mathutil.hpp"
#include "dialogxml/dialogs/strdlog.hpp"
#include "dialogxml/dialogs/choicedlog.hpp"
#include "dialogxml/dialogs/pictchoice.hpp"
#include "dialogxml/dialogs/3choice.hpp"
#include "tools/winutil.hpp"
#include "fileio/fileio.hpp"
#include "fileio/resmgr/res_dialog.hpp"
#include "boe.menus.hpp"
#include <boost/lexical_cast.hpp>
#include "dialogxml/widgets/button.hpp"
#include "spell.hpp"
#include "tools/cursors.hpp"
#include "gfx/render_shapes.hpp" // for colour constants

extern short skill_bonus[21];

// TODO: Use magic-names.txt instead of these arrays
bool get_mage[30] = {1,1,1,1,1,1,0,1,1,0, 1,1,1,1,1,1,0,0,1,1, 1,1,1,1,1,0,0,0,1,1};
bool get_priest[30] = {1,1,1,1,1,1,0,0,0,1, 1,1,1,1,1,0,0,0,1,1, 1,0,1,1,0,0,0,1,0,0};
short combat_percent[20] = {
	150,120,100,90,80,80,80,70,70,70,
	70,70,67,62,57,52,47,42,40,40};

short who_cast,which_pc_displayed;
// Light can be cast in or out of combat
const eSpell DEFAULT_SELECTED_MAGE = eSpell::LIGHT;
// Bless can only be cast in combat, so separate defaults are needed
const eSpell DEFAULT_SELECTED_PRIEST = eSpell::HEAL_MINOR;
const eSpell DEFAULT_SELECTED_PRIEST_COMBAT = eSpell::BLESS_MINOR;
eSpell town_spell;
extern bool spell_freebie;
extern eSpecCtxType spec_target_type;
extern short spec_target_fail, spec_target_options;
extern short shop_identify_cost, shop_recharge_limit, shop_recharge_amount;
bool spell_button_active[90];

extern short fast_bang;
extern bool flushingInput;
extern eItemWinMode stat_window;
extern eGameMode overall_mode;
extern fs::path progDir;
extern location center;
extern bool spell_forced,spell_recast,boom_anim_active;
extern eSpell store_mage, store_priest;
extern short store_mage_lev, store_priest_lev;
extern short store_mage_target, store_priest_target;
extern short store_mage_caster, store_priest_caster;
extern short store_spell_target,pc_casting;
extern short store_item_spell_level;
extern bool targeting_line_visible;
extern eStatMode stat_screen_mode;
extern effect_pat_type current_pat;
extern short current_spell_range;
extern std::array<short, 51> hit_chance;
extern short combat_active_pc;
extern std::map<eDamageType,int> boom_gr;
extern short current_ground;
extern location golem_m_locs[16];
extern cUniverse univ;
extern std::queue<pending_special_type> special_queue;

// Variables for spell selection
short on_which_spell_page = 0;
short store_last_cast_mage = 6,store_last_cast_priest = 6;
short buttons_on[38] = {1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0};
// buttons_on determines which buttons can be hit when on the second spell page
short spell_index[38] = {38,39,40,41,42,43,44,45,90,90,46,47,48,49,50,51,52,53,90,90,
	54,55,56,57,58,59,60,61,90,90, 90,90,90,90,90,90,90,90};
// Says which buttons hit which spells on second spell page, 90 means no button
bool can_choose_caster;

const sf::Color SELECTED_COLOUR = Colours::LIGHT_GREEN;
const sf::Color DISABLED_COLOUR = Colours::GREY;

// Dialog vars
short store_graphic_pc_num ;
short store_graphic_mode ;
short store_pc_graphic;

// When the party is placed into a scen from the starting screen, this is called to put the game into game
// mode and load in the scen and init the party info
// party record already contains scen name
void put_party_in_scen(std::string scen_name, bool force, bool allow_unpacked) {
	bool item_took = false;
	
	// Drop debug mode
	univ.debug_mode = false;
	univ.ghost_mode = false;
	univ.node_step_through = false;
	
	for(short j = 0; j < 6; j++)
		for(short i = 23; i >= 0; i--) {
			cItem& thisItem = univ.party[j].items[i];
			thisItem.special_class = 0;
			if(thisItem.ability == eItemAbil::CALL_SPECIAL) {
				univ.party[j].take_item(i);
				item_took = true;
			} else if(thisItem.ability == eItemAbil::WEAPON_CALL_SPECIAL) {
				univ.party[j].take_item(i);
				item_took = true;
			} else if(thisItem.ability == eItemAbil::HIT_CALL_SPECIAL) {
				univ.party[j].take_item(i);
				item_took = true;
			} else if(thisItem.ability == eItemAbil::DROP_CALL_SPECIAL) {
				univ.party[j].take_item(i);
				item_took = true;
			} else if(thisItem.ability == eItemAbil::PROTECT_FROM_SPECIES && thisItem.abil_data.race == eRace::IMPORTANT) {
				univ.party[j].take_item(i);
				item_took = true;
			} else if(thisItem.ability == eItemAbil::SLAYER_WEAPON && thisItem.abil_data.race == eRace::IMPORTANT) {
				univ.party[j].take_item(i);
				item_took = true;
			}
		}
	if(item_took)
		cChoiceDlog("removed-special-items").show();
	
	fs::path path = scen_name;
	if(!path.is_absolute()){
		path = locate_scenario(scen_name, allow_unpacked);
		if(path.empty()) {
			showError("Could not find scenario!");
			return;
		}
	}
	set_cursor(watch_curs);
	if(!load_scenario(path, univ.scenario))
		return;
	
	bool stored_item = false;
	for(auto& store : univ.party.stored_items)
		stored_item = stored_item || std::any_of(store.second.begin(), store.second.end(), [](const cItem& item) {
			return item.variety != eItemType::NO_ITEM;
		});
	if(stored_item)
		if(cChoiceDlog("keep-stored-items", {"yes", "no"}).show() == "yes") {
			std::vector<cItem*> saved_item_refs;
			for(short i = 0; i < 3;i++)
				for(short j = 0; j < univ.party.stored_items[i].size(); j++)
					if(univ.party.stored_items[i][j].variety != eItemType::NO_ITEM)
						saved_item_refs.push_back(&univ.party.stored_items[i][j]);
			short pc = 0;
			while(univ.party[pc].main_status != eMainStatus::ALIVE && pc < 6) pc++;
			show_get_items("Choose stored items to keep:", saved_item_refs, pc, true);
		}
	
	// Make sure the game build supports all the scenario's features
	for(auto pair : univ.scenario.feature_flags){
		if(!has_feature_flag(pair.first, pair.second)){
			showError("This scenario requires a feature that is not supported in your version of Blades of Exile: " + pair.first + " should support '" + pair.second + "'");
			return;
		}
	}

	univ.enter_scenario(scen_name);
	
	// if at this point, startup must be over, so make this call to make sure we're ready,
	// graphics wise
	end_startup();
	
	stat_screen_mode = MODE_INVEN;
	build_outdoors();
	erase_out_specials();
	
	univ.cur_pc = first_active_pc();
	force_town_enter(univ.scenario.which_town_start,univ.scenario.where_start);
	start_town_mode(univ.scenario.which_town_start,9);
	while(!special_queue.empty())
		special_queue.pop(); // Preserve legacy behaviour of not calling the "enter town" node at scenario start
	center = univ.scenario.where_start;
	update_explored(univ.scenario.where_start);
	overall_mode = MODE_TOWN;
	redraw_screen(REFRESH_ALL);
	set_stat_window(ITEM_WIN_PC1);
	adjust_spell_menus();
	adjust_monst_menu();
	
	if(!force){
		// Throw up intro dialog
		for(short j = 0; j < univ.scenario.intro_strs.size(); j++)
			if(!univ.scenario.intro_strs[j].empty()) {
				std::array<short, 3> buttons = {0,-1,-1};
				custom_choice_dialog(univ.scenario.intro_strs, univ.scenario.intro_mess_pic, PIC_SCEN, buttons);
				j = 6;
			}
		run_special(eSpecCtx::STARTUP, eSpecCtxType::SCEN, univ.scenario.init_spec, loc(0,0));
		give_help(1,2);
	}
	
	// Compatibility flags
	if(univ.scenario.format.prog_make_ver[0] < 2){
		univ.scenario.is_legacy = true;
	}
}

//spot; // if spot is 6, find one
bool create_pc(short spot,cDialog* parent) {
	bool still_ok = true;
	
	if(spot == 6) spot = univ.party.free_space();
	if(spot == 6) return false;
	univ.party.new_pc(spot);
	
	pick_race_abil(&univ.party[spot],0,parent);
	
	still_ok = spend_xp(spot,0,parent);
	if(!still_ok)
		return false;
	
	pick_pc_graphic(spot,0,parent);

	pick_pc_name(spot,parent);
	
	univ.party[spot].main_status = eMainStatus::ALIVE;
	
	if(overall_mode != MODE_STARTUP) {
		// If this is called while in startup mode, it means we're in the middle of building a party.
		// Thus, the PC should not be finalized yet.
		// However, if we're not in startup mode, it means we're adding a new PC to an existing party.
		// Thus, we must finalize the PC now.
		univ.party[spot].finish_create();
	}
	univ.party[spot].cur_health = univ.party[spot].max_health;
	univ.party[spot].cur_sp = univ.party[spot].max_sp;
	
	return true;
}

bool take_sp(short pc_num,short amt) {
	if(univ.party[pc_num].cur_sp < amt)
		return false;
	univ.party[pc_num].cur_sp -= amt;
	return true;
}

void increase_light(short amt) {
	location where;
	
	univ.party.light_level += amt;
	if(univ.party.light_level < 0)
		univ.party.light_level = 0;
	if(is_combat()) {
		for(short i = 0; i < 6; i++)
			if(univ.party[i].main_status == eMainStatus::ALIVE) {
				update_explored(univ.party[i].combat_pos);
			}
	}
	else {
		where = get_cur_loc();
		update_explored(where);
	}
	put_pc_screen();
}

void award_party_xp(short amt) {
	for(short i = 0; i < 6; i++)
		if(univ.party[i].main_status == eMainStatus::ALIVE)
			award_xp(i,amt);
}

void award_xp(short pc_num,short amt,bool force) {
	short adjust,add_hp;
	short xp_percent[30] = {
		150,120,100,90,80,70,60,50,50,50,
		45,40,40,40,40,35,30,25,23,20,
		15,15,15,15,15,15,15,15,15,15};
	if(univ.party[pc_num].level > 49) {
		univ.party[pc_num].level = 50;
		return;
	}
	if(!force && amt > 200) { // debug
		beep();
		ASB("Oops! Too much xp!");
		ASB("Report this!");
		return;
	}
	if(amt < 0) { // debug
		beep();
		ASB("Oops! Negative xp!");
		ASB("Report this!");
		return;
	}
	if(univ.party[pc_num].main_status != eMainStatus::ALIVE)
		return;
	
	if(univ.party[pc_num].level >= 40)
		adjust = 15;
	else adjust = xp_percent[univ.party[pc_num].level / 2];
	
	if((amt > 0) && (univ.party[pc_num].level > 7)) {
		if(get_ran(1,1,100) < xp_percent[univ.party[pc_num].level / 2])
			amt--;
	}
	if(amt <= 0)
		return;
	
	amt = percent(amt, adjust);
	amt = max(amt, 0);
	amt = percent(amt, univ.party[pc_num].exp_adj);
	univ.party[pc_num].experience += amt;
	univ.party.total_xp_gained += amt;
	
	
	
	if(univ.party[pc_num].experience < 0) {
		beep();
		ASB("Oops! Xp became negative somehow!");
		ASB("Report this!");
		univ.party[pc_num].experience = univ.party[pc_num].level * (univ.party[pc_num].get_tnl()) - 1;
		return;
	}
	if(univ.party[pc_num].experience > 15000) {
		univ.party[pc_num].experience = 15000;
		return;
	}
	
	while(univ.party[pc_num].experience >= (univ.party[pc_num].level * (univ.party[pc_num].get_tnl()))) {
		play_sound(7);
		univ.party[pc_num].level++;
		std::string level = std::to_string(univ.party[pc_num].level);
		add_string_to_buf("  " + univ.party[pc_num].name + " is level " + level + "!");
		univ.party[pc_num].skill_pts += (univ.party[pc_num].level < 20) ? 5 : 4;
		add_hp = (univ.party[pc_num].level < 26) ? get_ran(1,2,6) + skill_bonus[univ.party[pc_num].skills[eSkill::STRENGTH]]
			: max (skill_bonus[univ.party[pc_num].skills[eSkill::STRENGTH]],0);
		if(add_hp < 0)
			add_hp = 0;
		univ.party[pc_num].max_health += add_hp;
		if(univ.party[pc_num].max_health > 250)
			univ.party[pc_num].max_health = 250;
		univ.party[pc_num].cur_health += add_hp;
		if(univ.party[pc_num].cur_health > 250)
			univ.party[pc_num].cur_health = 250;
		put_pc_screen();
		
	}
}

void drain_pc(cPlayer& which_pc,short how_much) {
	if(which_pc.main_status == eMainStatus::ALIVE) {
		which_pc.experience = max(which_pc.experience - how_much,0);
		add_string_to_buf("  " + which_pc.name + " drained.");
	}
}

static short check_party_stat_get(short pc, eSkill which_stat) {
	switch(which_stat) {
		case eSkill::MAX_HP:
			return univ.party[pc].max_health;
		case eSkill::MAX_SP:
			return univ.party[pc].max_sp;
		case eSkill::CUR_HP:
			return univ.party[pc].cur_health;
		case eSkill::CUR_SP:
			return univ.party[pc].cur_sp;
		case eSkill::CUR_XP:
			return univ.party[pc].experience;
		case eSkill::CUR_SKILL:
			return univ.party[pc].skill_pts;
		case eSkill::CUR_LEVEL:
			return univ.party[pc].level;
		default:
			return univ.party[pc].skill(which_stat);
	}
	return 0;
}

// mode: 0 = total, 1 = mean, 2 = min, 3 = max, 10+i = just PC i
short check_party_stat(eSkill which_stat, short mode) {
	if(mode >= 10) return check_party_stat_get(mode - 10, which_stat);
	
	short total = mode == 2 ? std::numeric_limits<short>::max() : 0, num_pcs = 0;
	
	for(short i = 0; i < 6; i++)
		if(univ.party[i].main_status == eMainStatus::ALIVE) {
			num_pcs++;
			if(mode < 2)
				total += check_party_stat_get(i,which_stat);
			else if(mode == 3)
				total = max(check_party_stat_get(i,which_stat), total);
			else if(mode == 2)
				total = min(check_party_stat_get(i,which_stat), total);
		}
	
	if(mode == 1 && num_pcs > 0)
		total /= num_pcs;
	
	return total;
}

bool poison_weapon(short pc_num, short how_much,bool safe) {
	short p_level,r1;
	short p_chance[21] = {
		40,72,81,85,88,89,90,
		91,92,93,94,94,95,95,96,97,98,100,100,100,100};
	// TODO: This doesn't allow you to choose between poisoning a melee weapon and poisoning arrows, except by temporarily dequipping one
	
	auto weap = univ.party[pc_num].items.begin();
	do {
		weap = std::find_if(weap, univ.party[pc_num].items.end(), is_poisonable_weap);
		
		if(!univ.party[pc_num].equip[weap - univ.party[pc_num].items.begin()]) {
			weap++;
			continue;
		}
		
		p_level = how_much;
		add_string_to_buf("  You poison your weapon.");
		r1 = get_ran(1,1,100);
		// Nimble?
		if(univ.party[pc_num].traits[eTrait::NIMBLE])
			r1 -= 6;
		if(r1 > p_chance[univ.party[pc_num].skill(eSkill::POISON)] && !safe) {
			add_string_to_buf("  Poison put on badly.");
			p_level = p_level / 2;
			r1 = get_ran(1,1,100);
			if(r1 > p_chance[univ.party[pc_num].skill(eSkill::POISON)] + 10) {
				add_string_to_buf("  You nick yourself.");
				univ.party[pc_num].status[eStatus::POISON] += p_level;
			}
		}
		if(!safe)
			play_sound(55);
		univ.party[pc_num].weap_poisoned.slot = weap - univ.party[pc_num].items.begin();
		univ.party[pc_num].status[eStatus::POISONED_WEAPON] = max(univ.party[pc_num].status[eStatus::POISONED_WEAPON], p_level);
		
		return true;
	} while(weap != univ.party[pc_num].items.end());
	
	add_string_to_buf("  No weapon equipped.");
	return false;
}

bool is_poisonable_weap(cItem& weap) {
	if(weap.variety == eItemType::ONE_HANDED || weap.variety == eItemType::TWO_HANDED ||
		weap.variety == eItemType::ARROW || weap.variety == eItemType::BOLTS)
		return true;
	else return false;
	
}

//short type; // 0 - mage  1 - priest
void cast_spell(eSkill type) {
	eSpell spell;
	
	if((is_town()) && (univ.town.is_antimagic(univ.party.town_loc.x,univ.party.town_loc.y))) {
		add_string_to_buf("Cast: Not in antimagic field.");
		return;
	}
	
	if(!spell_forced)
		spell = pick_spell(6, type);
	else {
		if(spell_recast && !repeat_cast_ok(type))
			return;
		spell = type == eSkill::MAGE_SPELLS ? store_mage : store_priest;
		pc_casting = type == eSkill::MAGE_SPELLS ? store_mage_caster : store_priest_caster;
	}
	if(spell != eSpell::NONE) {
		print_spell_cast(spell,type);
		
		if(type == eSkill::MAGE_SPELLS)
			do_mage_spell(pc_casting,spell);
		else do_priest_spell(pc_casting,spell);
		put_pc_screen();
		
	}
}

bool repeat_cast_ok(eSkill type) {
	short who_would_cast;
	eSpellSelect store_select;
	eSpell what_spell;
	
	if(!prime_time()) return false;
	else if(overall_mode == MODE_COMBAT)
		who_would_cast = univ.cur_pc;
	else{
		if(has_feature_flag("store-spell-caster", "fixed")){
			who_would_cast = type == eSkill::MAGE_SPELLS ? store_mage_caster : store_priest_caster;
			if(who_would_cast == 6) who_would_cast = pc_casting;
		}else{
			who_would_cast = pc_casting;
		}
	}
	
	if(is_combat())
		what_spell = univ.party[who_would_cast].last_cast[type];
	else
		what_spell = type == eSkill::MAGE_SPELLS ? store_mage : store_priest;
	
	if(what_spell == eSpell::NONE){
		std::ostringstream sout;
		sout << "Repeat cast: No " << (type == eSkill::MAGE_SPELLS ? "mage" : "priest") << " spell stored.";
		add_string_to_buf(sout.str());
		return false;
	}

	if(!pc_can_cast_spell(univ.party[who_would_cast],what_spell)) {
		add_string_to_buf("Repeat cast: Can't cast.");
		return false;
	}
	store_select = (*what_spell).need_select;
	if(has_feature_flag("store-spell-target", "fixed")){
		if(is_combat())
			store_spell_target = univ.party[who_would_cast].last_target[type];
		else
			store_spell_target = type == eSkill::MAGE_SPELLS ? store_mage_target : store_priest_target;
	}
	if(store_select != SELECT_NO && store_spell_target == 6) {
		add_string_to_buf("Repeat cast: No target stored.");
		return false;
	}
	if(store_select == SELECT_ANY &&
			isAbsent(univ.party[store_spell_target].main_status)) {
		add_string_to_buf("Repeat cast: No target stored.");
		return false;
	}
	if(store_select == SELECT_ACTIVE &&
	   univ.party[store_spell_target].main_status != eMainStatus::ALIVE) {
		add_string_to_buf("Repeat cast: No target stored.");
		return false;
	}
	
	return true;
	
}

//which; // 100 + x : priest spell x
void give_party_spell(short which) {
	bool sound_done = false;
	
	if((which < 0) || (which > 161) || ((which > 61) && (which < 100))) {
		showError("The scenario has tried to give you a non-existant spell.");
		return;}
	
	// TODO: This seems like the wrong sounds
	// TODO: The order of checking seems wrong here; why check for alive after checking for the spell?
	if(which < 100)
		for(short i = 0; i < 6; i++)
			if(!univ.party[i].mage_spells[which]) {
				univ.party[i].mage_spells[which] = true;
				if(univ.party[i].main_status == eMainStatus::ALIVE)
					add_string_to_buf(univ.party[i].name + " learns spell.");
				give_help(41,0);
				if(!sound_done) {
					sound_done = true;
					play_sound(62);
				};
			}
	if(which >= 100)
		for(short i = 0; i < 6; i++)
			if(!univ.party[i].priest_spells[which - 100]) {
				univ.party[i].priest_spells[which - 100] = true;
				if(univ.party[i].main_status == eMainStatus::ALIVE)
					add_string_to_buf(univ.party[i].name + " learns spell.");
				give_help(41,0);
				if(!sound_done) {
					sound_done = true;
					play_sound(62);
				}
			}
}

void do_mage_spell(short pc_num,eSpell spell_num,bool freebie) {
	short target,r1,adj,store;
	location where;
	
	if(univ.party[pc_num].traits[eTrait::ANAMA]) {
		add_string_to_buf("Cast: You're an Anama!");
		return;
	}
	if(univ.party[pc_num].traits[eTrait::PACIFIST] && spell_num != eSpell::NONE && !(*spell_num).peaceful) {
		add_string_to_buf("Cast: You're a pacifist!");
		return;
	}
	
	where = univ.party.town_loc;
	play_sound(25);
	current_spell_range = 8;
	store_mage = spell_num;
	store_mage_target = store_spell_target;
	store_mage_caster = pc_casting;
	
	adj = freebie ? 1 : univ.party[pc_num].stat_adj(eSkill::INTELLIGENCE);
	short level = freebie ? store_item_spell_level : univ.party[pc_num].level;
	if(!freebie && (*spell_num).level <= univ.party[pc_num].get_prot_level(eItemAbil::MAGERY))
		level++;
	
	switch(spell_num) {
		case eSpell::LIGHT:
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			increase_light(50);
			break;
			
		case eSpell::IDENTIFY:{
			bool all_identified = univ.party.all_items_identified();

			// Cancel without spending points if there are no unidentified items
			if(!(freebie || all_identified))
				univ.party[pc_num].cur_sp -= (*spell_num).cost;

			if(!univ.scenario.is_legacy && is_town() && !all_identified) {
				ASB("Select items to identify. Press Space");
				ASB("   when done.");
				overall_mode = MODE_ITEM_TARGET;
				stat_screen_mode = MODE_IDENTIFY;
				shop_identify_cost = 0;
				put_item_screen(stat_window);
				break;
			}else{
				std::ostringstream sstr;
				sstr << "All of your items are ";
				if(all_identified){
					sstr << "already ";
				}else{
					for(cPlayer& pc : univ.party)
						for(cItem& item : pc.items)
							item.ident = true;
				}
				sstr << "identified.";
				ASB(sstr.str());
			}
			break;
		}

		case eSpell::RECHARGE:
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			if(is_town()) {
				ASB("Select items to recharge. Press Space");
				ASB("   when done.");
				overall_mode = MODE_ITEM_TARGET;
				stat_screen_mode = MODE_RECHARGE;
				shop_identify_cost = 0;
				shop_recharge_limit = 0;
				shop_recharge_amount = 1;
				put_item_screen(stat_window);
				break;
			}
			ASB("All of your items are recharged.");
			for(cPlayer& pc : univ.party)
				for(cItem& item : pc.items)
					if(item.rechargeable && item.charges < item.max_charges)
						item.charges = item.max_charges;
			break;
			
		case eSpell::TRUE_SIGHT:
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			for(where.x = 0; where.x < 64; where.x++)
				for(where.y = 0; where.y < 64; where.y++)
					if(dist(where,univ.party.town_loc) <= 2)
						make_explored(where.x,where.y);
			clear_map();
			break;
			
		case eSpell::SUMMON_BEAST:
			r1 = get_summon_monster(1);
			if(r1 < 0) break;
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			store = get_ran(3,1,4) + adj;
			if(!summon_monster(r1,where,store,eAttitude::FRIENDLY,true))
				add_string_to_buf("  Summon failed.");
			break;
		case eSpell::SUMMON_WEAK:
			store = level / 5 + adj / 3 + get_ran(1,0,2);
			(void) store; // TODO: Why is this calculation just discarded?
			r1 = get_summon_monster(1); ////
			if(r1 < 0) break;
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			store = get_ran(4,1,4) + adj;
			for(short i = 0; i < minmax(1,7,store); i++)
				if(!summon_monster(r1,where,store,eAttitude::FRIENDLY,true))
					add_string_to_buf("  Summon failed.");
			break;
		case eSpell::SUMMON:
			store = level / 7 + adj / 3 + get_ran(1,0,1);
			(void) store; // TODO: Why is this calculation just discarded?
			r1 = get_summon_monster(2); ////
			if(r1 < 0) break;
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			store = get_ran(5,1,4) + adj;
			for(short i = 0; i < minmax(1,6,store); i++)
				if(!summon_monster(r1,where,store,eAttitude::FRIENDLY,true))
					add_string_to_buf("  Summon failed.");
			break;
		case eSpell::SUMMON_AID:
			r1 = get_summon_monster(2);
			if(r1 < 0) break;
			store = get_ran(5,1,4) + adj;
			if(!summon_monster(r1,where,store,eAttitude::FRIENDLY,true))
				add_string_to_buf("  Summon failed.");
			break;
		case eSpell::SUMMON_MAJOR:
			store = level / 10 + adj / 3 + get_ran(1,0,1);
			(void) store; // TODO: Why is this calculation just discarded?
			r1 = get_summon_monster(3); ////
			if(r1 < 0) break;
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			store = get_ran(7,1,4) + adj;
			for(short i = 0; i < minmax(1,5,store); i++)
				if(!summon_monster(r1,where,store,eAttitude::FRIENDLY,true))
					add_string_to_buf("  Summon failed.");
			break;
		case eSpell::SUMMON_AID_MAJOR:
			r1 = get_summon_monster(3);
			if(r1 < 0) break;
			store = get_ran(7,1,4) + adj;
			if(!summon_monster(r1,where,store,eAttitude::FRIENDLY,true))
				add_string_to_buf("  Summon failed.");
			break;
		case eSpell::DEMON:
			store = get_ran(5,1,4) + 2 * adj;
			if(!summon_monster(85,where,store,eAttitude::FRIENDLY,true))
				add_string_to_buf("  Summon failed.");
			else if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			break;
		case eSpell::SUMMON_RAT:
			store = get_ran(5,1,4) + 2 * adj;
			if(!summon_monster(80,where,store,eAttitude::FRIENDLY,true))
				add_string_to_buf("  Summon failed.");
			break;
			
		case eSpell::DISPEL_SQUARE:
			start_town_targeting(spell_num,pc_num,freebie,PAT_SQ);
			break;
			
		case eSpell::LIGHT_LONG:
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			increase_light(200);
			break;
			
		case eSpell::MAGIC_MAP:
		{
			cInvenSlot item = univ.party[pc_num].has_abil(eItemAbil::SAPPHIRE);
			if(!item && !freebie)
				add_caster_needs_to_buf("a sapphire");
			else if(univ.town->defy_scrying || univ.town->defy_mapping)
				add_string_to_buf("  The spell fails.");
			else {
				if(freebie) add_string_to_buf("  You have a vision.");
				else {
					univ.party[pc_num].remove_charge(item.slot);
					if(stat_window == pc_num)
						put_item_screen(stat_window);
					univ.party[pc_num].cur_sp -= (*spell_num).cost;
					add_string_to_buf("  As the sapphire dissolves, you have a vision.", 2);
				}
				for(short i = 0; i < 64; i++)
					for(short j = 0; j < 64; j++)
						make_explored(i,j);
				clear_map();
			}
			break;
		}
			
			
		case eSpell::STEALTH:
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			univ.party.status[ePartyStatus::STEALTH] += max(6,level * 2);
			break;
			
			
		case eSpell::SCRY_MONSTER: case eSpell::UNLOCK: case eSpell::CAPTURE_SOUL: case eSpell::DISPEL_BARRIER:
		case eSpell::BARRIER_FIRE: case eSpell::BARRIER_FORCE: case eSpell::QUICKFIRE:
			start_town_targeting(spell_num,pc_num,freebie);
			break;
			
		case eSpell::ANTIMAGIC:
			start_town_targeting(spell_num,pc_num,freebie,PAT_RAD2);
			break;
			
		case eSpell::FLIGHT:
			if(univ.party.status[ePartyStatus::FLIGHT] > 0) {
				add_string_to_buf("  Not while already flying.");
				return;
			}
			if(univ.party.in_boat >= 0)
				add_string_to_buf("  Leave boat first.");
			else if(univ.party.in_horse >= 0) ////
				add_string_to_buf("  Leave horse first.");
			else {
				if(!freebie)
					univ.party[pc_num].cur_sp -= (*spell_num).cost;
				add_string_to_buf("  You start flying!");
				univ.party.status[ePartyStatus::FLIGHT] = 3;
			}
			break;
			
		case eSpell::RESIST_MAGIC: case eSpell::PROTECTION:
			target = store_spell_target;
			if(target < 6 && !freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			if(spell_num == eSpell::PROTECTION && target < 6) {
				univ.party[target].status[eStatus::INVULNERABLE] += 2 + adj + get_ran(2,1,2);
				for(short i = 0; i < 6; i++)
					if(univ.party[i].main_status == eMainStatus::ALIVE) {
						univ.party[i].status[eStatus::MAGIC_RESISTANCE] += 4 + level / 3 + adj;
					}
				add_string_to_buf("  Party protected.");
			}
			if(spell_num == eSpell::RESIST_MAGIC && target < 6) {
				univ.party[target].status[eStatus::MAGIC_RESISTANCE] += 2 + adj + get_ran(2,1,2);
				add_string_to_buf("  " + univ.party[target].name + " protected.");
			}
			break;
		default:
			add_string_to_buf("  Error: Mage spell " + (*spell_num).name() + " not implemented for town mode.", 4);
			break;
	}
}

void do_priest_spell(short pc_num,eSpell spell_num,bool freebie) {
	short r1,r2, target,store,adj,x,y;
	location loc;
	location where;
	
	if(univ.party[pc_num].traits[eTrait::PACIFIST] && spell_num != eSpell::NONE && !(*spell_num).peaceful) {
		add_string_to_buf("Cast: You're a pacifist!");
		return;
	}
	
	short store_victim_health,store_caster_health,targ_damaged; // for symbiosis
	
	where = univ.party.town_loc;
	
	adj = freebie ? 1 : univ.party[pc_num].stat_adj(eSkill::INTELLIGENCE);
	short level = freebie ? store_item_spell_level : univ.party[pc_num].level;
	if(!freebie && univ.current_pc().traits[eTrait::ANAMA])
		level++;
	
	play_sound(24);
	current_spell_range = 8;
	store_priest = spell_num;
	store_priest_target = store_spell_target;
	store_priest_caster = pc_casting;
	std::ostringstream loc_str;
	
	switch(spell_num) {
		case eSpell::LOCATION:
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			
			if(is_town()) {
				loc = (overall_mode == MODE_OUTDOORS) ? univ.party.out_loc : univ.party.town_loc;
				loc_str <<  "  You're at: x " << loc.x << "  y " << loc.y << '.';
			}
			if(is_out()) {
				loc = (overall_mode == MODE_OUTDOORS) ? univ.party.out_loc : univ.party.town_loc;
				x = loc.x; y = loc.y;
				x += 48 * univ.party.outdoor_corner.x; y += 48 * univ.party.outdoor_corner.y;
				loc_str << "  You're outside at: x " << x << "  y " << y << '.';
				
			}
			add_string_to_buf(loc_str.str());
			break;
			
		case eSpell::MANNA_MINOR: case eSpell::MANNA:
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			store = level / 3 + 2 * adj + get_ran(2,1,4);
			r1 = max(0,store);
			if(spell_num == eSpell::MANNA_MINOR)
				r1 = r1 / 3;
			add_string_to_buf("  You gain " + std::to_string(r1) + " food.   ");
			give_food(r1,true);
			break;
			
		case eSpell::RITUAL_SANCTIFY:
			add_string_to_buf("  Sanctify which space?");
			start_town_targeting(spell_num,pc_num,freebie);
			break;
			
		case eSpell::LIGHT_DIVINE:
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			increase_light(210);
			break;
			
		case eSpell::SUMMON_SPIRIT:
			if(!summon_monster(125,where,get_ran(2,1,4) + adj,eAttitude::FRIENDLY,true))
				add_string_to_buf("  Summon failed.");
			else if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			break;
		case eSpell::STICKS_TO_SNAKES:
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			r1 = level / 6 + adj / 3 + get_ran(1,0,1);
			for(int i = 0; i < r1; i++) {
				r2 = get_ran(1,0,7);
				store = get_ran(2,1,5) + adj;
				if(!summon_monster((r2 == 1) ? 100 : 99,where,store,eAttitude::FRIENDLY,true))
					add_string_to_buf("  Summon failed.");
			}
			break;
		case eSpell::SUMMON_HOST:
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			store = get_ran(2,1,4) + adj;
			if(!summon_monster(126,where,store,eAttitude::FRIENDLY,true))
				add_string_to_buf("  Summon failed.");
			for(int i = 0; i < 4; i++)	{
				store = get_ran(2,1,4) + adj;
				if(!summon_monster(125,where,store,eAttitude::FRIENDLY,true))
					add_string_to_buf("  Summon failed.");
			}
			break;
		case eSpell::SUMMON_GUARDIAN:
			store = get_ran(6,1,4) + adj;
			if(!summon_monster(122,where,store,eAttitude::FRIENDLY,true))
				add_string_to_buf("  Summon failed.");
			else if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			break;
			
		case eSpell::MOVE_MOUNTAINS: case eSpell::MOVE_MOUNTAINS_MASS:
			add_string_to_buf("  Destroy what?");
			start_town_targeting(spell_num,pc_num,freebie, spell_num == eSpell::MOVE_MOUNTAINS ? PAT_SINGLE : PAT_SQ);
			break;
			
		case eSpell::DISPEL_SPHERE: case eSpell::DISPEL_FIELD:
			start_town_targeting(spell_num,pc_num,freebie, spell_num == eSpell::DISPEL_SPHERE ? PAT_RAD2 : PAT_SINGLE);
			break;
			
		case eSpell::DETECT_LIFE:
			add_string_to_buf("  Monsters now on map.");
			univ.party.status[ePartyStatus::DETECT_LIFE] += 6 + get_ran(1,0,6);
			clear_map();
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			break;
		case eSpell::FIREWALK:
			add_string_to_buf("  You are now firewalking.");
			univ.party.status[ePartyStatus::FIREWALK] += level / 12 + 2;
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			break;
			
		case eSpell::SHATTER:
			add_string_to_buf("  You send out a burst of energy.");
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			for(loc.x = where.x - 1;loc.x < where.x + 2; loc.x++)
				for(loc.y = where.y - 1;loc.y < where.y + 2; loc.y++)
					crumble_wall(loc);
			update_explored(univ.party.town_loc);
			break;
			
		case eSpell::WORD_RECALL:
			if(!is_out()) {
				add_string_to_buf("  Can only cast outdoors.");
				return;
			}
			if(univ.party.in_boat >= 0) {
				add_string_to_buf("  Not while in boat.");
				return;
			}
			if(univ.party.in_horse >= 0) {////
				add_string_to_buf("  Not while on horseback.");
				return;
			}
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			// Clear forcecage status
			for(int i = 0; i < 6; i++)
				univ.party[i].status[eStatus::FORCECAGE] = 0;
			add_string_to_buf("  You are moved... ");
			force_town_enter(univ.scenario.which_town_start,univ.scenario.where_start);
			start_town_mode(univ.scenario.which_town_start,9);
			position_party(univ.scenario.out_sec_start.x,univ.scenario.out_sec_start.y,
						   univ.scenario.out_start.x,univ.scenario.out_start.y);
			center = univ.party.town_loc = univ.scenario.where_start;
//			overall_mode = MODE_OUTDOORS;
//			center = univ.party.p_loc;
//			update_explored(univ.party.p_loc);
			redraw_screen(REFRESH_ALL);
			break;
			
		case eSpell::HEAL_MINOR: case eSpell::HEAL: case eSpell::HEAL_MAJOR:
		case eSpell::POISON_WEAKEN: case eSpell::POISON_CURE: case eSpell::DISEASE_CURE:
		case eSpell::RESTORE_MIND: case eSpell::CLEANSE: case eSpell::AWAKEN: case eSpell::PARALYSIS_CURE:
			target = store_spell_target;
			if(target < 6) {
				if(!freebie)
					univ.party[pc_num].cur_sp -= (*spell_num).cost;
				std::ostringstream sout;
				sout << "  " << univ.party[target].name;
				switch(spell_num) {
					case eSpell::HEAL_MINOR: case eSpell::HEAL: case eSpell::HEAL_MAJOR:
						if(spell_num == eSpell::HEAL_MINOR)
							r1 = get_ran(2, 1, 4);
						else r1 = get_ran(2 + (spell_num == eSpell::HEAL ? 6 : 12), 1, 4);
						sout << " healed " << r1 << '.';
						univ.party[target].heal(r1);
						one_sound(52);
						break;
						
					case eSpell::POISON_WEAKEN: case eSpell::POISON_CURE:
						sout << " cured.";
						r1 = ((spell_num == eSpell::POISON_WEAKEN) ? 1 : 3) + get_ran(1,0,2) + adj / 2;
						univ.party[target].cure(r1);
						break;
						
					case eSpell::AWAKEN:
						if(univ.party[target].status[eStatus::ASLEEP] <= 0) {
							sout << " is already awake!";
							break;
						}
						sout << " wakes up.";
						univ.party[target].status[eStatus::ASLEEP] = 0;
						break;
					case eSpell::PARALYSIS_CURE:
						if(univ.party[target].status[eStatus::PARALYZED] <= 0) {
							sout << " isn't paralyzed!";
							break;
						}
						sout << " can move now.";
						univ.party[target].status[eStatus::PARALYZED] = 0;
						break;
						
					case eSpell::DISEASE_CURE:
						sout << " recovers.";
						r1 = 2 + get_ran(1,0,2) + adj / 2;
						univ.party[target].status[eStatus::DISEASE] = max(0,univ.party[target].status[eStatus::DISEASE] - r1);
						break;
						
					case eSpell::RESTORE_MIND:
						if(univ.party[target].status[eStatus::DUMB] <= 0) {
							sout << " isn't dumbfounded!";
							break;
						}
						sout << " restored.";
						r1 = 1 + get_ran(1,0,2) + adj / 2;
						univ.party[target].status[eStatus::DUMB] = max(0,univ.party[target].status[eStatus::DUMB] - r1);
						break;
						
					case eSpell::CLEANSE:
						sout << " cleansed.";
						univ.party[target].status[eStatus::DISEASE] = 0;
						univ.party[target].status[eStatus::WEBS] = 0;
						break;
					default:
						add_string_to_buf("  Error: Healing spell " + (*spell_num).name() + " not implemented for town mode.", 4);
						break;
				}
				add_string_to_buf(sout.str());
			}
			put_pc_screen();
			break;
			
		case eSpell::REVIVE: case eSpell::DESTONE: case eSpell::RAISE_DEAD: case eSpell::RESURRECT:
		case eSpell::CURSE_REMOVE: case eSpell::SANCTUARY: case eSpell::SYMBIOSIS: case eSpell::MARTYRS_SHIELD:
			target = store_spell_target;
			
			if(target < 6) {
				if(spell_num == eSpell::SYMBIOSIS && target == pc_num) {
					add_string_to_buf("  Can't cast on self.");
					return;
				}
				
				if(!freebie && spell_num != eSpell::RAISE_DEAD && spell_num != eSpell::RESURRECT)
					univ.party[pc_num].cur_sp -= (*spell_num).cost;
				std::ostringstream sout;
				sout << "  " << univ.party[target].name;
				if(spell_num == eSpell::MARTYRS_SHIELD) {
					sout << " shielded.";
					r1 = max(1,get_ran((level + 5) / 5,1,3) + adj);
					univ.party[target].status[eStatus::MARTYRS_SHIELD] += r1;
				} else if(spell_num == eSpell::SANCTUARY) {
					sout << " hidden.";
					r1 = max(0,get_ran(0,1,3) + level / 4 + adj);
					univ.party[target].status[eStatus::INVISIBLE] += r1;
				} else if(spell_num == eSpell::SYMBIOSIS) {
					store_victim_health = univ.party[target].cur_health;
					store_caster_health = univ.party[pc_num].cur_health;
					targ_damaged = univ.party[target].max_health - univ.party[target].cur_health;
					while((targ_damaged > 0) && (univ.party[pc_num].cur_health > 0)) {
						univ.party[target].cur_health++;
						r1 = get_ran(1,1,100) + level / 2 + 3 * adj;
						if(r1 < 100)
							univ.party[pc_num].cur_health--;
						if(r1 < 50)
							move_to_zero(univ.party[pc_num].cur_health);
						targ_damaged = univ.party[target].max_health - univ.party[target].cur_health;
					}
					add_string_to_buf("  You absorb damage.");
					sout << " healed " << univ.party[target].cur_health - store_victim_health << '.';
					add_string_to_buf(sout.str());
					clear_sstr(sout);
					sout << univ.party[pc_num].name << " takes " << store_caster_health - univ.party[pc_num].cur_health << '.';
				} else if(spell_num == eSpell::REVIVE) {
					sout << " healed.";
					univ.party[target].heal(250);
					univ.party[target].status[eStatus::POISON] = 0;
					one_sound(-53); one_sound(52);
				} else if(spell_num == eSpell::DESTONE) {
					if(univ.party[target].main_status == eMainStatus::STONE) {
						univ.party[target].main_status = eMainStatus::ALIVE;
						sout << " destoned.";
						play_sound(53);
					}
					else sout << " wasn't stoned.";
				} else if(spell_num == eSpell::CURSE_REMOVE) {
					for(cItem& item : univ.party[target].items)
						if(item.cursed){
							r1 = get_ran(1,0,200) - 10 * adj;
							if(r1 < 60) {
								item.cursed = item.unsellable = false;
							}
						}
					play_sound(52);
					sout.str("  Your items glow.");
				} else {
					// Scenario feature flag: requiring resurrection balm
					if(univ.scenario.has_feature_flag("resurrection-balm")) {
						if(cInvenSlot item = univ.party[pc_num].has_abil(eItemAbil::RESURRECTION_BALM)) {
							univ.party[pc_num].take_item(item.slot);
						} else {
							add_caster_needs_to_buf("resurrection balm");
							break;
						}
					}
					univ.party[pc_num].cur_sp -= (*spell_num).cost;
					if(spell_num == eSpell::RAISE_DEAD) {
						if(univ.party[target].main_status == eMainStatus::DEAD)
							if(get_ran(1,1,level / 2) == 1) {
								sout << " now dust.";
								play_sound(5);
								univ.party[target].main_status = eMainStatus::DUST;
							}
							else {
								univ.party[target].main_status = eMainStatus::ALIVE;
								for(int i = 0; i < 3; i++)
									if(get_ran(1,0,2) < 2) {
										eSkill skill = eSkill(i);
										univ.party[target].skills[skill] -= (univ.party[target].skills[skill] > 1) ? 1 : 0;
									}
								univ.party[target].cur_health = 1;
								sout << " raised.";
								play_sound(52);
							}
						else if(univ.party[target].main_status != eMainStatus::ALIVE)
							sout.str("  Didn't work.");
						else sout << " was OK.";
					} else if(spell_num == eSpell::RESURRECT) {
						if(univ.party[target].main_status != eMainStatus::ALIVE) {
							univ.party[target].main_status = eMainStatus::ALIVE;
							for(int i = 0; i < 3; i++)
								if(get_ran(1,0,2) < 1) {
									eSkill skill = eSkill(i);
									univ.party[target].skills[skill] -= (univ.party[target].skills[skill] > 1) ? 1 : 0;
								}
							univ.party[target].cur_health = 1;
							sout << " raised.";
							play_sound(52);
						}
						else sout << " was OK.";
					}
				}
				add_string_to_buf(sout.str());
				put_pc_screen();
			}
			break;
			
		case eSpell::HEAL_ALL_LIGHT: case eSpell::HEAL_ALL: case eSpell::REVIVE_ALL:
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			if(spell_num != eSpell::REVIVE_ALL) {
				r1 = get_ran((spell_num == eSpell::HEAL_ALL ? 6 : 3) + adj, 1, 4);
				add_string_to_buf("  Party healed " + std::to_string(r1) + ".");
				univ.party.heal(r1);
				play_sound(52);
			} else {
				r1 = get_ran(7 + adj, 1, 4);
				add_string_to_buf("  Party revived.");
				r1 = r1 * 2;
				univ.party.heal(r1);
				play_sound(-53);
				play_sound(-52);
				univ.party.cure(3 + adj);
			}
			break;
			
		case eSpell::POISON_CURE_ALL:
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			add_string_to_buf("  Party cured.");
			univ.party.cure(3 + adj);
			break;
			
		case eSpell::SANCTUARY_MASS: case eSpell::CLEANSE_MAJOR: case eSpell::HYPERACTIVITY:
			if(!freebie)
				univ.party[pc_num].cur_sp -= (*spell_num).cost;
			switch(spell_num) {
				case eSpell::SANCTUARY_MASS: add_string_to_buf("  Party hidden.");break;
				case eSpell::CLEANSE_MAJOR: add_string_to_buf("  Party cleansed.");break;
				case eSpell::HYPERACTIVITY: add_string_to_buf("  Party is now really, REALLY awake.");break;
				default:
					add_string_to_buf("  Error: Priest group spell " + (*spell_num).name() + " not implemented for town mode.", 4);
					break;
			}
			
			for(int i = 0; i < 6; i++)
				if(univ.party[i].main_status == eMainStatus::ALIVE) {
					if(spell_num == eSpell::SANCTUARY_MASS) {
						store = get_ran(0,1,3) + level / 6 + adj;
						r1 = max(0,store);
						univ.party[i].status[eStatus::INVISIBLE] += r1;
					}
					if(spell_num == eSpell::CLEANSE_MAJOR) {
						univ.party[i].status[eStatus::WEBS] = 0;
						univ.party[i].status[eStatus::DISEASE] = 0;
					}
					if(spell_num == eSpell::HYPERACTIVITY) {
						univ.party[i].status[eStatus::ASLEEP] -= 6 + 2 * adj;
						univ.party[i].status[eStatus::HASTE_SLOW] = max(0,univ.party[i].status[eStatus::HASTE_SLOW]);
					}
				}
			break;
			
		case eSpell::AVATAR:
			univ.party[pc_num].avatar();
			break;
			
		default:
			add_string_to_buf("  Error: Priest spell " + (*spell_num).name() + " not implemented for town mode.", 4);
			break;
	}
}

extern short spell_caster;
void cast_town_spell(location where) {
	short r1,store;
	bool need_redraw = false;
	location loc;
	ter_num_t ter;
	
	if((where.x <= univ.town->in_town_rect.left) ||
		(where.x >= univ.town->in_town_rect.right) ||
		(where.y <= univ.town->in_town_rect.top) ||
		(where.y >= univ.town->in_town_rect.bottom)) {
		add_string_to_buf("  Can't target outside town.");
		if(town_spell == eSpell::NONE)
			run_special(eSpecCtx::TARGET, spec_target_type, spec_target_fail, where);
		return;
	}
	
	short adjust = can_see_light(univ.party.town_loc,where,sight_obscurity);
	if(!spell_freebie)
		univ.party[who_cast].cur_sp -= (*town_spell).cost;
	ter = univ.town->terrain(where.x,where.y);
	short adj = spell_freebie ? 1 : univ.party[who_cast].stat_adj(eSkill::INTELLIGENCE);
	short level = spell_freebie ? store_item_spell_level : univ.party[who_cast].level;
	if(!spell_freebie && (*town_spell).level <= univ.party[who_cast].get_prot_level(eItemAbil::MAGERY) && !(*town_spell).is_priest())
		level++;
	if(!spell_freebie && univ.party[who_cast].traits[eTrait::ANAMA] && (*town_spell).is_priest())
		level++;
	
	// TODO: Should we do this here? Or in the handling of targeting modes?
	// (It really depends whether we want to be able to trigger it for targeting something other than a spell.)
	if(adjust <= 4 && !cast_spell_on_space(where, town_spell)) {
		// The special node intercepted and cancelled regular spell behaviour.
		queue_special(eSpecCtx::TARGET, spec_target_type, spec_target_fail, where);
		return;
	}
	if(spec_target_options / 10 == 1 && univ.town.is_antimagic(where.x,where.y)) {
		add_string_to_buf("  Target in antimagic field.");
		queue_special(eSpecCtx::TARGET, spec_target_type, spec_target_fail, where);
		return;
	}
	
	bool failed = town_spell == eSpell::NONE && adjust > 4;
	
	if(adjust > 4)
		add_string_to_buf("  Can't see target.");
	else switch(town_spell) {
		case eSpell::NONE: // Not a spell but a special node targeting
			run_special(eSpecCtx::TARGET, spec_target_type, spell_caster, where, nullptr, nullptr, &need_redraw);
			if(need_redraw) redraw_screen(REFRESH_ALL);
			break;
		case eSpell::SCRY_MONSTER: case eSpell::CAPTURE_SOUL:
			if(iLiving* targ = univ.target_there(where, TARG_MONST)) {
				cCreature* monst = dynamic_cast<cCreature*>(targ);
				if(town_spell == eSpell::SCRY_MONSTER) {
					univ.party.m_noted.insert(monst->number);
					adjust_monst_menu();
					display_monst(0,monst,0);
				}
				else record_monst(monst);
			}
			else add_string_to_buf("  No monster there.");
			break;
			
		case eSpell::DISPEL_FIELD: case eSpell::DISPEL_SPHERE: case eSpell::DISPEL_SQUARE:
			add_string_to_buf("  You attempt to dispel.");
			place_spell_pattern(current_pat,where,FIELD_DISPEL,7);
			break;
		case eSpell::MOVE_MOUNTAINS:
		case eSpell::MOVE_MOUNTAINS_MASS:
			add_string_to_buf("  You blast the area.");
			place_spell_pattern(current_pat, where, FIELD_SMASH, 7);
			update_explored(univ.party.town_loc);
			break;
		case eSpell::BARRIER_FIRE:
			if(sight_obscurity(where.x,where.y) == 5 || univ.target_there(where, TARG_MONST)) {
				add_string_to_buf("  Target space obstructed.");
				break;
			}
			univ.town.set_fire_barr(where.x,where.y,true);
			if(univ.town.is_fire_barr(where.x,where.y))
				add_string_to_buf("  You create the barrier.");
			else add_string_to_buf("  Failed.");
			break;
		case eSpell::BARRIER_FORCE:
			if(sight_obscurity(where.x,where.y) == 5 || univ.target_there(where, TARG_MONST)) {
				add_string_to_buf("  Target space obstructed.");
				break;
			}
			univ.town.set_force_barr(where.x,where.y,true);
			if(univ.town.is_force_barr(where.x,where.y))
				add_string_to_buf("  You create the barrier.");
			else add_string_to_buf("  Failed.");
			break;
		case eSpell::QUICKFIRE:
			univ.town.set_quickfire(where.x,where.y,true);
			if(univ.town.is_quickfire(where.x,where.y))
				add_string_to_buf("  You create quickfire.");
			else add_string_to_buf("  Failed.");
			break;
			
		case eSpell::ANTIMAGIC:
			add_string_to_buf("  You create an antimagic cloud.");
			for(loc.x = 0; loc.x < univ.town->max_dim; loc.x++)
				for(loc.y = 0; loc.y < univ.town->max_dim; loc.y++)
					if(dist(where,loc) <= 2 && can_see(where,loc,sight_obscurity) < 5 &&
					   ((abs(loc.x - where.x) < 2) || (abs(loc.y - where.y) < 2)))
						univ.town.set_antimagic(loc.x,loc.y,true);
			break;
			
		case eSpell::RITUAL_SANCTIFY:
			// Nothing to do here anymore; the code that used to be here is now just above the switch statement.
			break;
			
		case eSpell::UNLOCK:
			// TODO: Is the unlock spell supposed to have a max range?
			if(univ.scenario.ter_types[ter].special == eTerSpec::UNLOCKABLE){
				if(univ.scenario.ter_types[ter].flag2 == 10)
					r1 = 10000;
				else{
					r1 = get_ran(1,1,100) - 5 * adj + 5 * univ.town->difficulty;
					r1 += univ.scenario.ter_types[ter].flag2 * 7;
				}
				if(r1 < (135 - combat_percent[min(19,level)])) {
					add_string_to_buf("  Door unlocked.");
					play_sound(9);
					univ.town->terrain(where.x,where.y) = univ.scenario.ter_types[ter].flag1;
					univ.town->door_unlocked.push_back(where);
				}
				else {
					play_sound(41);
					add_string_to_buf("  Didn't work.");
				}
			}else
				add_string_to_buf("  Wrong terrain type.");
			break;
			
		case eSpell::DISPEL_BARRIER:
			if((univ.town.is_fire_barr(where.x,where.y)) || (univ.town.is_force_barr(where.x,where.y))) {
				r1 = get_ran(1,1,100) - 5 * adj + 5 * (univ.town->difficulty / 10) + 25 * univ.town->strong_barriers;
				if(univ.town.is_fire_barr(where.x,where.y))
					r1 -= 8;
				if(r1 < (120 - combat_percent[min(19,level)])) {
					add_string_to_buf("  Barrier broken.");
					univ.town.set_fire_barr(where.x,where.y,false);
					univ.town.set_force_barr(where.x,where.y,false);
					
					// Now, show party new things
					update_explored(univ.party.town_loc);
				}
				else {
					store = get_ran(1,0,1);
					(void) store; // TODO: Why does it even do this?
					play_sound(41);
					add_string_to_buf("  Didn't work.");
				}
			} else if(univ.town.is_force_cage(where.x,where.y)) {
				add_string_to_buf("  Cage broken.");
				break_force_cage(where);
			}
			
			else add_string_to_buf("  No barrier there.");
			
			break;
		default:
			add_string_to_buf("  Error: Spell " + (*town_spell).name() + " not implemented for town mode.", 4);
			break;
			
	}
	if(failed)
		queue_special(eSpecCtx::TARGET, spec_target_type, spec_target_fail, where);
}

// TODO: Currently, the node is called before any spell-specific behaviour (eg missiles) occurs.
bool cast_spell_on_space(location where, eSpell spell) {
	short s1 = 0;
	
	for(size_t i = 0; i < univ.town->special_locs.size(); i++)
		if(where == univ.town->special_locs[i]) {
			bool need_redraw = false;
			// TODO: Is there a way to skip this condition without breaking compatibility?
			if(univ.town->specials[univ.town->special_locs[i].spec].type == eSpecType::IF_CONTEXT)
				run_special(eSpecCtx::TARGET, eSpecCtxType::TOWN, univ.town->special_locs[i].spec, where, &s1, nullptr, &need_redraw);
			if(need_redraw) redraw_terrain();
			return !s1;
		}
	if(spell == eSpell::RITUAL_SANCTIFY)
		add_string_to_buf("  Nothing happens.");
	return true;
}

void crumble_wall(location where) {
	ter_num_t ter;
	
	if(loc_off_act_area(where))
		return;
	ter = univ.town->terrain(where.x,where.y);
	if(univ.scenario.ter_types[ter].special == eTerSpec::CRUMBLING && univ.scenario.ter_types[ter].flag2 < 2) {
		// TODO: This seems like the wrong sound
		play_sound(60);
		univ.town->terrain(where.x,where.y) = univ.scenario.ter_types[ter].flag1;
		add_string_to_buf("  Barrier crumbles.");
	}
	
}

void do_mindduel(short pc_num,cCreature *monst) {
	short i = 0,adjust,r1,r2,balance = 0;
	
	adjust = (univ.party[pc_num].level + univ.party[pc_num].skill(eSkill::INTELLIGENCE)) / 2 - monst->level * 2;
	adjust += univ.party[pc_num].get_prot_level(eItemAbil::WILL) * 5;
	if(monst->is_friendly()) {
		make_town_hostile();
		monst->attitude = eAttitude::HOSTILE_A;
	}
	
	std::ostringstream sout;
	add_string_to_buf("Mindduel!");
	while(univ.party[pc_num].main_status == eMainStatus::ALIVE && monst->is_alive() && i < 10) {
		play_sound(1);
		r1 = get_ran(1,1,100) + adjust;
		r1 += 5 * (monst->status[eStatus::DUMB] - univ.party[pc_num].status[eStatus::DUMB]);
		r1 += 5 * balance;
		r2 = get_ran(1,1,6);
		if(r1 < 30) {
			sout << "  " << univ.party[pc_num].name << " is drained " << r2 << '.';
			add_string_to_buf(sout.str(), 4);
			monst->mp += r2;
			balance++;
			if(univ.party[pc_num].cur_sp == 0) {
				univ.party[pc_num].status[eStatus::DUMB] += 2;
				clear_sstr(sout);
				sout << "  " << univ.party[pc_num].name << " is dumbfounded.";
				add_string_to_buf(sout.str(), 4);
				if(univ.party[pc_num].status[eStatus::DUMB] > 7) {
					clear_sstr(sout);
					sout << "  " << univ.party[pc_num].name << " is killed!";
					add_string_to_buf(sout.str(), 4);
					kill_pc(univ.party[pc_num],eMainStatus::DEAD);
				}
				
			}
			else {
				univ.party[pc_num].cur_sp = max(0,univ.party[pc_num].cur_sp - r2);
			}
		}
		if(r1 > 70) {
			sout << "  " << univ.party[pc_num].name << " drains " << r2 << '.';
			add_string_to_buf(sout.str(), 4);
			univ.party[pc_num].cur_sp += r2;
			balance--;
			if(monst->mp == 0) {
				monst->status[eStatus::DUMB] += 2;
				monst->spell_note(22);
				if(monst->status[eStatus::DUMB] > 7) {
					kill_monst(*monst,pc_num);
				}
				
			}
			else {
				monst->mp = max(0,monst->mp - r2);
			}
			
			
		}
		print_buf();
		i++;
	}
}

// mode 0 - dispel spell, 1 - always take  2 - always take and take fire and force too
void dispel_fields(short i,short j,short mode) {
	short r1;
	
	if(mode == 2) {
		univ.town.set_fire_barr(i,j,false);
		univ.town.set_force_barr(i,j,false);
		univ.town.set_barrel(i,j,false);
		univ.town.set_crate(i,j,false);
		univ.town.set_web(i,j,false);
	}
	if(mode >= 1)
		mode = -10;
	univ.town.set_fire_wall(i,j,false);
	univ.town.set_force_wall(i,j,false);
	univ.town.set_scloud(i,j,false);
	r1 = get_ran(1,1,6) + mode;
	if(r1 <= 1)
		univ.town.set_web(i,j,false);
	r1 = get_ran(1,1,6) + mode;
	if(r1 < 6)
		univ.town.set_ice_wall(i,j,false);
	r1 = get_ran(1,1,6) + mode;
	if(r1 < 5)
		univ.town.set_sleep_cloud(i,j,false);
	r1 = get_ran(1,1,8) + mode;
	if(r1 <= 1)
		univ.town.set_quickfire(i,j,false);
	r1 = get_ran(1,1,7) + mode;
	if(r1 < 5)
		univ.town.set_blade_wall(i,j,false);
	r1 = get_ran(1,1,12) + mode;
	if(r1 < 3)
		break_force_cage(loc(i,j));
}

extern std::array<short, 51> hit_chance;
eCastStatus pc_can_cast_spell(const cPlayer& pc,const eSkill type) {
	if(type == eSkill::MAGE_SPELLS && pc.traits[eTrait::ANAMA]) {
		return NO_CAST_ANAMA;
	}
	if(pc.skill(type) == 0) {
		return NO_CAST_SKILL;
	}
	if(pc.cur_sp == 0) {
		return NO_CAST_SP;
	}

	if(is_combat() && univ.town.is_antimagic(pc.combat_pos.x,pc.combat_pos.y)) {
		return NO_CAST_ANTIMAGIC;
	}
	if(is_combat() && type == eSkill::MAGE_SPELLS && pc.total_encumbrance(hit_chance) > 1) {
		return NO_CAST_ENCUMBERED;
	}
	
	if(type == eSkill::MAGE_SPELLS && pc_can_cast_spell(pc, eSpell::LIGHT))
		return CAST_OK;
	if(type == eSkill::PRIEST_SPELLS && pc_can_cast_spell(pc, eSpell::HEAL_MINOR))
		return CAST_OK;
	
	// If they can't cast the most basic level 1 spell, let's just make sure they can't cast any spells.
	// Find a spell they definitely know, and see if they can cast that.
	if(type == eSkill::MAGE_SPELLS && pc.mage_spells.any()){
		for(int i = 0; i < 62; i++){
			if(pc.mage_spells[i]){
				if(pc_can_cast_spell(pc, eSpell(i))) return CAST_OK;
				break;
			}
		}
	}
	if(type == eSkill::PRIEST_SPELLS && pc.priest_spells.any()){
		for(int i = 0; i < 62; i++){
			if(pc.priest_spells[i]){
				if(pc_can_cast_spell(pc, eSpell(i + 100))) return CAST_OK;
			}
		}
	}
	// If we get this far, either they don't know any spells (very unlikely) or they can't cast any of the spells they know.
	if(pc.status[eStatus::DUMB] > 0){
		return NO_CAST_DUMBFOUNDED;
	}
	if(pc.status[eStatus::PARALYZED] != 0){
		return NO_CAST_PARALYZED;
	}
	if(pc.status[eStatus::ASLEEP] > 0){
		return NO_CAST_ASLEEP;
	}
	return NO_CAST_UNKNOWN;
}

bool pc_can_cast_spell(const cPlayer& pc,eSpell spell_num) {
	if(spell_num == eSpell::NONE) return false;
	short level,store_w_cast;
	eSkill type = (*spell_num).type;
	cSpell spell = *spell_num;
	
	if(spell.need_select == eSpellSelect::SELECT_DEAD){
		bool any_dead = false;
		for(int i = 0; i < 6; ++i){
			if(dead_statuses.count(univ.party[i].main_status)) any_dead = true;
		}
		if(!any_dead) return false;
	}else if(spell.need_select == eSpellSelect::SELECT_STONE){
		bool any_stone = false;
		for(int i = 0; i < 6; ++i){
			if(univ.party[i].main_status == eMainStatus::STONE) any_stone = true;
		}
		if(!any_stone) return false;
	}

	level = spell.level;
	int effective_skill = pc.skill(type);
	if(pc.status[eStatus::DUMB] < 0)
		effective_skill -= pc.status[eStatus::DUMB];
	
	if(overall_mode >= MODE_TALKING)
		return false; // From Windows version. It does kinda make sense, though this function shouldn't even be called in these modes.
	if(!isMage(spell_num) && !isPriest(spell_num))
		return false;
	if(has_feature_flag("pacifist-spellcast-check", "V2") && pc.traits[eTrait::PACIFIST] && !spell.peaceful)
		return false;
	if(effective_skill < level)
		return false;
	if(pc.main_status != eMainStatus::ALIVE)
		return false;
	if(pc.cur_sp < spell.cost)
		return false;
	// TODO: Maybe get rid of the casts here?
	if(type == eSkill::MAGE_SPELLS && !pc.mage_spells[int(spell_num)])
		return false;
	if(type == eSkill::PRIEST_SPELLS && !pc.priest_spells[int(spell_num) - 100])
		return false;
	if(pc.status[eStatus::DUMB] >= 8 - level)
		return false;
	if(pc.status[eStatus::PARALYZED] != 0)
		return false;
	if(pc.status[eStatus::ASLEEP] > 0)
		return false;
	
	store_w_cast = spell.when_cast;
	if(is_out() && WHEN_OUTDOORS &~ store_w_cast)
		return false;
	if(is_town() && WHEN_TOWN &~ store_w_cast)
		return false;
	if(is_combat() &&  WHEN_COMBAT &~ store_w_cast)
		return false;
	return true;
}

// MARK: Begin spellcasting dialog

static void draw_caster_buttons(cDialog& me, const eSkill store_situation) {
	for(short i = 0; i < 6; i++) {
		std::string id = "caster" + boost::lexical_cast<std::string>(i + 1);
		if(!can_choose_caster) {
			if(i == pc_casting) {
				me[id].show();
			}
			else {
				me[id].hide();
			}
		}
		else {
			if(pc_can_cast_spell(univ.party[i],store_situation) == CAST_OK) {
				me[id].show();
			}
			else {
				me[id].hide();
			}
		}
	}
}

static void draw_spell_info(cDialog& me, const eSkill store_situation, const short store_spell) {
	
	
	if(store_spell == 70) { // No spell selected
		for(int i = 0; i < 6; i++) {
			std::string id = "target" + boost::lexical_cast<std::string>(i + 1);
			me[id].hide();
		}
		
	}
	else { // Spell selected
		
		for(int i = 0; i < 6; i++) {
			std::string id = "target" + boost::lexical_cast<std::string>(i + 1);
			switch((*cSpell::fromNum(store_situation,store_spell)).need_select) {
				case SELECT_NO:
					me[id].hide();
					break;
				case SELECT_ACTIVE:
					if(univ.party[i].main_status != eMainStatus::ALIVE) {
						me[id].hide();
					}
					else {
						me[id].show();
					}
					break;
				case SELECT_ANY:
					// Absent party members and split-off party members are excluded
					if(univ.party[i].main_status != eMainStatus::ABSENT && univ.party[i].main_status < eMainStatus::SPLIT) {
						me[id].show();
					}
					else {
						me[id].hide();
					}
					break;
				case SELECT_DEAD:
					if(dead_statuses.count(univ.party[i].main_status)) {
						me[id].show();
					}else{
						me[id].hide();
					}
					break;
				case SELECT_STONE:
					if(univ.party[i].main_status == eMainStatus::STONE) {
						me[id].show();
					}
					else {
						me[id].hide();
					}
					break;
			}
		}
	}
}

static void put_target_status_graphics(cDialog& me, short for_pc) {
	bool isAlive = univ.party[for_pc].main_status == eMainStatus::ALIVE;
	univ.party[for_pc].status[eStatus::HASTE_SLOW]; // This just makes sure it exists in the map, without changing its value if it does
	std::string id = "pc" + std::to_string(for_pc + 1);
	int slot = 0;
	for(auto next : univ.party[for_pc].status) {
		std::string id2 = id + "-stat" + std::to_string(slot + 1);
		cPict& pic = dynamic_cast<cPict&>(me[id2]);
		pic.setFormat(TXT_FRAME, FRM_NONE);
		if(isAlive) {
			short placedIcon = -1;
			const auto& statInfo = *next.first;
			if(statInfo.special && next.second >= statInfo.special->lo && next.second <= statInfo.special->hi) {
				placedIcon = statInfo.special->icon;
			}
			else if(next.second > 0) placedIcon = statInfo.icon;
			else if(next.second < 0) placedIcon = statInfo.negIcon;
			if(placedIcon >= 0) {
				pic.show();
				pic.setPict(placedIcon);
				slot++;
			} else pic.hide();
		} else pic.hide();
	}
	while(slot < 15) {
		std::string id2 = id + "-stat" + std::to_string(slot + 1);
		me[id2].hide();
		slot++;
	}
}

static void draw_spell_pc_info(cDialog& me) {
	for(short i = 0; i < 6; i++) {
		std::string n = boost::lexical_cast<std::string>(i + 1);
		me["arrow" + n].hide();
		put_target_status_graphics(me, i);
		if(univ.party[i].main_status != eMainStatus::ABSENT) {
			me["pc" + n].setText(univ.party[i].name);
			if(univ.party[i].main_status == eMainStatus::ALIVE) {
				me["hp" + n].setTextToNum(univ.party[i].cur_health);
				me["sp" + n].setTextToNum(univ.party[i].cur_sp);
				me["dead" + n].setText("");
			}else{
				me["hp" + n].setText("");
				me["sp" + n].setText("");
				switch(univ.party[i].main_status){
					case eMainStatus::DEAD:
						me["dead" + n].setText("Dead");
						break;
					case eMainStatus::STONE:
						me["dead" + n].setText("Stone");
						break;
					case eMainStatus::DUST:
						me["dead" + n].setText("Dust");
						break;
					default:
						me["dead" + n].setText("");
						break;
				}
			}
		}
	}
}


static void put_pc_caster_buttons(cDialog& me) {
	for(short i = 0; i < 6; i++) {
		std::string n = boost::lexical_cast<std::string>(i + 1);
		me["pc" + n].setColour(me.getDefTextClr());
		if(me["caster" + n].isVisible() && i == pc_casting){
			me["pc" + n].setColour(SELECTED_COLOUR);
		}
	}
}

static void put_pc_target_buttons(cDialog& me, short& store_last_target_darkened) {
	
	if(store_spell_target < 6) {
		std::string n = boost::lexical_cast<std::string>(store_spell_target + 1);
		me["arrow" + n].show();
	}
	if((store_last_target_darkened < 6) && (store_last_target_darkened != store_spell_target)) {
		std::string n = boost::lexical_cast<std::string>(store_last_target_darkened + 1);
		me["arrow" + n].hide();
	}
	store_last_target_darkened = store_spell_target;
}

// TODO: This stuff may be better handled by using an LED group with a custom focus handler
static void put_spell_led_buttons(cDialog& me, const eSkill store_situation,const short store_spell) {
	short spell_for_this_button;
	
	for(short i = 0; i < 38; i++) {
		spell_for_this_button = (on_which_spell_page == 0) ? i : spell_index[i];
		std::string id = "spell" + boost::lexical_cast<std::string>(i + 1);
		cLed& led = dynamic_cast<cLed&>(me[id]);
		
		if(spell_for_this_button < 90) {
			eSpell spell = cSpell::fromNum(store_situation, spell_for_this_button);
			if(store_spell == spell_for_this_button) {
				led.setState(led_green);
				// Text color:
				led.setColour(SELECTED_COLOUR);
			}
			else if(pc_can_cast_spell(univ.party[pc_casting],spell)) {
				led.setState(led_red);
				led.setColour(me.getDefTextClr());
			}
			else {
				led.setState(led_off);
				led.setColour(DISABLED_COLOUR);
			}
		}
	}
}

static void put_spell_list(cDialog& me, const eSkill store_situation) {
	if(on_which_spell_page == 0) {
		me["col1"].setText("Level 1:");
		me["col2"].setText("Level 2:");
		me["col3"].setText("Level 3:");
		me["col4"].setText("Level 4:");
		for(short i = 0; i < 38; i++) {
			std::ostringstream name;
			std::string id = "spell" + boost::lexical_cast<std::string>(i + 1);
			name << get_str("magic-names", i + (store_situation == eSkill::MAGE_SPELLS ? 1 : 101));
			name << " (";
			if((*cSpell::fromNum(store_situation,i)).cost < 0) { // Simulacrum, which has a variable cost
				name << '?';
			} else name << (*cSpell::fromNum(store_situation,i)).cost;
			name << ")";
			me[id].setText(name.str());
			rectangle bounds = me[id].getBounds();
			bounds.width() = me[id].getPreferredSize().x;
			me[id].setBounds(bounds);
			if(spell_index[i] == 90)
				me[id].show();
		}
	}
	else {
		me["col1"].setText("Level 5:");
		me["col2"].setText("Level 6:");
		me["col3"].setText("Level 7:");
		me["col4"].setText("");
		for(short i = 0; i < 38; i++) {
			std::ostringstream name;
			std::string id = "spell" + boost::lexical_cast<std::string>(i + 1);
			if(spell_index[i] < 90) {
				name << get_str("magic-names", spell_index[i] + (store_situation == eSkill::MAGE_SPELLS ? 1 : 101));
				name << " (";
				if((*cSpell::fromNum(store_situation,spell_index[i])).cost < 0) {
					name << '?';
				} else name << (*cSpell::fromNum(store_situation,spell_index[i])).cost;
				name << ")";
				me[id].setText(name.str());
				rectangle bounds = me[id].getBounds();
				bounds.width() = me[id].getPreferredSize().x;
				me[id].setBounds(bounds);
			}
			else me[id].hide();
		}
	}
}

static bool pick_spell_caster(cDialog& me, std::string id, const eSkill store_situation, short& last_darkened, short& store_spell) {
	short item_hit = id[id.length() - 1] - '1';
	pc_casting = item_hit;
	if(!pc_can_cast_spell(univ.party[pc_casting],cSpell::fromNum(store_situation,store_spell))) {
		if(store_situation == eSkill::MAGE_SPELLS)
			store_spell = 70;
		else store_spell = 70;
		store_spell_target = 6;
	}
	draw_spell_info(me, store_situation,store_spell);
	draw_spell_pc_info(me);
	put_spell_led_buttons(me, store_situation,store_spell);
	put_pc_caster_buttons(me);
	put_pc_target_buttons(me, last_darkened);
	return true;
}

static bool pick_spell_target(cDialog& me, std::string id, const eSkill store_situation, short& last_darkened, const short& store_spell) {
	static const char*const got_target = " Target selected.";
	short item_hit = id[id.length() - 1] - '1';
	me["feedback"].setText(got_target);
	store_spell_target = item_hit;
	draw_spell_info(me, store_situation, store_spell);
	put_pc_target_buttons(me, last_darkened);
	return true;
}

static bool pick_spell_event_filter(cDialog& me, std::string item_hit, const eSkill store_situation, const short& store_spell) {
	if(item_hit == "other") {
		on_which_spell_page = 1 - on_which_spell_page;
		put_spell_list(me, store_situation);
		put_spell_led_buttons(me, store_situation, store_spell);
	} else if(item_hit == "help") {
		give_help(7,8,me,true);
	}
	return true;
}

static bool pick_spell_select_led(cDialog& me, std::string id, eKeyMod mods, const eSkill store_situation, short& last_darkened, short& store_spell) {
	static const char*const choose_target = " Now pick a target.";
	static const char*const bad_spell = " Spell not available.";
	short item_hit = boost::lexical_cast<short>(id.substr(5)) - 1;
	if(mod_contains(mods, mod_alt)) {
		int i = (on_which_spell_page == 0) ? item_hit : spell_index[item_hit];
		display_spells(store_situation,i,&me);
	}
	else if(dynamic_cast<cLed&>(me[id]).getState() == led_off) {
		me["feedback"].setText(bad_spell);
	}
	else {
		store_spell = (on_which_spell_page == 0) ? item_hit : spell_index[item_hit];
		draw_spell_info(me, store_situation, store_spell);
		put_spell_led_buttons(me, store_situation, store_spell);
		
		if(store_spell_target < 6) {
			std::string targ = "target" + boost::lexical_cast<std::string>(store_spell_target + 1);
			if(!me[targ].isVisible()) {
				store_spell_target = 6;
				draw_spell_info(me, store_situation, store_spell);
				put_pc_target_buttons(me, last_darkened);
			}
		}
		bool any_targets = false;
		for(int i = 0; i < 6; ++i){
			std::string targ = "target" + boost::lexical_cast<std::string>(i + 1);
			if(me[targ].isVisible()){
				any_targets = true;
				break;
			}
		}
		if((store_spell_target == 6) && any_targets) {
			me["feedback"].setText(choose_target);
			draw_spell_info(me, store_situation, store_spell);
			play_sound(45); // formerly force_play_sound
		}
		else{
			me["feedback"].setText("");
			store_spell_target = 6;
			put_pc_target_buttons(me, last_darkened);
		}
		
	}
	return true;
}

static bool finish_pick_spell(cDialog& me, bool spell_toast, const short store_store_target, const short& store_spell, const eSkill store_situation) {
	if(spell_toast) {
		store_spell_target = store_store_target ;
		if(store_situation == eSkill::MAGE_SPELLS)
			store_last_cast_mage = pc_casting;
		else store_last_cast_priest = pc_casting;
		me.toast(false);
		me.setResult<short>(70);
		return true;
	}
	
	eSpell picked_spell = cSpell::fromNum(store_situation,store_spell);
	if(store_spell == 70) {
		add_string_to_buf("Cast: No spell selected.");
		store_spell_target = store_store_target ;
		me.toast(false);
		me.setResult<short>(70);
		return true;
	}
	if(store_situation == eSkill::MAGE_SPELLS && (*picked_spell).need_select == SELECT_NO) {
		store_last_cast_mage = pc_casting;
		univ.party[pc_casting].last_cast[store_situation] = picked_spell;
		univ.party[pc_casting].last_target[store_situation] = 6;
		univ.party[pc_casting].last_cast_type = store_situation;
		me.toast(false);
		me.setResult<short>(store_spell);
		return true;
	}
	if(store_situation == eSkill::PRIEST_SPELLS && (*picked_spell).need_select == SELECT_NO) {
		store_last_cast_priest = pc_casting;
		univ.party[pc_casting].last_cast[store_situation] = picked_spell;
		univ.party[pc_casting].last_target[store_situation] = 6;
		univ.party[pc_casting].last_cast_type = store_situation;
		me.toast(false);
		me.setResult<short>(store_spell);
		return true;
	}
	if(store_spell_target == 6) {
		add_string_to_buf("Cast: Need to select target.");
		store_spell_target = store_store_target ;
		me.toast(false);
		give_help(39,0,me);
		me.setResult<short>(70);
		return true;
	}
	me.setResult<short>((store_situation == eSkill::MAGE_SPELLS) ? store_spell : store_spell);
	if(store_situation == eSkill::MAGE_SPELLS)
		store_last_cast_mage = pc_casting;
	else store_last_cast_priest = pc_casting;
	univ.party[pc_casting].last_cast[store_situation] = picked_spell;
	univ.party[pc_casting].last_target[store_situation] = store_spell_target;
	univ.party[pc_casting].last_cast_type = store_situation;
	me.toast(true);
	return true;
}

eCastStatus check_can_cast(const cPlayer& pc, eSkill type) {
	return CAST_OK;	
}

void print_cast_status(eCastStatus status, eSkill type, std::string pc_name) {
	std::string prefix = "Cast";
	// When multiple PCs are checked, explain why each one can't cast.
	if(!pc_name.empty()) prefix += " (" + pc_name + ")";
	prefix += ": ";
	switch(status){
		case NO_CAST_ANAMA:
			add_string_to_buf(prefix + "You're an Anama!");
			break;
		case NO_CAST_SKILL:
			if(type == eSkill::MAGE_SPELLS) add_string_to_buf(prefix + "No mage skill.");
			else add_string_to_buf(prefix + "No priest skill.");
			break;
		case NO_CAST_ENCUMBERED:
			add_string_to_buf(prefix + "Too encumbered.");
			break;
		case NO_CAST_SP:
			add_string_to_buf(prefix + "No spell points.");
			break;
		case NO_CAST_ANTIMAGIC:
			add_string_to_buf(prefix + "Not in antimagic field.");
			break;
		case NO_CAST_DUMBFOUNDED:
			add_string_to_buf(prefix + "You're dumbfounded!");
			break;
		case NO_CAST_PARALYZED:
			add_string_to_buf(prefix + "You're paralyzed!");
			break;
		case NO_CAST_ASLEEP:
			add_string_to_buf(prefix + "You're asleep!");
			break;
		case NO_CAST_UNKNOWN:
			add_string_to_buf(prefix + "You can't!");
			break;
	}
}

//short pc_num; // if 6, anyone
//short type; // 0 - mage   1 - priest
//short situation; // 0 - out  1 - town  2 - combat
eSpell pick_spell(short pc_num,const eSkill type, bool check_done) { // 70 - no spell OW spell num
	using namespace std::placeholders;
	eSpell default_spell = type == eSkill::MAGE_SPELLS ? store_mage : store_priest;
	short former_target = store_spell_target;
	short dark = 6;
	
	pc_casting = type == eSkill::MAGE_SPELLS ? store_last_cast_mage : store_last_cast_priest;
	if(pc_casting == 6)
		pc_casting = univ.cur_pc;
	
	if(pc_num == 6) { // See if can keep same caster
		can_choose_caster = true;
		eCastStatus same_caster_status = pc_can_cast_spell(univ.party[pc_casting],type);
		// If the party is in an antimagic field, individual statuses won't matter
		if(same_caster_status == NO_CAST_ANTIMAGIC){
			print_cast_status(NO_CAST_ANTIMAGIC, type);
		}
		else if(same_caster_status != CAST_OK) {
			
			auto iter = std::find_if(univ.party.begin(), univ.party.end(), [type](const cPlayer& who) {
				eCastStatus status = pc_can_cast_spell(who, type);
				if(status == CAST_OK) return true;
				if(status >= NO_CAST_SP && status <= NO_CAST_ASLEEP)
					print_cast_status(status, type, who.name);
				return false;
			});
			
			if(iter == univ.party.end()) {
				add_string_to_buf("Cast: Nobody can.");
				return eSpell::NONE;
			}
			pc_casting = iter - univ.party.begin();
		}
	}
	else {
		can_choose_caster = false;
		pc_casting = pc_num;
	}
	
	if(!can_choose_caster && !check_done) {
		eCastStatus status = check_can_cast(univ.party[pc_casting], type);
		if(status != CAST_OK){
			print_cast_status(status, type);	
			return eSpell::NONE;
		}
	}
	
	// If in combat, make the spell being cast this PCs most recent spell
	if(is_combat()) {
		if(type == eSkill::MAGE_SPELLS)
			default_spell = univ.party[pc_casting].last_cast[eSkill::MAGE_SPELLS];
		else{
			default_spell = univ.party[pc_casting].last_cast[eSkill::PRIEST_SPELLS];
			if(default_spell == eSpell::NONE){
				default_spell = DEFAULT_SELECTED_PRIEST_COMBAT;
			}
		}
	}
	
	if(default_spell == eSpell::NONE){
		default_spell = type == eSkill::MAGE_SPELLS ? DEFAULT_SELECTED_MAGE : DEFAULT_SELECTED_PRIEST;
	}

	// Keep the stored spell, if it's still castable
	if(!pc_can_cast_spell(univ.party[pc_casting],default_spell)) {
		if(type == eSkill::MAGE_SPELLS) {
			default_spell = DEFAULT_SELECTED_MAGE;
		}
		else {
			default_spell = DEFAULT_SELECTED_PRIEST;
		}
	}
	
	// If a target is needed, keep the same target if that PC still targetable
	if(store_spell_target < 6) {
		if((*default_spell).need_select != SELECT_NO) {
			if(univ.party[store_spell_target].main_status != eMainStatus::ALIVE)
				store_spell_target = 6;
		} else store_spell_target = 6;
	}
	
	short former_spell = int(default_spell) % 100;
	// Set the spell page, based on starting spell
	if(former_spell >= 38) on_which_spell_page = 1;
	else on_which_spell_page = 0;
	
	set_cursor(sword_curs);
	
	extern std::unique_ptr<cDialog> storeCastSpell;
	cDialog& castSpell = *storeCastSpell;
	// untoast() is not usually called directly, but here it must be so that the reused dialog initializes
	// correctly, because cControl::isVisible() returns false for toasted dialogs
	castSpell.untoast(false);

	castSpell.attachClickHandlers(std::bind(pick_spell_caster, _1, _2, type, std::ref(dark), std::ref(former_spell)), {"caster1","caster2","caster3","caster4","caster5","caster6"});
	castSpell.attachClickHandlers(std::bind(pick_spell_target,_1,_2, type, std::ref(dark), std::ref(former_spell)), {"target1","target2","target3","target4","target5","target6"});
	castSpell.attachClickHandlers(std::bind(pick_spell_event_filter, _1, _2, type,std::ref(former_spell)), {"other", "help"});
	castSpell["cast"].attachClickHandler(std::bind(finish_pick_spell, _1, false, former_target, std::ref(former_spell), type));
	castSpell["cancel"].attachClickHandler(std::bind(finish_pick_spell, _1, true, former_target, std::ref(former_spell), type));
	
	dynamic_cast<cPict&>(castSpell["pic"]).setPict(14 + (type == eSkill::PRIEST_SPELLS),PIC_DLOG);
	for(int i = 0; i < 38; i++) {
		std::string id = "spell" + boost::lexical_cast<std::string>(i + 1);
		cKey key;
		if(i > 25)
			key = {false, static_cast<unsigned char>('a' + i - 26), mod_shift};
		else key = {false, static_cast<unsigned char>('a' + i), mod_none};
		cLed& led = dynamic_cast<cLed&>(castSpell[id]);
		led.attachKey(key);
		castSpell.addLabelFor(id, {static_cast<char>(i > 25 ? toupper(key.c) : key.c)}, LABEL_LEFT, 8, true);
		// All LEDs should get the click handler and set state, because page 2 will hide them if necessary
		led.setState((pc_can_cast_spell(univ.party[pc_casting],cSpell::fromNum(type,on_which_spell_page == 0 ? i : spell_index[i])))
					 ? led_red : led_green);
		led.attachClickHandler(std::bind(pick_spell_select_led, _1, _2, _3, type, std::ref(dark), std::ref(former_spell)));
	}
	
	put_spell_list(castSpell, type);
	draw_spell_info(castSpell, type, former_spell);
	draw_spell_pc_info(castSpell);
	draw_caster_buttons(castSpell, type);
	put_pc_caster_buttons(castSpell);
	put_spell_led_buttons(castSpell, type, former_spell);
	
	castSpell.runWithHelp(7, 8);
	
	return cSpell::fromNum(type, castSpell.getResult<short>());
}


//short which; // 0 - mage  1 - priest
void print_spell_cast(eSpell spell,eSkill which) {
	short spell_num = (which == eSkill::PRIEST_SPELLS ? int(spell) - 100 : int(spell));
	std::string name = get_str("magic-names", spell_num + (which == eSkill::MAGE_SPELLS ? 1 : 101));
	add_string_to_buf("Spell: " + name);
}

void start_town_targeting(eSpell s_num,short who_c,bool freebie,eSpellPat pat) {
	add_string_to_buf("  Target spell.");
	overall_mode = MODE_TOWN_TARGET;
	targeting_line_visible = true;
	town_spell = s_num;
	who_cast = who_c;
	spell_freebie = freebie;
	auto pattern = cPattern::get_builtin(pat);
	if(pattern.rotatable) {
		// Town targeting doesn't support rotatable walls
		// TODO: Some sort of error message instead of silently substituting SINGLE
		current_pat = cPattern::get_builtin(PAT_SINGLE).pattern;
	} else current_pat = pattern.pattern;
}

void do_alchemy() {
	short r1;
	short pc_num;
	
	pc_num = select_pc(eSelectPC::ONLY_LIVING, "Who will make a potion?", eSkill::ALCHEMY);
	if(pc_num == 6)
		return;
	
	eAlchemy potion = alch_choice(pc_num);
	// TODO: Remove need for this cast by changing the above data to either std::maps or an unary operator*
	if(potion != eAlchemy::NONE) {
		if(!univ.party[pc_num].has_space()) {
			add_string_to_buf("Alchemy: Can't carry another item.");
			return;
		}
		const cAlchemy& info = *potion;
		
		cInvenSlot which_item = univ.party[pc_num].has_abil(info.ingred1);
		if(!which_item) {
			add_string_to_buf("Alchemy: Don't have ingredients.");
			return;
		}
		
		if(info.ingred2 != eItemAbil::NONE) {
			cInvenSlot which_item2 = univ.party[pc_num].has_abil(info.ingred2);
			if(!which_item2) {
				add_string_to_buf("Alchemy: Don't have ingredients.");
				return;
			}
			// Take care with the order of removal so that remove_charge does not change the index of the second item removed
			if(which_item.slot < which_item2.slot) {
				univ.party[pc_num].remove_charge(which_item2.slot);
				univ.party[pc_num].remove_charge(which_item.slot);
			} else {
				univ.party[pc_num].remove_charge(which_item.slot);
				univ.party[pc_num].remove_charge(which_item2.slot);
			}
		} else univ.party[pc_num].remove_charge(which_item.slot);
		
		play_sound(8);
		
		r1 = get_ran(1,1,100);
		short skill = univ.party[pc_num].skill(eSkill::ALCHEMY);
		if(r1 < info.fail_chance(skill)) {
			add_string_to_buf("Alchemy: Failed.");
			play_sound(41);
		}
		else {
			cItem store_i(potion);
			store_i.charges = info.charges(skill);
			store_i.graphic_num += get_ran(1,0,2);
			if(univ.party[pc_num].give_item(store_i,false) != eBuyStatus::OK) {
				add_string_to_buf("No room in inventory. Potion placed on floor.", 2);
				place_item(store_i,univ.party.town_loc);
			}
			else add_string_to_buf("Alchemy: Successful.");
		}
		put_item_screen(stat_window);
	}
	
}

static bool alch_choice_event_filter(cDialog& me, std::string item_hit, eKeyMod) {
	if(item_hit == "help") {
		give_help(20,21,me,true);
		return true;
	}
	if(item_hit == "cancel")
		me.setResult<eAlchemy>(eAlchemy::NONE);
	else {
		int potion_id = boost::lexical_cast<int>(item_hit.substr(6));
		me.setResult<eAlchemy>(eAlchemy(potion_id));
	}
	me.toast(true);
	return true;
}

eAlchemy alch_choice(short pc_num) {
	
	set_cursor(sword_curs);
	
	cDialog chooseAlchemy(*ResMgr::dialogs.get("pick-potion"));
	chooseAlchemy.attachClickHandlers(alch_choice_event_filter, {"cancel", "help"});
	for(short i = 0; i < 20; i++) {
		std::string n = boost::lexical_cast<std::string>(i + 1);
		chooseAlchemy["potion" + n].hide();
		if(!univ.party.alchemy[i]){
			continue;
		}
		chooseAlchemy["label" + n].setText(get_str("magic-names", i + 200) + " (" + std::to_string((*eAlchemy(i)).difficulty)+ ")");
		chooseAlchemy["potion" + n].attachClickHandler(alch_choice_event_filter);
		if((*eAlchemy(i)).can_make(univ.party[pc_num].skill(eSkill::ALCHEMY))){
			chooseAlchemy["potion" + n].show();
		}
	}
	std::ostringstream sout;
	sout << univ.party[pc_num].name;
	sout << " (skill " << univ.party[pc_num].skill(eSkill::ALCHEMY) << ")";
	chooseAlchemy["mixer"].setText(sout.str());
	
	chooseAlchemy.runWithHelp(20, 21);
	return chooseAlchemy.getResult<eAlchemy>();
}

// mode ... 0 - create new pc  1 - change existing pc graphic
bool pick_pc_graphic(short pc_num,short mode,cDialog* parent) {
	store_graphic_pc_num = pc_num;
	store_graphic_mode = mode;
	
	set_cursor(sword_curs);
	
	cPictChoice pcPic(0,36,PIC_PC,parent);
	// When creating a new PC, the player has already put time into stat selection, so they won't want to
	// cancel and lose that work
	if(mode == 0){
		pcPic.disableCancel();
	}

	// Customize it for this special case of choosing a PC graphic
	dynamic_cast<cPict&>(pcPic->getControl("mainpic")).setPict(7);
	pcPic->getControl("prompt").setText("Select a graphic for your PC:");
	pcPic->getControl("help").setText("Click button to left of graphic to select.");
	
	bool madeChoice = pcPic.show(univ.party[pc_num].which_graphic);
	if(mode == 0 && !madeChoice && univ.party[pc_num].main_status < eMainStatus::ABSENT)
		univ.party[pc_num].main_status = eMainStatus::ABSENT;
	else if(madeChoice)
		univ.party[pc_num].which_graphic = pcPic.getPicChosen();
	
	return madeChoice;
}

static bool pc_name_event_filter(cDialog& me, short store_train_pc) {
	std::string pcName = me["name"].getText();
	
	if(pcName.empty()){
		me["error"].setText("Cannot be empty.");
	}else if(!isalpha(pcName[0])){
		me["error"].setText("Must begin with a letter.");
	}else{
		// TODO: This was originally truncated to 18 characters; is that really necessary?
		univ.party[store_train_pc].name = pcName;
		me.toast(true);
	}
	return true;
}

bool pick_pc_name(short pc_num,cDialog* parent) {
	using namespace std::placeholders;
	set_cursor(sword_curs);
	
	cDialog pcPickName(*ResMgr::dialogs.get("pick-pc-name"), parent);
	pcPickName["name"].setText(univ.party[pc_num].name);
	pcPickName["okay"].attachClickHandler(std::bind(pc_name_event_filter, _1, pc_num));
	
	pcPickName.run();
	
	return 1;
}

bool has_trapped_monst() {
	for(mon_num_t which : univ.party.imprisoned_monst) {
		if(which != 0) return true;
	}
	return false;
}

mon_num_t pick_trapped_monst() {
	short i = 0;
	std::string sp;
	cMonster get_monst;
	
	set_cursor(sword_curs);
	
	cChoiceDlog soulCrystal("soul-crystal",{"cancel","pick1","pick2","pick3","pick4"});
	
	for(mon_num_t which : univ.party.imprisoned_monst) {
		std::string n = boost::lexical_cast<std::string>(i + 1);
		if(which == 0) {
			soulCrystal->getControl("pick" + n).hide();
		}
		else {
			sp = get_m_name(which);
			soulCrystal->getControl("slot" + n).setText(sp);
			get_monst = which >= 10000 ? univ.party.summons[which - 10000] : univ.scenario.scen_monsters[which];
			soulCrystal->getControl("lvl" + n).setTextToNum(get_monst.level);
		}
		i++;
	}
	
	
	std::string result = soulCrystal.show();
	
	if(result == "cancel")
		return 0;
	else return univ.party.imprisoned_monst[result[4] - '1'];
}


bool flying() {
	if(univ.party.status[ePartyStatus::FLIGHT] == 0)
		return false;
	else return true;
}

void hit_party(short how_much,eDamageType damage_type,short snd_type) {
	short max_dam = 0;

	for(short i = 0; i < 6; i++){
		if(univ.party[i].main_status == eMainStatus::ALIVE){
			short dam = damage_pc(univ.party[i],how_much,damage_type,eRace::UNKNOWN,snd_type, true, is_combat());
			if(dam > max_dam) max_dam = dam;
		}
	}
	// Peace mode: one boom for the whole party, use the highest damage actually taken
	if(!is_combat() && max_dam > 0){
		int boom_type = boom_gr[damage_type];
		if(is_town())
			boom_space(univ.party.town_loc,overall_mode,boom_type,max_dam,snd_type);
		else
			boom_space(univ.party.out_loc,100,boom_type,max_dam,snd_type);
	}
	put_pc_screen();
}

void slay_party(eMainStatus mode) {
	boom_anim_active = false;
	for(short i = 0; i < 6; i++)
		if(univ.party[i].main_status == eMainStatus::ALIVE)
			univ.party[i].main_status = mode;
	put_pc_screen();
}

short damage_pc(cPlayer& which_pc,short how_much,eDamageType damage_type,eRace type_of_attacker, short sound_type,bool do_print, bool boom) {
	
	if(which_pc.main_status != eMainStatus::ALIVE)
		return false;
	
	// Note: sound type 0 can now be forced for UNBLOCKABLE by passing sound_type 0,
	// but -1 is the new value for "use default"
	sound_type = get_sound_type(damage_type, sound_type);

	int boom_type = boom_gr[damage_type];

	// armor
	static std::set<eDamageType> armor_resist_damage = { eDamageType::WEAPON, eDamageType::UNDEAD, eDamageType::DEMON };
	if(armor_resist_damage.count(damage_type)) {
		how_much -= minmax(-5,5,which_pc.status[eStatus::BLESS_CURSE]);
		for(short i = 0; i < cPlayer::INVENTORY_SIZE; i++) {
			const cItem& item = which_pc.items[i];
			if(item.variety != eItemType::NO_ITEM && which_pc.equip[i]) {
				if((*item.variety).is_armour) {
					short defense = 0;
					if(item.item_level > 0){
						defense = get_ran(1,1,item.item_level);
					}
					
					// bonus for magical items
					if(item.bonus > 0) {
						defense += get_ran(1,1,item.bonus) + item.bonus / 2;
					} else if(item.bonus < 0) {
						defense -= item.bonus;
					}
					how_much -= defense;
					short roll = get_ran(1,1,100);
					if(roll < hit_chance[which_pc.skill(eSkill::DEFENSE)] - 20)
						how_much -= 1;
				}
				if(item.protection > 0) {
					how_much -= get_ran(1,1,item.protection);
				} else if(item.protection < 0) {
					how_much += get_ran(1,1,-item.protection);
				}
			}
		}
	}
	
	// parry
	// TODO: Used to also apply this to fire damage; is that correct?
	if(damage_type == eDamageType::WEAPON && which_pc.parry < 100)
		how_much -= which_pc.parry / 4;
	
	if(damage_type != eDamageType::MARKED) {
		if(univ.party.easy_mode)
			how_much -= 3;
		// toughness
		if(which_pc.traits[eTrait::TOUGHNESS])
			how_much--;
		// luck
		if(get_ran(1,1,100) < 2 * (hit_chance[which_pc.skill(eSkill::LUCK)] - 20))
			how_much -= 1;
	}
	
	short prot_from_dmg = which_pc.get_prot_level(eItemAbil::DAMAGE_PROTECTION,int(damage_type));
	// Acid damage used to be magic damage, so magic protection counts as acid protection:
	if(damage_type == eDamageType::ACID){
		prot_from_dmg += which_pc.get_prot_level(eItemAbil::DAMAGE_PROTECTION,int(eDamageType::MAGIC));
	}
	if(prot_from_dmg > 0) {
		// TODO: Why does this not depend on the ability strength if it's not weapon damage?
		if(damage_type == eDamageType::WEAPON) how_much -= prot_from_dmg;
		else how_much = how_much / 2;
	}
	
	// TODO: Do these perhaps depend on the ability strength less than they should?
	short prot_from_race = which_pc.get_prot_level(eItemAbil::PROTECT_FROM_SPECIES,int(type_of_attacker));
	if(prot_from_race > 0)
		how_much = how_much / 2;
		
	if(isHumanoid(type_of_attacker) && !isHuman(type_of_attacker) && type_of_attacker != eRace::HUMANOID) {
		// If it's a slith, nephil, vahnatai, or goblin, Protection from Humanoids also helps
		// Humanoid is explicitly excluded here because otherwise it would help twice.
		short prot_from_humanoid = which_pc.get_prot_level(eItemAbil::PROTECT_FROM_SPECIES,int(eRace::HUMANOID));
		if(prot_from_humanoid > 0)
			how_much = how_much / 2;
	}
	
	if(type_of_attacker == eRace::SKELETAL) {
		// Protection from Undead helps with both types of undead
		short prot_from_undead = which_pc.get_prot_level(eItemAbil::PROTECT_FROM_SPECIES,int(eRace::UNDEAD));
		if(prot_from_undead > 0)
			how_much = how_much / 2;
	}
	
	// invuln
	if(damage_type != eDamageType::SPECIAL && which_pc.status[eStatus::INVULNERABLE] > 0)
		how_much = 0;
	
	// Mag. res helps w. fire and cold
	static std::set<eDamageType> magic_resist_damage = { eDamageType::FIRE, eDamageType::COLD };
	// Now it also helps with MAGIC:
	if(has_feature_flag("magic-resistance", "fixed")){
		magic_resist_damage.insert(eDamageType::MAGIC);
		magic_resist_damage.insert(eDamageType::ACID);
	}
	if(magic_resist_damage.count(damage_type)) {
		int magic_res = which_pc.status[eStatus::MAGIC_RESISTANCE];
		if(magic_res > 0)
			how_much /= 2;
		else if(magic_res < 0)
			how_much *= 2;
	}
	
	// major resistance
	short full_prot = which_pc.get_prot_level(eItemAbil::FULL_PROTECTION);
	std::set<eDamageType> major_resist_damage = { eDamageType::FIRE, eDamageType::POISON, eDamageType::MAGIC, eDamageType::ACID, eDamageType::COLD};
	if(major_resist_damage.count(damage_type)
	   && (full_prot > 0))
		how_much = how_much / ((full_prot >= 7) ? 4 : 2);
	
	if(boom_anim_active) {
		if(how_much < 0)
			how_much = 0;
		which_pc.marked_damage += how_much;
		short boom_type = get_boom_type(damage_type);
		if(is_town())
			add_explosion(univ.party.town_loc,how_much,0,boom_type,0,0);
		else add_explosion(which_pc.combat_pos,how_much,0,boom_type,0,0);
		if(how_much == 0)
			return false;
		else return true;
	}
	
	if(how_much <= 0) {
		if(damage_type == eDamageType::WEAPON || damage_type == eDamageType::UNDEAD || damage_type == eDamageType::DEMON)
			play_sound(2);
		add_string_to_buf ("  No damage.");
		return false;
	}
	else {
		// if asleep, get bonus
		if(which_pc.status[eStatus::ASLEEP] > 0)
			which_pc.status[eStatus::ASLEEP]--;
		
		if(do_print)
			add_string_to_buf("  " + which_pc.name + " takes " + std::to_string(how_much) + '.');
		if(damage_type != eDamageType::MARKED && boom) {
			if(is_combat())
				boom_space(which_pc.combat_pos,overall_mode,boom_type,how_much,sound_type);
			else if(is_town())
				boom_space(univ.party.town_loc,overall_mode,boom_type,how_much,sound_type);
			else
				boom_space(univ.party.out_loc,100,boom_type,how_much,sound_type);
		}
		// TODO: When outdoors it flushed only key events, not mouse events. Why?
		flushingInput = true;
	}
	
	univ.party.total_dam_taken += how_much;
	
	if(which_pc.cur_health >= how_much)
		which_pc.cur_health = which_pc.cur_health - how_much;
	else if(which_pc.cur_health > 0)
		which_pc.cur_health = 0;
	else // Check if PC can die
		if(how_much > 25) {
			add_string_to_buf("  " + which_pc.name + " is obliterated.");
			kill_pc(which_pc, eMainStatus::DUST);
		}
		else {
			add_string_to_buf("  " + which_pc.name + " is killed.");
			kill_pc(which_pc,eMainStatus::DEAD);
		}
	if(which_pc.cur_health == 0 && which_pc.main_status == eMainStatus::ALIVE)
		play_sound(3);
	put_pc_screen();
	
	return how_much;
}

void petrify_pc(cPlayer& which_pc,int strength) {
	std::ostringstream create_line;
	short r1 = get_ran(1,0,20);
	r1 += which_pc.level / 4;
	r1 += which_pc.status[eStatus::BLESS_CURSE];
	r1 -= strength;
	
	if(which_pc.has_abil_equip(eItemAbil::PROTECT_FROM_PETRIFY))
		r1 = 20;
	
	if(r1 > 14) {
		create_line << "  " << which_pc.name << " resists.";
	} else {
		create_line << "  " << which_pc.name << " is turned to stone.";
		kill_pc(which_pc,eMainStatus::STONE);
	}
	add_string_to_buf(create_line.str());
}

void kill_pc(cPlayer& which_pc,eMainStatus type) {
	bool dummy,no_save = false;
	location item_loc;
	
	if(isSplit(type)) {
		type -= eMainStatus::SPLIT;
		no_save = true;
	}
	
	cInvenSlot weap = type == eMainStatus::STONE
		? cInvenSlot(which_pc) // Life-saving doesn't protect from petrification
		: which_pc.has_abil_equip(eItemAbil::LIFE_SAVING);
	
	int luck = which_pc.skill(eSkill::LUCK);
	if(!no_save && type != eMainStatus::ABSENT && luck > 0 &&
	   get_ran(1,1,100) < hit_chance[luck]) {
		add_string_to_buf("  But you luck out!");
		which_pc.cur_health = 0;
	} else if(!weap || type == eMainStatus::ABSENT) {
		if(combat_active_pc < 6 && &which_pc == &univ.party[combat_active_pc])
			combat_active_pc = 6;
		
		for(short i = 0; i < cPlayer::INVENTORY_SIZE; i++)
			which_pc.equip[i] = false;
		
		item_loc = is_combat() ? which_pc.combat_pos : univ.party.town_loc;
		
		if(!is_out()) {
			if(type == eMainStatus::DUST)
				univ.town.set_ash(item_loc.x,item_loc.y,true);
			else if(type == eMainStatus::ABSENT);
			else switch(which_pc.race) {
				case eRace::DEMON:
					univ.town.set_ash(item_loc.x,item_loc.y,true);
					break;
				case eRace::UNDEAD:
					break;
				case eRace::SKELETAL:
					univ.town.set_bones(item_loc.x,item_loc.y,true);
					break;
				case eRace::SLIME: case eRace::PLANT: case eRace::BUG:
					univ.town.set_lg_slime(item_loc.x,item_loc.y,true);
					break;
				case eRace::STONE:
					univ.town.set_rubble(item_loc.x,item_loc.y,true);
					break;
				default:
					univ.town.set_lg_blood(item_loc.x,item_loc.y,true);
					break;
			}
		}
		
		if(overall_mode != MODE_OUTDOORS)
			for(cItem& item : which_pc.items)
				if(item.variety != eItemType::NO_ITEM) {
					dummy = place_item(item,item_loc);
					(void) dummy;
					item.variety = eItemType::NO_ITEM;
				}
		if(type == eMainStatus::DEAD || type == eMainStatus::DUST)
			play_sound(21);
		which_pc.main_status = type;
		which_pc.ap = 0;
	}
	else {
		add_string_to_buf("  Life saved!");
		which_pc.take_item(weap.slot);
		which_pc.heal(200);
	}
	if(univ.current_pc().main_status != eMainStatus::ALIVE)
		univ.cur_pc = first_active_pc();
	put_pc_screen();
	set_stat_window_for_pc(univ.cur_pc);
}

void set_pc_moves() {
	for(short i = 0; i < 6; i++)
		if(univ.party[i].main_status != eMainStatus::ALIVE)
			univ.party[i].ap = 0;
		else {
			univ.party[i].ap = univ.party[i].traits[eTrait::SLUGGISH] ? 3 : 4;
			short r = univ.party[i].total_encumbrance(hit_chance);
			univ.party[i].ap = minmax(1,8,univ.party[i].ap - (r / 3));
			
			if(int speed = univ.party[i].get_prot_level(eItemAbil::SPEED))
				univ.party[i].ap += speed;
			univ.party[i].ap -= univ.party[i].get_prot_level(eItemAbil::SLOW_WEARER);
			
			if(univ.party[i].status[eStatus::HASTE_SLOW] < 0 && univ.party.age % 2 == 1) // slowed?
				univ.party[i].ap = 0;
			else { // do webs
				univ.party[i].ap = max(0,univ.party[i].ap - univ.party[i].status[eStatus::WEBS] / 2);
				if(univ.party[i].ap == 0) {
					add_string_to_buf(univ.party[i].name + " must clean webs.");
					univ.party[i].status[eStatus::WEBS] = max(0,univ.party[i].status[eStatus::WEBS] - 3);
				}
			}
			if(univ.party[i].status[eStatus::HASTE_SLOW] > 7)
				univ.party[i].ap = univ.party[i].ap * 3;
			else if(univ.party[i].status[eStatus::HASTE_SLOW] > 0)
				univ.party[i].ap = univ.party[i].ap * 2;
			if(univ.party[i].status[eStatus::ASLEEP] > 0 || univ.party[i].status[eStatus::PARALYZED] > 0)
				univ.party[i].ap = 0;
			
		}
	
}

void take_ap(short num) {
	univ.current_pc().ap = max(0,univ.current_pc().ap - num);
}

short trait_present(eTrait which_trait) {
	short ret = 0;
	for(short i = 0; i < 6; i++)
		if(univ.party[i].main_status == eMainStatus::ALIVE && univ.party[i].traits[which_trait])
			ret += 1;
	return ret;
}

short race_present(eRace which_race) {
	short ret = 0;
	for(short i = 0; i < 6; i++)
		if(univ.party[i].main_status == eMainStatus::ALIVE && univ.party[i].race == which_race)
			ret += 1;
	return ret;
}

short wilderness_lore_present(ter_num_t ter_type) {
	cTerrain& ter = univ.scenario.ter_types[ter_type];
	if(ter.special == eTerSpec::WILDERNESS_CAVE || ter.special == eTerSpec::WATERFALL_CAVE)
		return trait_present(eTrait::CAVE_LORE);
	else if(ter.special == eTerSpec::WILDERNESS_SURFACE || ter.special == eTerSpec::WATERFALL_SURFACE)
		return trait_present(eTrait::WOODSMAN);
	return false;
}

short party_size(bool only_living) {
	short num_pcs = 0;
	for(short i = 0; i < 6; i++) {
		if(!only_living) {
			if(univ.party[i].main_status != eMainStatus::ABSENT)
				num_pcs++;
		}
		else {
			if(univ.party[i].main_status == eMainStatus::ALIVE)
				num_pcs++;		
		}
	}
	
	return num_pcs;
	
}
