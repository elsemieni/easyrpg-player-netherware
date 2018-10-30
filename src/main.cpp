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

#include "player.h"
#include <cstdlib>

/*#ifdef STEAMSHIM_WIN
#	if defined(_WIN32)
#		include "../steamshim/windows/steamshim_child.h"
#	else
#		include "../steamshim/generic/steamshim_child.h"
#	endif
#endif*/

#ifdef STEAMSHIM_WIN
#   include "../steamshim/windows/steamshim_child.h"
#endif

// This is needed on Windows
#ifdef USE_SDL
#  include <SDL.h>
#endif

extern "C" int main(int argc, char* argv[]) {

#ifdef STEAMSHIM_WIN
	//Initialize steamshim
	if (!STEAMSHIM_init()) {
		printf("Please run Game instead\n");
		return EXIT_FAILURE;
	}

	STEAMSHIM_requestStats();

#endif
	Player::Init(argc, argv);
	Player::Run();

#ifdef STEAMSHIM_WIN
	//Shutdown steamshim
    STEAMSHIM_deinit();
#endif

	return EXIT_SUCCESS;
}
