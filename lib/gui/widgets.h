#ifndef WIDGETS_H
#define WIDGETS_H

#include "types.h"
#include <string>

bool button(float x, float y, float w, float h, const std::string& label, double mx, double my, bool click, Color bg = {-1,-1,-1,1}, float fontSize = 16.0f);
void draw_textbox(TextBox& tb, float x, float y, float w, float h,
                  double mx, double my, bool click, float fontSize = 1.0f);
void draw_card(float x, float y, float w, float h, const std::string& title);

void icon_menu(float x, float y);
void icon_home(float x, float y);

#endif
