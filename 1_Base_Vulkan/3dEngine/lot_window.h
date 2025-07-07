#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace lot {
    class LotWindow {
        public:
            LotWindow(int w, int h, std::string name);
            ~LotWindow();

            LotWindow(const LotWindow &) = delete;
            LotWindow &operator=(const LotWindow &) = delete;

            bool shouldClose() { return glfwWindowShouldClose(window); }

        private:
            void initWindow();

            const int width;
            const int height;
            std::string windowName;
            GLFWwindow *window;
    };
}