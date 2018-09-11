//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//  Copyright (C) 2016 M. Teufel <mteufel@supertux.org>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "supertux/textscroller_screen.hpp"

#include <sstream>

#include "audio/sound_manager.hpp"
#include "control/input_manager.hpp"
#include "object/textscroller.hpp"
#include "supertux/fadetoblack.hpp"
#include "supertux/globals.hpp"
#include "supertux/info_box_line.hpp"
#include "supertux/screen_manager.hpp"
#include "util/log.hpp"
#include "util/reader.hpp"
#include "util/reader_document.hpp"
#include "util/reader_mapping.hpp"
#include "video/compositor.hpp"
#include "video/drawing_context.hpp"
#include "video/surface.hpp"
#include "video/video_system.hpp"
#include "video/viewport.hpp"

namespace {

const float DEFAULT_SPEED = 20;
const float SCROLL = 60;

} // namespace

TextScrollerScreen::TextScrollerScreen(const std::string& filename) :
  m_defaultspeed(DEFAULT_SPEED),
  m_music(),
  m_background(),
  m_text_scroller(),
  m_fading(false)
{
  std::string text;
  std::string background_file;

  try {
    auto doc = ReaderDocument::from_file(filename);
    auto root = doc.get_root();

    m_text_scroller = std::make_unique<TextScroller>(root);

    if (root.get_name() != "supertux-text") {
      throw std::runtime_error("File isn't a supertux-text file");
    } else {
      auto text_lisp = root.get_mapping();

      if (!text_lisp.get("background", background_file)) {
        throw std::runtime_error("File doesn't contain a background file");
      }

      text_lisp.get("speed", m_defaultspeed);
      text_lisp.get("music", m_music);
    }
  } catch (std::exception& e) {
    std::ostringstream msg;
    msg << "Couldn't load file '" << filename << "': " << e.what() << std::endl;
    throw std::runtime_error(msg.str());
  }

  // load background image
  m_background = Surface::from_file("images/background/" + background_file);
}

TextScrollerScreen::~TextScrollerScreen()
{
}

void
TextScrollerScreen::setup()
{
  SoundManager::current()->play_music(m_music);
  ScreenManager::current()->set_screen_fade(std::make_unique<FadeToBlack>(FadeToBlack::FADEIN, 0.5));
}

void
TextScrollerScreen::update(float elapsed_time)
{
  Controller* controller = InputManager::current()->get_controller();
  if (controller->hold(Controller::UP)) {
    m_text_scroller->set_speed(-m_defaultspeed * 5);
  } else if (controller->hold(Controller::DOWN)) {
    m_text_scroller->set_speed(m_defaultspeed * 5);
  } else {
    m_text_scroller->set_speed(m_defaultspeed);
  }

  if ((controller->pressed(Controller::JUMP) ||
       controller->pressed(Controller::ACTION) ||
       controller->pressed(Controller::MENU_SELECT)) &&
      !(controller->pressed(Controller::UP))) { // prevent skipping if jump with up is enabled
    m_text_scroller->scroll(SCROLL);
  }

  if (controller->pressed(Controller::START) ||
      controller->pressed(Controller::ESCAPE)) {
    ScreenManager::current()->pop_screen(std::make_unique<FadeToBlack>(FadeToBlack::FADEOUT, 0.5));
  }

  { // close when done
    if (m_text_scroller->is_finished() && !m_fading)
    {
      m_fading = true;
      ScreenManager::current()->pop_screen(std::make_unique<FadeToBlack>(FadeToBlack::FADEOUT, 0.5));
    }
  }

  m_text_scroller->update(elapsed_time);
}

void
TextScrollerScreen::draw(Compositor& compositor)
{
  auto& context = compositor.make_context();

  const float ctx_w = static_cast<float>(context.get_width());
  const float ctx_h = static_cast<float>(context.get_height());

  { // draw background
    const float bg_w = static_cast<float>(m_background->get_width());
    const float bg_h = static_cast<float>(m_background->get_height());

    const float bg_ratio = bg_w / bg_h;
    const float ctx_ratio = ctx_w / ctx_h;

    if (bg_ratio > ctx_ratio)
    {
      const float new_bg_w = ctx_h * bg_ratio;
      context.color().draw_surface_scaled(m_background,
                                          Rectf::from_center(Vector(ctx_w / 2.0f, ctx_h / 2.0f),
                                                             Sizef(new_bg_w, ctx_h)),
                                          0);
    }
    else
    {
      const float new_bg_h = ctx_w / bg_ratio;
      context.color().draw_surface_scaled(m_background,
                                          Rectf::from_center(Vector(ctx_w / 2.0f, ctx_h / 2.0f),
                                                             Sizef(ctx_w, new_bg_h)),
                                          0);
    }
  }

  m_text_scroller->draw(context);
}

/* EOF */