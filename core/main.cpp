#include "main.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3_ttf/SDL_ttf.h"
#include "text.h"
#include <SDL3/SDL_render.h>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>

namespace GameVariable {
constexpr int WindowHeight = 720;
constexpr int WindowLength = 1280;
constexpr int TextPixel = 30;
constexpr SDL_Color white = {255, 255, 255, 255};

} // namespace GameVariable

App::App()
    : m_window("Traffic Simulator", GameVariable::WindowLength,
               GameVariable::WindowHeight) {

  if (TTF_Init() < 0) {
    throw std::runtime_error(SDL_GetError());
  }

  m_font = TTF_OpenFont("../assets/fonts/DaddyTimeMonoNerdFontMono-Regular.ttf",
                        GameVariable::TextPixel);
  if (!m_font) {
    throw std::runtime_error(SDL_GetError());
  }

  m_text = std::make_unique<Text>(m_window.renderer(), m_font);
  m_text->setText("Traffic Simulator", GameVariable::white);

  // Initialize other sub system
}

App::~App() {
  SDL_Quit();
  // Clean up
}

void App::run() {
  while (m_running) {
    process_event();
    update();
    render();
  }
}

void App::process_event() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      m_running = false;
    }
  }
}

void App::update() {}
void App::render() {

  m_window.clear();

  m_text->render(20, 30);

  // Rendering code goes here
  m_window.present();
}

int main(int argc, const char *argv[]) {

  (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));

  TTF_Font *font =
      TTF_OpenFont("../assets/fonts/DaddyTimeMonoNerdFontMono-Regular.ttf", 10);
  try {
    App app;
    app.run();

  } catch (const std::exception &e) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: %s\n", e.what());
  }

  return 0;
}
