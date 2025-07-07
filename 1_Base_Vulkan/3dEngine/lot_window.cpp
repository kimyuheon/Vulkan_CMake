#include "lot_window.h"

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
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
    }
}