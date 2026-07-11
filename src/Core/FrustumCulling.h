#pragma once

#include <glm/glm.hpp>
#include "../Scene/Model.h"

namespace Utils {
    
    bool isAABBInFrustum(const Model& model, const glm::mat4& viewProj);
}