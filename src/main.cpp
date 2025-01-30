#include "main.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_log.h"
#include "SDL3_ttf/SDL_ttf.h"
#include <SDL3/SDL_render.h>
#include <exception>
#include <memory>
#include <iostream>


App::App() : m_window("Traffic Simulator", 1280, 720) {

if (  TTF_Init() == -1){
    std::cout << "Error";

  }

  m_font == TTF_OpenFont("../assets/fonts/DaddyTimeMonoNerdFontMono-Regular.ttf", 24);
    m_text = std::make_unique<Text>(
        m_window.renderer(),
        m_font,
        "Hello, SDL!",
        SDL_Color{255, 255, 255, 255}
    );


  // Initialize other sub system
}

App::~App() {
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

  // Rendering code goes here
  m_window.present();
}

int main(int argc, const char *argv[]) {
  (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));
  try {
    App app;
    app.run();

  } catch (const std::exception &e) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Error: %s\n", e.what());
  }

  SDL_Quit();
  return 0;
}
