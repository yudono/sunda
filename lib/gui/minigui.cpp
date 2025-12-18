#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <OpenGL/gl.h>
#include "minigui.h"
#include "types.h"
#include "parser.h"
#include "layout.h"
#include "renderer.h"
#include <iostream>
#include <vector>

// Define global registries
std::map<std::string, std::function<void()>> clickRegistry;
std::map<std::string, std::function<void(std::string)>> changeRegistry;
std::map<std::string, std::function<std::string()>> dataRegistry;

void bind_click(const std::string& id, std::function<void()> callback) {
    clickRegistry[id] = callback;
}
void bind_change(const std::string& id, std::function<void(std::string)> callback) {
    changeRegistry[id] = callback;
}
void bind_str(const std::string& id, std::function<std::string()> callback) {
    dataRegistry[id] = callback;
}

// Helpers for internal use
void trigger_click(const std::string& id) {
    if (clickRegistry.count(id)) clickRegistry[id]();
    else std::cout << "Warning: No click handler for " << id << std::endl;
}

void trigger_change(const std::string& id, const std::string& newValue) {
    if (changeRegistry.count(id)) changeRegistry[id](newValue);
}

std::string resolve_binding(const std::string& text) {
    // Check if text is "{something}"
    if (text.size() > 2 && text.front() == '{' && text.back() == '}') {
        std::string key = text.substr(1, text.size() - 2);
        if (dataRegistry.count(key)) {
            return dataRegistry[key]();
        }
    }
    return text;
}

// Global App State wrapper
extern AppState appState;

void char_callback(GLFWwindow* window, unsigned int c) {
    if (appState.activeTextbox && c >= 32 && c <= 126) {
        appState.activeTextbox->value.push_back((char)c);
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_BACKSPACE && appState.activeTextbox) {
            if (!appState.activeTextbox->value.empty()) {
                appState.activeTextbox->value.pop_back();
            }
        }
    }
}

static bool isDirty = false;
void request_rerender() {
    isDirty = true;
}

void render_gui(std::function<std::string()> layoutGenerator) {
    if (!glfwInit()) return;

    GLFWwindow* window = glfwCreateWindow(1024, 768, "Mini-GUI App", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);
    glfwSetCharCallback(window, char_callback);
    glfwSetKeyCallback(window, key_callback);

    init_freetype();
    
    // Initial content
    Node root;
    auto updateLayout = [&]() {
        try {
            std::string xml = layoutGenerator();
            XmlParser parser(xml);
            root = parser.parse();
        } catch (const std::exception& e) {
            std::cerr << "XML Error: " << e.what() << std::endl;
        }
        isDirty = false;
    };
    
    updateLayout();

    glfwSetScrollCallback(window, [](GLFWwindow* w, double xoffset, double yoffset) {
        appState.scrollOffset -= yoffset * 20;
        if (appState.scrollOffset < 0) appState.scrollOffset = 0;
        if (appState.scrollOffset > appState.maxScroll) appState.scrollOffset = appState.maxScroll;
    });

    bool mouseClicked = false;

    while (!glfwWindowShouldClose(window)) {
        if (isDirty) updateLayout();

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        int winW, winH;
        glfwGetWindowSize(window, &winW, &winH);
        
        glViewport(0, 0, width, height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width, height, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        float scaleX = (float)width / (float)winW;
        float scaleY = (float)height / (float)winH;
        mx *= scaleX;
        my *= scaleY;

        int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        bool click = (state == GLFW_PRESS && !mouseClicked);
        mouseClicked = (state == GLFW_PRESS);

        glClearColor(1,1,1,1);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render
        float startY = 0.0f;
        render_node(root, 0, startY, mx, my, click, (float)width, (float)height);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}

void render_gui(const std::string& xmlLayout) {
    // Adapter for static string
    render_gui([=](){ return xmlLayout; });
}
