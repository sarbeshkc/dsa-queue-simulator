

#ifndef TEXT_H
#define TEXT_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

class Text {
public:
  Text(SDL_Renderer *renderer, TTF_Font *font, const std::string &text,
       SDL_Color color);
  ~Text();

  void setText(const std::string &text);
  void setColor(SDL_Color color);
  void render(int x, int y) const;

private:
  SDL_Renderer *m_renderer;
  TTF_Font *m_font;
  SDL_Texture *m_texture;
  std::string m_text;
  SDL_Color m_color;
  int m_width;
  int m_height;

  void createTexture();
};

#endif // TEXT_H
