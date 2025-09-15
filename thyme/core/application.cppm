module;

export module th.core.application;

import std;

import th.core.engine;
import th.core.logger;
import th.scene.model;
import th.platform.imgui_context;

namespace th {

export class Application {
public:
    explicit Application(Logger& logger);
    std::string name{ "Thyme" };
    void run();

    ModelStorage modelStorage;

private:
    Logger& m_logger;
};

}// namespace th
