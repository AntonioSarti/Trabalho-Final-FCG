#include "collisions.h"

bool CheckAABBCollision(const BoundingBox& a, const BoundingBox& b) {
    if (a.max.x < b.min.x || a.min.x > b.max.x) return false;
    if (a.max.y < b.min.y || a.min.y > b.max.y) return false;
    if (a.max.z < b.min.z || a.min.z > b.max.z) return false;
    return true;
}

BoundingBox ComputeCarAABB(const glm::vec4& carPosition, const glm::vec3& bbox_min, const glm::vec3& bbox_max) {
    // Converte a posição para vec3
    glm::vec3 pos = glm::vec3(carPosition);

    BoundingBox box;
    box.min = bbox_min + pos;
    box.max = bbox_max + pos;
    return box;
}
