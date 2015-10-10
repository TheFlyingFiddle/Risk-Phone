/*
 * game.h
 *
 *  Created on: Feb 22, 2014
 *      Author: Lukas_2
 */

#ifndef GAME_H_
#define GAME_H_

#define ORIENTATION_LANDSCAPE ((uint8_t)0)
#define ORIENTATION_PORTRAIT  ((uint8_t)1)

#include "types.h"
#include "time_helpers.h"
#include "lua_core_private.h"
#include "new_renderer.h"
#include "luajit.h"
#include "display.h"

extern "C"
{
    typedef struct
    {
        vec3f acceleration;
    } SensorState;

	typedef struct
	{
		uint32_t width, height;
	} Screen;

	typedef struct
	{
		Clock* clock;
		SensorState* sensor;
		Screen* screen;
		uint32_t fps;
		lua_State* L;
	} Game;

	extern Game* gGame;
}

bool gameInitialized();

void gameInitialize(uint32_t screenWidth, uint32_t screenHeight);
void gameTerminate();
void gameStop();

void gameStep(ndk_helper::GLContext *engine);
void gameFinish();

#endif /* GAME_H_ */
