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

    Application(const Application&) = delete;
    Application(Application&&) = delete;
    auto operator=(const Application&) -> Application& = delete;
    auto operator=(Application&&) -> Application& = delete;
    virtual ~Application() = default;

    void run();

protected:
    ModelStorage m_model_storage;

private:
    std::string m_name{ "Thyme" };
    Logger& m_logger;
};

}// namespace th
