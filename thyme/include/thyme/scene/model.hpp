#pragma once

#include <thyme/core/logger.hpp>
#include <thyme/core/texture.hpp>
#include <thyme/export_macros.hpp>
#include <thyme/renderer/structs.hpp>
#include <thyme/scene/transformation.hpp>

#include <fmt/format.h>

#include <ranges>
#include <vector>

namespace th::scene {

class THYME_API Mesh {
public:
    std::vector<renderer::Vertex> vertices;
    std::vector<uint32_t> indices;
};
;

class THYME_API Model {
public:
    glm::vec4 solidColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    Transformation transformation;
    std::string name;
    Mesh mesh;
    Texture texture;// TODO! make array of textures
};

class THYME_API ModelStorage {
public:
    inline void addModel(const Model& model) {
        TH_API_LOG_INFO("Adding model (name: {}, vertices: {}, indices: {})",
                        model.name,
                        model.mesh.vertices.size(),
                        model.mesh.indices.size());
        m_models.emplace_back(model);
    }
    // inline inline void deleteModel(const Model& model) {
    //     m_models.erase(std::remove(m_models.begin(), m_models.end(), model), m_models.end());
    // }
    // inline void deleteModel(const std::string_view name) {
    //     const auto it = std::ranges::find_if(m_models, [name](const Model& model) { return model.name == name; });
    //     if (it != m_models.end()) {
    //         m_models.erase(it);
    //     }
    // }

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

    [[nodiscard]] inline auto& operator[](size_t index) noexcept {
        return m_models[index];
    }

    [[nodiscard]] inline auto& operator[](size_t index) const noexcept {
        return m_models[index];
    }

    [[nodiscard]] inline auto& operator[](const std::string_view name) {
        for (auto& model : m_models) {
            if (model.name == name) {
                return model;
            }
        }
        throw std::runtime_error(fmt::format("Model with name {} not found", name));
    }

    [[nodiscard]] inline auto& operator[](const std::string_view name) const {
        for (const auto& model : m_models) {
            if (model.name == name) {
                return model;
            }
        }
        throw std::runtime_error(fmt::format("Model with name {} not found", name));
    }

private:
    std::vector<Model> m_models;
};

}// namespace th::scene
