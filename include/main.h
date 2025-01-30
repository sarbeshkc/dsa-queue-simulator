#ifndef MAIN_H
#define MAIN_H

#include "SDL3_ttf/SDL_ttf.h"
#include "text.h"
#include "window.h"
#include <SDL3/SDL.h>
#include <memory>

class App {
public:
  App();
  ~App();

  void run();

private:
  void update();
  void render();
  void process_event();

  Window m_window;
  bool m_running = true;

  TTF_Font *m_font;
  std::unique_ptr<Text> m_text;
};

#endif // !MAIN_H
