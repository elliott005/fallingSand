#pragma once

#include <SDL3/SDL.h>
#include <iostream>
#include <string.h>

class Application
{
public:
    Application(const char* title);
    ~Application();
    void updateScreen();
    SDL_Event m_window_event;
    SDL_Window* m_window = NULL;
    SDL_Surface* m_window_surface = NULL;
    SDL_Renderer* m_renderer = NULL;
    const int SCREEN_WIDTH = 2000;
    const int SCREEN_HEIGHT = 1300;
};