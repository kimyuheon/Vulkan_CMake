#include "lot_app_base.h"

namespace lot {

    void LotAppBase::run() {
        while (!lotWindow.shouldClose()) {
            /* code */
            glfwPollEvents();
        }
    }
}