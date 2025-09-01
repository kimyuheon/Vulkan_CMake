#include "lot_window.h"
#include <stdexcept>

namespace lot {
    LotWindow::LotWindow(int w, int h, std::string name) 
    : width{w}, height{h}, windowName{name} {
        initWindow();
    }

    LotWindow::~LotWindow() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void LotWindow::initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    void LotWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surfacec_) {
        if (glfwCreateWindowSurface(instance, window, nullptr, surfacec_) != VK_SUCCESS)
            throw std::runtime_error("failed to ctreate window surface");
    }

    void LotWindow::framebufferResizeCallback(GLFWwindow *window, int width, int height) {
        auto lotWindow = reinterpret_cast<LotWindow *>(glfwGetWindowUserPointer(window));
        lotWindow->framebufferResized = true;
        lotWindow->width = width;
        lotWindow->height = height;
    }
} // namespace lot