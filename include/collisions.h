// collisions.hpp

#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <glm/glm.hpp>

// Estrutura para representar uma caixa delimitadora AABB
struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;
};

// Calcula a bounding box do carro com base na posição atual e no bbox local do modelo
BoundingBox ComputeCarAABB(const glm::vec4& carPosition, const glm::vec3& bbox_min, const glm::vec3& bbox_max);

// Verifica colisão entre a AABB do carro e a AABB do plano
bool CheckAABBCollisionWithPlane(const BoundingBox& carBox, const BoundingBox& planeBox);

// Calcula o tempo para colisão vertical com um plano dado
float CalculateAABBToPlaneCollisionTime(float plane_height, float bbox_min_height, float vel);

#endif // COLLISIONS_H
