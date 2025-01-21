module;

#include <glm/glm.hpp>

export module thyme.renderer.models;

export namespace Thyme::Renderer {

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

}// namespace thyme::renderer