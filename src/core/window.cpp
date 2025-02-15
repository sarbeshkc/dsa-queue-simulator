// src/core/window.cpp
#include "window.h"
#include <stdexcept>

// src/core/window.cpp
Window::Window(const std::string &title, int width, int height)
    : m_width(width), m_height(height) {

  m_window =
      SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_RESIZABLE);

  if (!m_window) {
    throw std::runtime_error(SDL_GetError());
  }

  // SDL3 uses different renderer flags
  m_renderer = SDL_CreateRenderer(m_window, nullptr);

  if (!m_renderer) {
    SDL_DestroyWindow(m_window);
    throw std::runtime_error(SDL_GetError());
  }
}
Window::~Window() {
  // Clean up SDL resources
  SDL_DestroyRenderer(m_renderer);
  SDL_DestroyWindow(m_window);
}

void Window::clear() const {
  // Set background color (dark gray)
  SDL_SetRenderDrawColor(m_renderer, 40, 40, 40, 255);
  SDL_RenderClear(m_renderer);
}

void Window::present() const { SDL_RenderPresent(m_renderer); }
