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

            VkExtent2D getExtent() {
                return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
            }
            bool wasWindowReSized() { return framebufferResized; }
            void resetWindowResizedFlag() { framebufferResized = false; }

            void createWindowSurface(VkInstance instance, VkSurfaceKHR *surfacec_);

        private:
            static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
            void initWindow();

            int width;
            int height;
            bool framebufferResized = false;

            std::string windowName;
            GLFWwindow *window;
    };
}