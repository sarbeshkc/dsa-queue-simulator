#include "text.h"
#include "SDL3_ttf/SDL_ttf.h"
#include <stdexcept>

Text::Text(SDL_Renderer *renderer, TTF_Font *font, const std::string &text,
           SDL_Color color)
    : m_renderer(renderer), m_font(font), m_text(text), m_color(color),
      m_texture(nullptr), m_width(0), m_height(0) {
  createTexture();
}
Text::~Text() {
  if (m_texture)
    SDL_DestroyTexture(m_texture);
}

void Text::setColor(SDL_Color color) {
  if (m_color.r != color.r || m_color.g != color.g || m_color.b != color.b ||
      m_color.a != color.a) {
    m_color = color;
    createTexture();
  }
}

void Text::setText(const std::string& text) {
  if (m_text != text) {
    m_text = text;
    createTexture();
  }
}

void Text::createTexture() {
  if (m_texture) {
    SDL_DestroyTexture(m_texture);
    m_texture = nullptr;
  }

  SDL_Surface* surface = TTF_RenderText_Solid(m_font, m_text.c_str(),m_text.size(), m_color);
  if(!surface) throw std::runtime_error(SDL_GetError());

  m_texture = SDL_CreateTextureFromSurface(m_renderer, surface);

  if (!m_texture) {
    SDL_DestroySurface(surface);
    throw std::runtime_error(SDL_GetError());
  }

  m_width = surface->w;
  m_height = surface->h;
  SDL_DestroySurface(surface);
}


void Text::render(int x, int y) const {
  if (!m_texture) return ;
    SDL_FRect dest = {
        static_cast<float>(x),
        static_cast<float>(y),
        static_cast<float>(m_width),
        static_cast<float>(m_height)
    };
  SDL_RenderTexture(m_renderer, m_texture ,nullptr, &dest);

}

