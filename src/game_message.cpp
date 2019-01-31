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
#include "game_message.h"
#include "game_player.h"
#include "game_temp.h"
#include "main_data.h"
#include "font.h"
#include "player.h"
#include "output.h"

//#include <regex>

namespace Game_Message {
	std::vector<std::string> texts;
	bool is_word_wrapped;

	unsigned int owner_id;

	int choice_start;
	int num_input_start;

	int choice_max;
	std::bitset<8> choice_disabled;

	int choice_cancel_type;

	int num_input_variable_id;
	int num_input_digits_max;

	bool message_waiting;
	bool visible;

	int choice_result;
}

RPG::SaveSystem& data = Main_Data::game_data.system;

void Game_Message::Init() {
	FullClear();
}

void Game_Message::SemiClear() {
	texts.clear();
	choice_disabled.reset();
	choice_start = 99;
	choice_max = 0;
	choice_cancel_type = 0;
	num_input_start = -1;
	num_input_variable_id = 0;
	num_input_digits_max = 0;
	is_word_wrapped = false; //netherware fix, desactivado por ahora
}

void Game_Message::FullClear() {
	SemiClear();
	SetFaceName("");
	SetFaceIndex(0);
}

std::string Game_Message::GetFaceName() {
	return data.face_name;
}

void Game_Message::SetFaceName(const std::string& face) {
	data.face_name = face;
}

int Game_Message::GetFaceIndex() {
	return data.face_id;
}

void Game_Message::SetFaceIndex(int index) {
	data.face_id = index;
}

bool Game_Message::IsFaceFlipped() {
	return data.face_flip;
}

void Game_Message::SetFaceFlipped(bool flipped) {
	data.face_flip = flipped;
}

bool Game_Message::IsFaceRightPosition() {
	return data.face_right;
}

void Game_Message::SetFaceRightPosition(bool right) {
	data.face_right = right;
}

bool Game_Message::IsTransparent() {
	if (Player::IsRPG2k() && Game_Temp::battle_running) {
		return false;
	}

	return data.message_transparent != 0;
}

void Game_Message::SetTransparent(bool transparent) {
	data.message_transparent = transparent;
}

int Game_Message::GetPosition() {
	return data.message_position;
}

void Game_Message::SetPosition(int new_position) {
	data.message_position = new_position;
}

bool Game_Message::IsPositionFixed() {
	return !data.message_prevent_overlap;
}

void Game_Message::SetPositionFixed(bool fixed) {
	data.message_prevent_overlap = !fixed;
}

bool Game_Message::GetContinueEvents() {
	return !!data.message_continue_events;
}

void Game_Message::SetContinueEvents(bool continue_events) {
	data.message_continue_events = continue_events;
}

int Game_Message::GetRealPosition() {
	if (Game_Temp::battle_running) {
		if (Player::IsRPG2k()) {
			return 2;
		}
		else {
			return 0;
		}
	}

	if (Game_Message::IsPositionFixed()) {
		return Game_Message::GetPosition();
	}
	else {
		// Move Message Box to prevent player hiding
		int disp = Main_Data::game_player->GetScreenY();

		switch (Game_Message::GetPosition()) {
		case 0: // Up
			return disp > (16 * 7) ? 0 : 2;
		case 1: // Center
			if (disp <= 16 * 7) {
				return 2;
			}
			else if (disp >= 16 * 10) {
				return 0;
			}
			else {
				return 1;
			}
		default: // Down
			return disp >= (16 * 10) ? 0 : 2;
		};
	}
}

int Game_Message::WordWrap(const std::string& line, int limit, const std::function<void(const std::string &line)> callback) {
	size_t start = 0;
	size_t lastfound = 0;
	int line_count = 0;
	bool end_of_string;
	FontRef font = Font::Default();
	Rect size;

	//netherware fix: se adapta para eliminar comandos
	//std::regex command_regex("\\\\[a-zA-Z$!.|<>^_](\\[[0-9]*\\])?");

	do {
		line_count++;
		size_t found = line.find(" ", start); //busca un espacio en la linea desde el punto de comienzo (el primero). Si no se encuentra, es -1
		std::string wrapped = line.substr(start, found - start); //extrae la palabra entre el principio y donde se encontro el espacio (wrapped).
		end_of_string = false; //no hay end of string aun
		do {
			lastfound = found; //ponemos que el ultimo encontrado es el que acabamos de encontrar
			found = line.find(" ", lastfound + 1); //seguimos buscando espacios desde el ultimo punto
			if (found == std::string::npos) { //si no encontramos espacios
				found = line.size(); //la linea llega hasta el final
			}
			wrapped = line.substr(start, found - start); //extraemos la palabra que acabamos de encontrar

			//limpiar wrapped de comandos maker
			//std::string clean_wrapped = std::regex_replace (wrapped,command_regex,"");
			//size = font->GetSize(clean_wrapped); //se obtiene sus dimensiones en pixeles
            size = font->GetSize(wrapped); //se obtiene sus dimensiones en pixeles
		} while (found < line.size() - 1 && size.width < limit); //repitase hasta que que haya llegado al final del string recorriendo, o hasta que el ultimo texto extraido exceda el limite puesto (???)
		if (found >= line.size() - 1) {
			// It's end of the string, not a word-break
			if (size.width < limit) {
				// And the last word of the string fits on the line
				// (otherwise do another word-break)
				lastfound = found;
				end_of_string = true;
			}
		}
		wrapped = line.substr(start, lastfound - start);
		callback(wrapped);
		start = lastfound + 1;
	} while (start < line.size() && !end_of_string);

	return line_count;
}
