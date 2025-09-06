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
    Application();
    std::string name{ "Thyme" };
    void run();

    ModelStorage modelStorage;
};

}// namespace th
