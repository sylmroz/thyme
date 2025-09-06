module;

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

export module th.scene.model;

import std;

import th.core.logger;
import th.scene.texture_data;
import th.scene.transformation;

export namespace th {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

class Model {
public:
    glm::vec4 solidColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    Transformation transformation;
    std::string name;
    Mesh mesh;
    TextureData texture;// TODO! make array of textures
    std::function<void(Model&)> onAnimate;
    void animate() {
        onAnimate(*this);
    }
};

class ModelStorage {
public:
    inline auto addModel(const Model& model) -> Model& {
         ThymeLogger::getLogger()->info("Adding model (name: {}, vertices: {}, indices: {})",
                        model.name,
                        model.mesh.vertices.size(),
                        model.mesh.indices.size());
        if (std::ranges::find_if(m_models, [&model](const Model& m) { return m.name == model.name; })
            != m_models.end()) {
            const auto msg = std::format("Cannot add model {}. Already exists.", model.name);
            ThymeLogger::getLogger()->error("{}", msg);
            throw std::runtime_error(msg);
        }
        return m_models.emplace_back(model);
    }

    inline void deleteModel(const std::string_view name) noexcept {
        if (const auto it = std::ranges::find_if(m_models, [name](const Model& model) { return model.name == name; });
            it != m_models.end()) {
            m_models.erase(it);
        }
    }

    inline void deleteModel(const Model& model) noexcept {
        deleteModel(model.name);
    }

    [[nodiscard]] inline auto begin() noexcept {
        return m_models.begin();
    }

    [[nodiscard]] inline auto begin() const noexcept {
        return m_models.begin();
    }

    [[nodiscard]] inline auto end() noexcept {
        return m_models.end();
    }

    [[nodiscard]] inline auto end() const noexcept {
        return m_models.end();
    }

    [[nodiscard]] inline auto size() const noexcept {
        return m_models.size();
    }

    [[nodiscard]] inline auto empty() const noexcept {
        return m_models.empty();
    }

    [[nodiscard]] inline auto operator[](const size_t index) noexcept -> Model& {
        return m_models[index];
    }

    [[nodiscard]] inline auto operator[](const size_t index) const noexcept -> const Model& {
        return m_models[index];
    }

private:
    std::vector<Model> m_models;
};

}// namespace th