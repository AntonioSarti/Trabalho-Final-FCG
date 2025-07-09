// collisions.hpp

#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <glm/glm.hpp>
#include <vector> 



// Estrutura para representar uma caixa delimitadora AABB
struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;
};

// Calcula a bounding box do carro com base na posição atual e no bbox local do modelo
BoundingBox ComputeCarAABB(const glm::vec4& carPosition, const glm::vec3& bbox_min, const glm::vec3& bbox_max);

// Verifica colisão entre a AABB do carro e a AABB do plano
bool CheckAABBCollisionWithPlane(const BoundingBox& box, float plane_y);

// Calcula o tempo para colisão vertical com um plano dado
float CalculateAABBToPlaneCollisionTime(float plane_height, float bbox_min_height, float vel);

// Verifica colisão entre cubos
bool CheckAABBCollision(const BoundingBox& a, const BoundingBox& b);

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
);

void ResolveCarGroundCollision(
    glm::vec4& carPos,
    const glm::vec3& car_bbox_min,
    float plane_y_position
);

// Verifica colisão entre duas esferas (carros)
bool CheckSphereCollision(
    const glm::vec3& centerA, float radiusA,
    const glm::vec3& centerB, float radiusB
);

bool ResolveSphereCollision(
    glm::vec4& posA, float radiusA, float& speedA,
    glm::vec4& posB, float radiusB, float& speedB
);

// Verifica se o carro está no limite da área do chão
void CheckCarbyBounds(glm::vec4& CarPos, float mapScale);

#endif // COLLISIONS_H
