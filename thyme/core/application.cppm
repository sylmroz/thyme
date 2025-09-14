module;

export module th.core.application;

import std;

import th.core.engine;
import th.core.logger;
import th.platform.glfw.glfw_context;
import th.scene.model;
import th.platform.imgui_context;

namespace th {

export class Application {
public:
    Application(Logger& logger);
    std::string name{ "Thyme" };
    void run();

    ModelStorage modelStorage;

private:
    Logger& m_logger;
};

}// namespace th
