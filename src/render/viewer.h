#pragma once

#include "view_data.h"

/*
    The window with the scene: lighting, red intersecting triangles, the frame of the cube, a flying camera
    Returns when the window is closed. Throws std::runtime_error if the window can not be created

    Keys:   W S A D - fly forward, back, left, right     Q E - down, up      Shift - faster
            left mouse button + move the mouse - look around
            O - the camera flies around the scene by itself      R - back to the start      Esc - exit
*/
int run_viewer(const ViewData& data);
