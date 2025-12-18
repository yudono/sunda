#ifndef MINIGUI_H
#define MINIGUI_H

#include <string>
#include <functional>
#include <map>

// Global Event Registry
void bind_click(const std::string& id, std::function<void()> callback);
void bind_change(const std::string& id, std::function<void(std::string)> callback);
void bind_str(const std::string& id, std::function<std::string()> callback);

void trigger_click(const std::string& id);
void trigger_change(const std::string& id, const std::string& newValue);
std::string resolve_binding(const std::string& text);

// Main Entry Point
void render_gui(const std::string& xmlLayout);
void render_gui(std::function<std::string()> layoutGenerator);

void request_rerender();

#endif
