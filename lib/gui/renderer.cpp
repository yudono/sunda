#include "renderer.h"
#include <iostream>
#include <map>
#include <set>

#include <ft2build.h>
#include FT_FREETYPE_H

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <cmath>
#ifdef _WIN32
#include <GL/gl.h>
// Define GL_CLAMP_TO_EDGE if not available in MinGW's gl.h
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#endif

#ifndef PI
#define PI 3.14159265359f
#endif


// --- FreeType State ---
struct Character {
    GLuint     TextureID;
    int        Size[2];
    int        Bearing[2];
    unsigned int Advance;
};

std::map<char, Character> Characters;

void init_freetype() {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }

    FT_Face face;
    // Try multiple common macOS fonts
    const char* fontPaths[] = {
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Menlo.ttc",
        "Roboto-Regular.ttf" // Local fallback
    };

    bool loaded = false;
    for (const char* path : fontPaths) {
        if (!FT_New_Face(ft, path, 0, &face)) {
            // std::cout << "[FREETYPE] Loaded font: " << path << std::endl;
            loaded = true;
            break;
        }
    }

    if (!loaded) {
        std::cerr << "ERROR::FREETYPE: Failed to load any system font or fallback!" << std::endl;
        return;
    }

    FT_Set_Pixel_Sizes(face, 0, 16); 

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_ALPHA,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_ALPHA,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        Character character = {
            texture, 
            {static_cast<int>(face->glyph->bitmap.width), static_cast<int>(face->glyph->bitmap.rows)},
            {face->glyph->bitmap_left, face->glyph->bitmap_top},
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        Characters.insert(std::pair<char, Character>(c, character));
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

void draw_text(float x, float y, const std::string& text, float scale, Color color) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glColor4f(color.r, color.g, color.b, color.a);
    glEnable(GL_TEXTURE_2D);

    for (std::string::const_iterator c = text.begin(); c != text.end(); c++) {
        if (Characters.find(*c) == Characters.end()) continue;
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing[0] * scale;
        float ypos = y + (16 * scale - ch.Bearing[1] * scale);

        float w = ch.Size[0] * scale;
        float h = ch.Size[1] * scale;

        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(xpos,     ypos);
        glTexCoord2f(0.0f, 1.0f); glVertex2f(xpos,     ypos + h);
        glTexCoord2f(1.0f, 1.0f); glVertex2f(xpos + w, ypos + h);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(xpos + w, ypos);
        glEnd();
        
        x += (ch.Advance >> 6) * scale; 
    }
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

float measure_text_width(const std::string& text, float scale) {
    float w = 0;
    for (char c : text) {
        if (Characters.count(c)) {
            w += (Characters[c].Advance >> 6) * scale;
        }
    }
    return w;
}

unsigned int load_image(const char* path) {
    int w, h, ch;
    static std::set<std::string> failures;
    unsigned char* data = stbi_load(path, &w, &h, &ch, 4);
    if (!data) {
        if (failures.find(path) == failures.end()) {
            std::cerr << "Failed to load image: " << path << std::endl;
            failures.insert(path);
        }
        return 0;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return tex;
}

void draw_image(unsigned int tex, float x, float y, float w, float h) {
    if (tex == 0) return;
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glColor3f(1, 1, 1);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(x, y);
    glTexCoord2f(1, 0); glVertex2f(x + w, y);
    glTexCoord2f(1, 1); glVertex2f(x + w, y + h);
    glTexCoord2f(0, 1); glVertex2f(x, y + h);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void rect(float x, float y, float w, float h, Color c) {
    glDisable(GL_TEXTURE_2D); // Safety: ensure texture is off for solid colors
    glColor4f(c.r, c.g, c.b, c.a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void draw_rounded_rect(float x, float y, float w, float h, float r, Color c) {
    glDisable(GL_TEXTURE_2D);
    glColor4f(c.r, c.g, c.b, c.a);

    // Clamp radius
    if (r > w/2) r = w/2;
    if (r > h/2) r = h/2;

    int num_segments = 12;

    // Center part
    glBegin(GL_QUADS);
    glVertex2f(x+r, y);     glVertex2f(x+w-r, y);
    glVertex2f(x+w-r, y+h); glVertex2f(x+r, y+h);
    
    glVertex2f(x, y+r);     glVertex2f(x+r, y+r);
    glVertex2f(x+r, y+h-r); glVertex2f(x, y+h-r);
    
    glVertex2f(x+w-r, y+r);     glVertex2f(x+w, y+r);
    glVertex2f(x+w, y+h-r);     glVertex2f(x+w-r, y+h-r);
    glEnd();

    // Corners
    auto draw_arc = [&](float cx, float cy, float start_angle) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for(int i = 0; i <= num_segments; i++) {
            float theta = start_angle + (PI * i / (2.0f * num_segments));
            float dx = r * cosf(theta);
            float dy = r * sinf(theta);
            glVertex2f(cx + dx, cy + dy);
        }
        glEnd();
    };

    const float PI_VAL = 3.14159265f;
    draw_arc(x+w-r, y+h-r, 0.0f);          // Bottom Right
    draw_arc(x+r,   y+h-r, 0.5f * PI_VAL); // Bottom Left
    draw_arc(x+r,   y+r,   1.0f * PI_VAL); // Top Left
    draw_arc(x+w-r, y+r,   1.5f * PI_VAL); // Top Right
}

Color parse_color(const std::string& hex) {
    if (hex.size() < 7 || hex[0] != '#') return {1,1,1,1};
    int r, g, b;
    sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b);
    return { r/255.0f, g/255.0f, b/255.0f, 1.0f };
}
