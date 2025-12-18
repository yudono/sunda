#include "widgets.h"
#include "renderer.h"
#include <iostream>

bool button(float x, float y, float w, float h, const std::string& label,
            double mx, double my, bool click, Color bgOverride, float fontSize) {
    bool hover = mx > x && mx < x+w && my > y && my < y+h;

    // Default Gray, Hover Blue. Or use override.
    Color base = (bgOverride.r == -1) 
                 ? (hover ? Color{0.6f, 0.6f, 0.6f, 1.0f} : Color{0.8f, 0.8f, 0.8f, 1.0f}) 
                 : bgOverride;
                 
    if (bgOverride.r != -1 && hover) {
        // slightly darken override on hover
        base.r *= 0.9f; base.g *= 0.9f; base.b *= 0.9f;
    }

    // Transparent handling (alpha 0)
    if (base.a > 0) {
        draw_rounded_rect(x, y, w, h, 6.0f, base);
    }
    
    // Text Color
    Color txtColor = {0.2f, 0.2f, 0.2f, 1};
    if (base.r < 0.5f && base.a > 0.5f) txtColor = {1,1,1,1}; // Dark bg -> White text
    if (bgOverride.r != -1 && base.a == 0) txtColor = {1,1,1,1}; // Transparent bg -> likely white text (for sidebar)
    
    float scale = fontSize / 16.0f;
    float textWidth = measure_text_width(label, scale);
    float tx = x + (w - textWidth) / 2;
    float ty = y + (h - (16 * scale)) / 2;
    draw_text(tx, ty, label, scale, txtColor);

    return hover && click;
}

void draw_textbox(TextBox& tb, float x, float y, float w, float h,
                  double mx, double my, bool click, float fontSize) {

    if (click) {
        bool hover = mx > x && mx < x+w && my > y && my < y+h;
        if (hover) {
            tb.focus = true;
            appState.activeTextbox = &tb;
        } else if (appState.activeTextbox == &tb) {
           // check outside
        }
    }
    
    bool isActive = (&tb == appState.activeTextbox);

    // Border (Rounded-ish feel via thinner strokes?)
    Color border = isActive ? Color{0.2f, 0.6f, 1.0f, 1} : Color{0.8f, 0.8f, 0.8f, 1}; 
    rect(x - 1, y - 1, w + 2, h + 2, border);

    // Background - White
    rect(x, y, w, h, {1.0f, 1.0f, 1.0f, 1});

    // Text - Dark Gray with fontSize
    // Placeholder logic
    if (tb.value.empty() && !isActive) {
        draw_text(x + 10, y + (h-(16*fontSize))/2, "Search...", fontSize * 0.9f, {0.7f, 0.7f, 0.7f, 1});
    } else {
        draw_text(x + 10, y + (h-(16*fontSize))/2, tb.value, fontSize, {0.2f, 0.2f, 0.2f, 1});
    }
    
    // Cursor - Blinking? (Simplified: Just draw if active)
    if (isActive) {
        float cursorX = x + 10 + measure_text_width(tb.value, fontSize);
        rect(cursorX, y + 8, 2, h - 16, {0.2f, 0.2f, 0.2f, 1});
    }
}

void draw_card(float x, float y, float w, float h, const std::string& title) {
    // Card Shadow/Border
    rect(x, y+2, w, h, {0.9f, 0.9f, 0.9f, 1}); // Subtle shadow offset
    rect(x, y, w, h, {1.0f, 1.0f, 1.0f, 1}); // Main White BG
    
    // Border outline
    rect(x, y, w, 1, {0.9f, 0.9f, 0.9f, 1}); // Top
    rect(x, y+h, w, 1, {0.9f, 0.9f, 0.9f, 1}); // Bottom
    rect(x, y, 1, h, {0.9f, 0.9f, 0.9f, 1}); // Left
    rect(x+w, y, 1, h, {0.9f, 0.9f, 0.9f, 1}); // Right
}


void icon_menu(float x, float y) {
    for (int i = 0; i < 3; i++)
        rect(x, y + i*7, 24, 3, {0.3f, 0.3f, 0.3f, 1.0f}); // Dark icons
}

void icon_home(float x, float y) {
    Color c = {0.3f, 0.3f, 0.3f, 1.0f}; // Dark icons
    rect(x + 4, y + 10, 16, 14, c);
    rect(x, y + 6, 24, 4, c);
}
