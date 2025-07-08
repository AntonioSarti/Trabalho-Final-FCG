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
    // Converte a posição para vec3
    glm::vec3 pos = glm::vec3(carPosition);

    BoundingBox box;
    box.min = bbox_min + pos;
    box.max = bbox_max + pos;
    return box;
}

bool CheckAABBCollisionWithPlane(const BoundingBox& box, float plane_y) {
    // A colisão acontece se a parte inferior da AABB estiver abaixo do plano
    return box.min.y < plane_y;
}

// verifica e corrige a colisão entre o carro e as paredes
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
)
{
    BoundingBox previousCarBox = ComputeCarAABB(previousCarPos, car_bbox_min, car_bbox_max);
    BoundingBox currentCarBox  = ComputeCarAABB(carPos,        car_bbox_min, car_bbox_max);
    bool collided = false;

    for (const auto& pos : wall_positions) {
        glm::vec3 wall_min = wall_bbox_min * wall_scale + pos;
        glm::vec3 wall_max = wall_bbox_max * wall_scale + pos;
        BoundingBox wallBox = { wall_min, wall_max };

       // if (!CheckAABBCollision(previousCarBox, wallBox) &&
        //     CheckAABBCollision(currentCarBox,  wallBox)) {
       //     collided = true;
       //     break;
      //  }
      if (CheckAABBCollision(currentCarBox, wallBox)) {
    collided = true;
    break;
}
    }

    if (collided) {
        carPos = previousCarPos; // volta antes do impacto

        glm::vec3 movement = glm::vec3(currentCarBox.min - previousCarBox.min);
        glm::vec3 step = movement * 0.02f; // passos pequenos para precisão
        int maxSteps = 50;

        for (int i = 0; i < maxSteps; ++i) {
            carPos += glm::vec4(step, 0.0f); // tenta novamente se aproximar
            BoundingBox testBox = ComputeCarAABB(carPos, car_bbox_min, car_bbox_max);

            bool stillColliding = false;
            for (const auto& pos : wall_positions) {
                glm::vec3 wall_min = wall_bbox_min * wall_scale + pos;
                glm::vec3 wall_max = wall_bbox_max * wall_scale + pos;
                BoundingBox wallBox = { wall_min, wall_max };

                if (CheckAABBCollision(testBox, wallBox)) {
                    stillColliding = true;
                    break;
                }
            }

            if (stillColliding) {
                carPos -= glm::vec4(step, 0.0f); // recua o último passo
                break;
            }
        }

        carSpeed = 0.0f;
        //std::cout << "Colisão com parede detectada! Ajuste preciso aplicado." << std::endl;
        return true;
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
        // Zera velocidades
        speedA = 0.0f;
        speedB = 0.0f;

        // Evita divisão por zero
        if (distance == 0.0f)
            return true;

        // Calcula vetor de separação
        glm::vec3 direction = glm::normalize(centerA - centerB);
        float overlap = radiusSum - distance;

        // Aplica deslocamento proporcional para afastar os objetos
        posA += glm::vec4(direction * (overlap * 0.5f), 0.0f);
        posB -= glm::vec4(direction * (overlap * 0.5f), 0.0f);

        return true;
    }

    return false;
}
