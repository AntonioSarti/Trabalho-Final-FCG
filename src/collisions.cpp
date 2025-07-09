#include "collisions.h"
#include <glm/glm.hpp>
#include <iostream>
#include <algorithm>

bool CheckAABBCollision(const BoundingBox& a, const BoundingBox& b) {
    if (a.max.x < b.min.x || a.min.x > b.max.x) return false;
    if (a.max.y < b.min.y || a.min.y > b.max.y) return false;
    if (a.max.z < b.min.z || a.min.z > b.max.z) return false;
    return true;
}

BoundingBox ComputeCarAABB(const glm::vec4& carPosition, const glm::vec3& bbox_min, const glm::vec3& bbox_max) {
    glm::vec3 pos = glm::vec3(carPosition);

    BoundingBox box;
    box.min = bbox_min + pos;
    box.max = bbox_max + pos;
    return box;
}

bool CheckAABBCollisionWithPlane(const BoundingBox& box, float plane_y) {
    return box.min.y < plane_y;
}

bool ResolveCarWallCollision(
    glm::vec4& carPos,
    const glm::vec4& previousCarPos,
    const glm::vec3& car_bbox_min,
    const glm::vec3& car_bbox_max,
    float& carSpeed,
    const std::vector<glm::vec3>& wall_positions,
    const glm::vec3& wall_bbox_min,
    const glm::vec3& wall_bbox_max,
    const glm::vec3& wall_scale
) {
    BoundingBox currentBox = ComputeCarAABB(carPos, car_bbox_min, car_bbox_max);

    for (const auto& pos : wall_positions) {
        glm::vec3 wall_min = wall_bbox_min * wall_scale + pos;
        glm::vec3 wall_max = wall_bbox_max * wall_scale + pos;
        BoundingBox wallBox = { wall_min, wall_max };

        if (CheckAABBCollision(currentBox, wallBox)) {
           
            glm::vec3 overlap(0.0f);

            if (currentBox.max.x > wallBox.min.x && currentBox.min.x < wallBox.max.x) {
                float overlapX1 = wallBox.max.x - currentBox.min.x;
                float overlapX2 = currentBox.max.x - wallBox.min.x;
                overlap.x = (overlapX1 < overlapX2 ? -overlapX1 : overlapX2);
            }

            if (currentBox.max.z > wallBox.min.z && currentBox.min.z < wallBox.max.z) {
                float overlapZ1 = wallBox.max.z - currentBox.min.z;
                float overlapZ2 = currentBox.max.z - wallBox.min.z;
                overlap.z = (overlapZ1 < overlapZ2 ? -overlapZ1 : overlapZ2);
            }

            
            if (std::abs(overlap.x) < std::abs(overlap.z)) {
                overlap.z = 0;
            } else {
                overlap.x = 0;
            }

            // Aplica correção de colisão
            carPos += glm::vec4(overlap, 0.0f);

            // Aplica empurrão adicional na direção oposta ao movimento
            glm::vec3 pushDir = glm::normalize(glm::vec3(previousCarPos - carPos));
            float pushStrength = 0.3f; 
            carPos += glm::vec4(pushDir * pushStrength, 0.0f);
            carSpeed = 0.0f;
            return true;
        }
    }

    return false;
}

void ResolveCarGroundCollision(
    glm::vec4& carPos,
    const glm::vec3& car_bbox_min,
    float plane_y_position
) {
    float car_base_y = carPos.y + car_bbox_min.y;
    if (car_base_y < plane_y_position) {
        float offset = car_bbox_min.y;
        carPos.y = plane_y_position - offset;
        //carPos.y += plane_y_position - car_base_y;

      //  std::cout << "Colisão com o plano detectada. Corrigindo posição." << std::endl;
    }
}

bool CheckSphereCollision(
    const glm::vec3& centerA, float radiusA,
    const glm::vec3& centerB, float radiusB
) {
    float distanceSquared = glm::dot(centerA - centerB, centerA - centerB);
    float radiusSum = radiusA + radiusB;
    return distanceSquared <= radiusSum * radiusSum;
}

bool ResolveSphereCollision(
    glm::vec4& posA, float radiusA, float& speedA,
    glm::vec4& posB, float radiusB, float& speedB
) {
    glm::vec3 centerA = glm::vec3(posA);
    glm::vec3 centerB = glm::vec3(posB);

    float distance = glm::distance(centerA, centerB);
    float radiusSum = radiusA + radiusB;

    if (distance <= radiusSum) {
        speedA = 0.0f;
        speedB = 0.0f;

        if (distance == 0.0f)
            return true;

        glm::vec3 direction = glm::normalize(centerA - centerB);
        float overlap = radiusSum - distance;

        posA += glm::vec4(direction * (overlap * 0.5f), 0.0f);
        posB -= glm::vec4(direction * (overlap * 0.5f), 0.0f);

        return true;
    }

    return false;
}

