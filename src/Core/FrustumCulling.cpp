#include "FrustumCulling.h"

namespace Utils {
    bool isAABBInFrustum(const Model& model, const glm::mat4& viewProj) {
        glm::vec3 min = model.localMinBounds;
        glm::vec3 max = model.localMaxBounds;
        
        glm::mat4 MVP = viewProj * model.transformMatrix;

        glm::vec4 corners[8] = {
            MVP * glm::vec4(min.x, min.y, min.z, 1.0f),
            MVP * glm::vec4(max.x, min.y, min.z, 1.0f),
            MVP * glm::vec4(min.x, max.y, min.z, 1.0f),
            MVP * glm::vec4(max.x, max.y, min.z, 1.0f),
            MVP * glm::vec4(min.x, min.y, max.z, 1.0f),
            MVP * glm::vec4(max.x, min.y, max.z, 1.0f),
            MVP * glm::vec4(min.x, max.y, max.z, 1.0f),
            MVP * glm::vec4(max.x, max.y, max.z, 1.0f)
        };

        int outLeft = 0, outRight = 0, outBottom = 0, outTop = 0, outNear = 0, outFar = 0;
        
        for (int i = 0; i < 8; i++) {
            if (corners[i].x < -corners[i].w) outLeft++;
            if (corners[i].x >  corners[i].w) outRight++;
            if (corners[i].y < -corners[i].w) outBottom++;
            if (corners[i].y >  corners[i].w) outTop++;
            if (corners[i].z < -corners[i].w) outNear++;
            if (corners[i].z >  corners[i].w) outFar++;
        }

        return !(outLeft == 8 || outRight == 8 || outBottom == 8 || outTop == 8 || outNear == 8 || outFar == 8);
    }
}