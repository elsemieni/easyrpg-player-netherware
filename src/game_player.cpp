/*
 * This file is part of EasyRPG Player.
 *
 * EasyRPG Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EasyRPG Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EasyRPG Player. If not, see <http://www.gnu.org/licenses/>.
 */

// Headers
#include "game_player.h"
#include "async_handler.h"
#include "game_actor.h"
#include "game_map.h"
#include "game_message.h"
#include "game_party.h"
#include "game_system.h"
#include "game_temp.h"
#include "graphics.h"
#include "input.h"
#include "main_data.h"
#include "player.h"
#include "util_macro.h"
#include "game_switches.h"
#include "output.h"
#include "reader_util.h"
#include <algorithm>
#include <cmath>

Game_Player::Game_Player():
	location(Main_Data::game_data.party_location) {
	SetDirection(RPG::EventPage::Direction_down);
	SetMoveSpeed(4);
}

int Game_Player::GetX() const {
	return location.position_x;
}

void Game_Player::SetX(int new_x) {
	location.position_x = new_x;
}

int Game_Player::GetY() const {
	return location.position_y;
}

void Game_Player::SetY(int new_y) {
	location.position_y = new_y;
}

int Game_Player::GetScreenZ() const {
	// Player is always slightly above events
	// (and always on "same layer as hero" obviously)
	return Game_Character::GetScreenZ() + 1;
}

int Game_Player::GetMapId() const {
	return location.map_id;
}

void Game_Player::SetMapId(int new_map_id) {
	location.map_id = new_map_id;
}

int Game_Player::GetDirection() const {
	return location.direction;
}

void Game_Player::SetDirection(int new_direction) {
	location.direction = new_direction;
}

int Game_Player::GetSpriteDirection() const {
	return location.sprite_direction;
}

void Game_Player::SetSpriteDirection(int new_direction) {
	location.sprite_direction = new_direction;
}

bool Game_Player::IsFacingLocked() const {
	return location.lock_facing;
}

void Game_Player::SetFacingLocked(bool locked) {
	location.lock_facing = locked;
}

int Game_Player::GetLayer() const {
	return location.layer;
}

void Game_Player::SetLayer(int new_layer) {
	location.layer = new_layer;
}

int Game_Player::GetMoveSpeed() const {
	return location.move_speed;
}

void Game_Player::SetMoveSpeed(int speed) {
	location.move_speed = speed;
}

int Game_Player::GetMoveFrequency() const {
	return location.move_frequency;
}

void Game_Player::SetMoveFrequency(int frequency) {
	location.move_frequency = frequency;
}

const RPG::MoveRoute& Game_Player::GetMoveRoute() const {
	return location.move_route;
}

void Game_Player::SetMoveRoute(const RPG::MoveRoute& move_route) {
	location.move_route = move_route;
}

int Game_Player::GetOriginalMoveRouteIndex() const {
	return 0;
}

void Game_Player::SetOriginalMoveRouteIndex(int /* new_index */) {
	// no-op
}

int Game_Player::GetMoveRouteIndex() const {
	return location.move_route_index;
}

void Game_Player::SetMoveRouteIndex(int new_index) {
	location.move_route_index = new_index;
}

bool Game_Player::IsMoveRouteOverwritten() const {
	return location.move_route_overwrite;
}

void Game_Player::SetMoveRouteOverwritten(bool force) {
	location.move_route_overwrite = force;
}

bool Game_Player::IsMoveRouteRepeated() const {
	return location.move_route_repeated;
}

void Game_Player::SetMoveRouteRepeated(bool force) {
	location.move_route_repeated = force;
}

const std::string& Game_Player::GetSpriteName() const {
	return location.sprite_name;
}

void Game_Player::SetSpriteName(const std::string& sprite_name) {
	location.sprite_name = sprite_name;
}

int Game_Player::GetSpriteIndex() const {
	return location.sprite_id;
}

void Game_Player::SetSpriteIndex(int index) {
	location.sprite_id = index;
}

bool Game_Player::GetVisible() const {
	return visible && !location.aboard;
}

Color Game_Player::GetFlashColor() const {
	return Color(location.flash_red, location.flash_green, location.flash_blue, flash_alpha);
}

void Game_Player::SetFlashColor(const Color& flash_color) {
	location.flash_red = flash_color.red;
	location.flash_blue = flash_color.blue;
	location.flash_green = flash_color.green;
	flash_alpha = flash_color.alpha;
}

double Game_Player::GetFlashLevel() const {
	return location.flash_current_level;
}

void Game_Player::SetFlashLevel(double flash_level) {
	location.flash_current_level = flash_level;
}

int Game_Player::GetFlashTimeLeft() const {
	return location.flash_time_left;
}

void Game_Player::SetFlashTimeLeft(int time_left) {
	location.flash_time_left = time_left;
}

bool Game_Player::GetThrough() const {
	return location.through;
}

void Game_Player::SetThrough(bool through) {
	location.through = through;
}

void Game_Player::ReserveTeleport(int map_id, int x, int y, int direction) {
	new_map_id = map_id;
	new_x = x;
	new_y = y;
	new_direction = direction;

	FileRequestAsync* request = Game_Map::RequestMap(new_map_id);
	request->SetImportantFile(true);
	request->Start();
}

void Game_Player::ReserveTeleport(const RPG::SaveTarget& target) {
	int map_id = target.map_id;

	if (Game_Map::GetMapType(target.map_id) == RPG::TreeMap::MapType_area) {
		// Area: Obtain the map the area belongs to
		map_id = Game_Map::GetParentId(target.map_id);
	}

	ReserveTeleport(map_id, target.map_x, target.map_y, Down);

	if (target.switch_on) {
		Game_Switches[target.switch_id] = true;
		Game_Map::SetNeedRefresh(Game_Map::Refresh_All);
	}
}

void Game_Player::StartTeleport() {
	teleporting = true;
}

void Game_Player::PerformTeleport() {
	if (!teleporting) return;

	teleporting = false;

	// Finish (un)boarding process
	if (location.boarding) {
		location.boarding = false;
		location.aboard = true;
	} else if (location.unboarding) {
		location.unboarding = false;
		location.aboard = false;
	}

	// Reset sprite if it was changed by a move
	// Even when target is the same map
	Refresh();

	if (Game_Map::GetMapId() != new_map_id) {
		pattern = RPG::EventPage::Frame_middle;
		Game_Map::Setup(new_map_id);
		last_pan_x = 0;
		last_pan_y = 0;
	}

	SetOpacity(255);

	MoveTo(new_x, new_y);
	if (new_direction >= 0) {
		SetDirection(new_direction);
		SetSpriteDirection(new_direction);
	}

	if (InVehicle())
		GetVehicle()->MoveTo(new_x, new_y);
}

bool Game_Player::MakeWay(int x, int y, int d) const {
	if (Player::debug_flag && Input::IsPressed(Input::DEBUG_THROUGH))
		return true;

	if (location.aboard)
		return GetVehicle()->MakeWay(x, y, d);

	return Game_Character::MakeWay(x, y, d);
}

bool Game_Player::IsTeleporting() const {
	return teleporting;
}

void Game_Player::Center() {
	int x = GetSpriteX() + SCREEN_TILE_WIDTH;
	int y = GetSpriteY() + SCREEN_TILE_WIDTH / 2;

	int dist_to_screen_left = -SCREEN_WIDTH / 2;
	int dist_to_screen_top = -SCREEN_HEIGHT / 2;

	Game_Map::AddScreenX(x, dist_to_screen_left);
	Game_Map::AddScreenY(y, dist_to_screen_top);

	actual_pan_x = Game_Map::GetPanX();
	actual_pan_y = Game_Map::GetPanY();

	Game_Map::AddScreenX(x, actual_pan_x);
	Game_Map::AddScreenY(y, actual_pan_y);

	Game_Map::SetPositionX(x);
	Game_Map::SetPositionY(y);
}

void Game_Player::MoveTo(int x, int y) {
	x = max(0, min(x, Game_Map::GetWidth() - 1));
	y = max(0, min(y, Game_Map::GetHeight() - 1));

	Game_Character::MoveTo(x, y);
	Center();
	Game_Map::Parallax::ResetPosition();
}

void Game_Player::UpdateScroll() {
	// First, update for the player's movement...

	int center_x = DisplayUi->GetWidth() / 2 - TILE_SIZE / 2 - actual_pan_x / (SCREEN_TILE_WIDTH / TILE_SIZE);
	int center_y = DisplayUi->GetHeight() / 2 + TILE_SIZE / 2 - actual_pan_y / (SCREEN_TILE_WIDTH / TILE_SIZE);

	int dx = 0;
	int dy = 0;

	if (!Game_Map::IsPanLocked()) {
		if (IsMoving() || last_remaining_move > 0) {
			if (last_remaining_move == 0)
				last_remaining_move = SCREEN_TILE_WIDTH;

			int d = GetDirection();
			if ((d == Right || d == UpRight || d == DownRight) && GetScreenX() > center_x)
				dx = 1;
			else if ((d == Left || d == UpLeft || d == DownLeft) && GetScreenX() < center_x)
				dx = -1;
			dx *= last_remaining_move - remaining_step;

			if ((d == Down || d == DownRight || d == DownLeft) && GetScreenY() > center_y)
				dy = 1;
			else if ((d == Up || d == UpRight || d == UpLeft) && GetScreenY() < center_y)
				dy = -1;
			dy *= last_remaining_move - remaining_step;
			last_remaining_move = remaining_step;

		} else if (IsJumping() || last_remaining_jump > 0) {
			if (last_remaining_jump == 0)
				last_remaining_jump = SCREEN_TILE_WIDTH;

			if ((GetX() > jump_x && GetScreenX() > center_x) || (GetX() < jump_x && GetScreenX() < center_x))
				dx = (GetX() - jump_x) * (last_remaining_jump - remaining_step);
			if ((GetY() > jump_y && GetScreenY() > center_y) || (GetY() < jump_y && GetScreenY() < center_y))
				dy = (GetY() - jump_y) * (last_remaining_jump - remaining_step);
			last_remaining_jump = remaining_step;
		}
	}

	Game_Map::ScrollRight(dx);
	Game_Map::ScrollDown(dy);

	// Second, update for the change in pan...

	int pan_x = Game_Map::GetPanX();
	int pan_y = Game_Map::GetPanY();
	int pan_dx = pan_x - last_pan_x;
	int pan_dy = pan_y - last_pan_y;
	last_pan_x = pan_x;
	last_pan_y = pan_y;

	// Change pan_dx/pan_dy to account for hitting the edges
	int screen_x = Game_Map::GetPositionX();
	int screen_y = Game_Map::GetPositionY();
	Game_Map::AddScreenX(screen_x, pan_dx);
	Game_Map::AddScreenY(screen_y, pan_dy);

	// Only move for the pan if we're closer to the target pan than we were before.
	if (std::abs(actual_pan_x + pan_dx - Game_Map::GetTargetPanX()) < std::abs(actual_pan_x - Game_Map::GetTargetPanX())) {
		Game_Map::ScrollRight(pan_dx);
		actual_pan_x += pan_dx;
	}
	if (std::abs(actual_pan_y + pan_dy - Game_Map::GetTargetPanY()) < std::abs(actual_pan_y - Game_Map::GetTargetPanY())) {
		Game_Map::ScrollDown(pan_dy);
		actual_pan_y += pan_dy;
	}
}

void Game_Player::Update() {
	int cur_frame_count = Player::GetFrames();
	// Only update the event once per frame
	if (cur_frame_count == frame_count_at_last_update_parallel) {
		return;
	}
	frame_count_at_last_update_parallel = cur_frame_count;

	bool last_moving = IsMoving() || IsJumping();

	// Workaround: If a blocking move route ends in this frame, Game_Player::CancelMoveRoute decides
	// which events to start. was_blocked is used to avoid triggering events the usual way.
	bool was_blocked = IsBlockedByMoveRoute();
	Game_Character::Update();

	if (!Game_Map::GetInterpreter().IsRunning() && !Game_Map::IsAnyEventStarting()) {
		if (IsMovable()) {
			switch (Input::dir4) {
				case 2:
					Move(Down);
					break;
				case 4:
					Move(Left);
					break;
				case 6:
					Move(Right);
					break;
				case 8:
					Move(Up);
			}
		}

		// ESC-Menu calling
		if (Game_System::GetAllowMenu() && !Game_Message::message_waiting && !IsBlockedByMoveRoute() && Input::IsTriggered(Input::CANCEL)) {
			Game_Temp::menu_calling = true;
		}
	}

	Game_Character::UpdateSprite();
	UpdateScroll();

	if (IsMoving() || was_blocked) return;

	if (last_moving && location.boarding) {
		// Boarding completed
		location.aboard = true;
		location.boarding = false;
		SetMoveSpeed(GetVehicle()->GetMoveSpeed());
		GetVehicle()->SetDirection(GetDirection());
		GetVehicle()->SetSpriteDirection(GetSpriteDirection());
		return;
	}

	if (last_moving && location.unboarding) {
		// Unboarding completed
		location.unboarding = false;
		CheckTouchEvent();
		return;
	}

	if (last_moving && CheckTouchEvent()) return;

	if (!Game_Map::GetInterpreter().IsRunning() && !Game_Map::IsAnyEventStarting()) {
		if (!Game_Message::visible && Input::IsTriggered(Input::DECISION)) {
			if ( GetOnOffVehicle() ) return;
			if ( CheckActionEvent() ) return;
		}
	}

	if (last_moving)
		Game_Map::UpdateEncounterSteps();
}

bool Game_Player::CheckActionEvent() {
	if (InAirship())
		return false;

	// Use | instead of || to avoid short-circuit evaluation
	return CheckEventTriggerHere({RPG::EventPage::Trigger_action}, true)
		| CheckEventTriggerThere({RPG::EventPage::Trigger_action,
		RPG::EventPage::Trigger_touched, RPG::EventPage::Trigger_collision}, true);
}

bool Game_Player::CheckTouchEvent() {
	if (InAirship())
		return false;

	if (IsMoveRouteOverwritten())
		return false;

	return CheckEventTriggerHere({RPG::EventPage::Trigger_touched});
}

bool Game_Player::CheckCollisionEvent() {
	if (InAirship())
		return false;
	return CheckEventTriggerHere({RPG::EventPage::Trigger_collision});
}

bool Game_Player::CheckEventTriggerHere(const std::vector<int>& triggers, bool triggered_by_decision_key) {
	bool result = false;

	std::vector<Game_Event*> events;
	Game_Map::GetEventsXY(events, GetX(), GetY());

	std::vector<Game_Event*>::iterator i;
	for (i = events.begin(); i != events.end(); ++i) {
		if (((*i)->GetLayer() != RPG::EventPage::Layers_same)
			&& std::find(triggers.begin(), triggers.end(), (*i)->GetTrigger() ) != triggers.end() ) {
			(*i)->Start(triggered_by_decision_key);
			result = (*i)->GetStarting();
		}
	}
	return result;
}

bool Game_Player::CheckEventTriggerThere(const std::vector<int>& triggers, bool triggered_by_decision_key) {
	if ( Game_Map::GetInterpreter().IsRunning() ) return false;

	bool result = false;

	int front_x = Game_Map::XwithDirection(GetX(), GetDirection());
	int front_y = Game_Map::YwithDirection(GetY(), GetDirection());

	std::vector<Game_Event*> events;
	Game_Map::GetEventsXY(events, front_x, front_y);

	for (const auto& ev : events) {
		if ( ev->GetLayer() == RPG::EventPage::Layers_same &&
			std::find(triggers.begin(), triggers.end(), ev->GetTrigger() ) != triggers.end()
		)
		{
			if (!ev->GetList().empty()) {
				ev->StartTalkToHero();
			}
			ev->Start(triggered_by_decision_key);
			result = true;
		}
	}

	if ( !result && Game_Map::IsCounter(front_x, front_y) ) {
		front_x = Game_Map::XwithDirection(front_x, GetDirection());
		front_y = Game_Map::YwithDirection(front_y, GetDirection());

		Game_Map::GetEventsXY(events, front_x, front_y);

		for (const auto& ev : events) {
			if ( ev->GetLayer() == 1 &&
				std::find(triggers.begin(), triggers.end(), ev->GetTrigger() ) != triggers.end()
			)
			{
				if (!ev->GetList().empty()) {
					ev->StartTalkToHero();
				}
				ev->Start(triggered_by_decision_key);
				result = true;
			}
		}
	}
	return result;
}

bool Game_Player::CheckEventTriggerTouch(int x, int y) {
	if ( Game_Map::GetInterpreter().IsRunning() ) return false;

	bool result = false;

	std::vector<Game_Event*> events;
	Game_Map::GetEventsXY(events, x, y);

	for (const auto& ev : events) {
		if (ev->GetLayer() == RPG::EventPage::Layers_same &&
			(ev->GetTrigger() == RPG::EventPage::Trigger_touched ||
			ev->GetTrigger() == RPG::EventPage::Trigger_collision) ) {
			if (!ev->GetList().empty()) {
				ev->StartTalkToHero();
			}
			ev->Start();
			result = true;

		}
	}
	return result;
}

void Game_Player::Refresh() {
	Game_Actor* actor;

	if (Main_Data::game_party->GetActors().empty()) {
		SetSpriteName("");
		return;
	}

	actor = Main_Data::game_party->GetActors()[0];

	SetSpriteName(actor->GetSpriteName());
	SetSpriteIndex(actor->GetSpriteIndex());

	if (location.aboard)
		GetVehicle()->SyncWithPlayer();
}

bool Game_Player::GetOnOffVehicle() {
	if (!IsMovable())
		return false;

	if (InVehicle())
		return GetOffVehicle();
	return GetOnVehicle();
}

bool Game_Player::GetOnVehicle() {
	int front_x = Game_Map::XwithDirection(GetX(), GetDirection());
	int front_y = Game_Map::YwithDirection(GetY(), GetDirection());
	Game_Vehicle::Type type;

	if (Game_Map::GetVehicle(Game_Vehicle::Airship)->IsInPosition(GetX(), GetY()))
		type = Game_Vehicle::Airship;
	else if (Game_Map::GetVehicle(Game_Vehicle::Ship)->IsInPosition(front_x, front_y))
		type = Game_Vehicle::Ship;
	else if (Game_Map::GetVehicle(Game_Vehicle::Boat)->IsInPosition(front_x, front_y))
		type = Game_Vehicle::Boat;
	else
		return false;

	location.vehicle = type;
	location.preboard_move_speed = GetMoveSpeed();
	if (type != Game_Vehicle::Airship) {
		location.boarding = true;
		if (!GetThrough()) {
			SetThrough(true);
			MoveForward();
			SetThrough(false);
		} else {
			MoveForward();
		}
	} else {
		location.aboard = true;
		SetMoveSpeed(GetVehicle()->GetMoveSpeed());
		SetDirection(RPG::EventPage::Direction_left);
	}

	walking_bgm = Game_System::GetCurrentBGM();
	GetVehicle()->GetOn();
	return true;
}

bool Game_Player::GetOffVehicle() {
	if (!InAirship()) {
		int front_x = Game_Map::XwithDirection(GetX(), GetDirection());
		int front_y = Game_Map::YwithDirection(GetY(), GetDirection());
		if (!CanWalk(front_x, front_y)
			&& !Game_Map::GetVehicle(Game_Vehicle::Boat)->IsInPosition(front_x, front_y)
			&& !Game_Map::GetVehicle(Game_Vehicle::Ship)->IsInPosition(front_x, front_y))
			return false;
	}

	GetVehicle()->GetOff();
	return true;
}

bool Game_Player::IsStopping() const {
	// Prevent MoveRoute execution while the airship is ascending/descending (Issue #1268)
	if (InAirship() && !GetVehicle()->IsMovable())
		return false;
	return Game_Character::IsStopping();
}

bool Game_Player::IsMovable() const {
	if (IsMoving() || IsJumping())
		return false;
	if (Graphics::IsTransitionPending())
		return false;
	if (IsMoveRouteOverwritten())
		return false;
	if (location.boarding || location.unboarding)
		return false;
	if (Game_Message::message_waiting)
		return false;
	if (InAirship() && !GetVehicle()->IsMovable())
		return false;
    return true;
}

bool Game_Player::IsBlockedByMoveRoute() const {
	if (!IsMoveRouteOverwritten())
		return false;

	// Check if it includes a blocking move command
	for (const auto& move_command : GetMoveRoute().move_commands) {
		int code = move_command.command_id;
		if ((code <= RPG::MoveCommand::Code::move_forward) || // Move
			(code <= RPG::MoveCommand::Code::face_away_from_hero && GetMoveFrequency() < 8) || // Turn
			(code == RPG::MoveCommand::Code::wait || code == RPG::MoveCommand::Code::begin_jump)) // Wait or jump
				return true;
	}

	return false;
}

bool Game_Player::InVehicle() const {
	return location.vehicle > 0;
}

bool Game_Player::InAirship() const {
	return location.vehicle == Game_Vehicle::Airship;
}

Game_Vehicle* Game_Player::GetVehicle() const {
	return Game_Map::GetVehicle((Game_Vehicle::Type) location.vehicle);
}

bool Game_Player::CanWalk(int x, int y) {
	return Game_Map::IsPassable(x, y, GetDirection(), this);
}

void Game_Player::BeginMove() {
	int terrain_id = Game_Map::GetTerrainTag(GetX(), GetY());
	const RPG::Terrain* terrain = ReaderUtil::GetElement(Data::terrains, terrain_id);
	bool red_flash = false;

	if (terrain) {
		if (!InAirship()) {
			if (!terrain->on_damage_se || (terrain->on_damage_se && (terrain->damage > 0))) {
				Game_System::SePlay(terrain->footstep);
			}
			if (terrain->damage > 0) {
				for (auto hero : Main_Data::game_party->GetActors()) {
					if (!hero->PreventsTerrainDamage()) {
						red_flash = true;
						hero->ChangeHp(-std::max<int>(0, std::min<int>(terrain->damage, hero->GetHp() - 1)));
					}
				}
			}
		}
	} else {
		Output::Warning("Player BeginMove: Invalid terrain ID %d at (%d, %d)", terrain_id, GetX(), GetY());
	}

	red_flash = red_flash || Main_Data::game_party->ApplyStateDamage();

	if (red_flash) {
		Main_Data::game_screen->FlashOnce(31, 10, 10, 19, 6);
	}
}

void Game_Player::CancelMoveRoute() {
	if (!IsMoveRouteOverwritten())
		return;

	// Bugfix: Moved up from end of function. The fix for #1051 in CheckTouchEvent made the Touch check always returning
	// false because the MoveRoute was still marked as overwritten
	Game_Character::CancelMoveRoute();

	// If the last executed command of the move route was a Move command, check touch and collision triggers
	const RPG::MoveRoute& active_route = GetMoveRoute();

	int index = GetMoveRouteIndex();
	if (!active_route.move_commands.empty()) {
		int move_size = static_cast<int>(active_route.move_commands.size());
		if (index >= move_size) {
			index = move_size - 1;
		}

		// Touch/Collision events are only triggered after the end of a move route when the last command of the move
		// route was any movement command.
		// "any_move_successful" handles the corner case that the last command was a movement but the Player never
		// changed the tile (because the way was blocked), then no event handling occurs.
		if (active_route.move_commands[index].command_id <= RPG::MoveCommand::Code::move_forward && any_move_successful) {
			CheckTouchEvent();
			CheckCollisionEvent();
		}
	}
}

void Game_Player::Unboard() {
	location.aboard = false;
	SetMoveSpeed(location.preboard_move_speed);

	Game_System::BgmPlay(walking_bgm);
}

bool Game_Player::IsBoardingOrUnboarding() const {
	return location.boarding || location.unboarding;
}

void Game_Player::UnboardingFinished() {
	Unboard();
	if (InAirship()) {
		SetDirection(RPG::EventPage::Direction_down);
	} else {
		location.unboarding = true;
		if (!GetThrough()) {
			SetThrough(true);
			MoveForward();
			SetThrough(false);
		} else {
			MoveForward();
		}
	}
	location.vehicle = Game_Vehicle::None;
}
