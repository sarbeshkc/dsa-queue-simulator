// src/core/window.h
#ifndef WINDOW_H
#define WINDOW_H

#include <SDL3/SDL.h>
#include <string>

class Window {
public:
    // Initialize window with given title and dimensions
    Window(const std::string& title, int width, int height);
    ~Window();

    // Main window operations
    void clear() const;       // Clear the rendering canvas
    void present() const;     // Display the rendered content

    // Access underlying SDL components
    SDL_Renderer* getRenderer() { return m_renderer; }
    SDL_Window* getWindow() { return m_window; }

private:
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    int m_width;
    int m_height;
};

#endif // WINDOW_H
