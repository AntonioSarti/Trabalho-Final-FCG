#include "collisions.hpp"

#include <cmath>
#include <limits>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Definição das caixas delimitadoras
BoundingBox playerBox;
BoundingBox enemyBox;
std::vector<BoundingBox> treeBoxes;

// AABB para plano
float CalculateAABBToPlaneCollisionTime(float plane_height, float bbox_min_height, float vel) {
    return (plane_height - bbox_min_height) / vel;
}

// Raio para AABB
bool RayIntersectsAABB(glm::vec4 &ray_origin, glm::vec4 &ray_direction, glm::vec4 &aabb_min, glm::vec4 &aabb_max) {
    float t_min = 0.0f;
    float t_max = std::numeric_limits<float>::max();

    // Iterate over the three axes (x, y, z)
    for (int i = 0; i < 3; i++) {
        float invD = 1.0f / (i == 0 ? ray_direction.x : (i == 1 ? ray_direction.y : ray_direction.z));
        float t0 = ((i == 0 ? aabb_min.x : (i == 1 ? aabb_min.y : aabb_min.z)) - (i == 0 ? ray_origin.x : (i == 1 ? ray_origin.y : ray_origin.z))) * invD;
        float t1 = ((i == 0 ? aabb_max.x : (i == 1 ? aabb_max.y : aabb_max.z)) - (i == 0 ? ray_origin.x : (i == 1 ? ray_origin.y : ray_origin.z))) * invD;

        if (invD < 0.0f) std::swap(t0, t1);

        t_min = std::max(t_min, t0);
        t_max = std::min(t_max, t1);

        if (t_max <= t_min) {
            return false; // No intersection
        }
    }

    return true; // Intersection exists
}

// AABB para AABB
bool CheckCollision(const BoundingBox &box1, const BoundingBox &box2) {
    return (box1.min.x <= box2.max.x && box1.max.x >= box2.min.x) && 
    (box1.min.y <= box2.max.y && box1.max.y >= box2.min.y) &&
    (box1.min.z <= box2.max.z && box1.max.z >= box2.min.z);
}

// Ponto para AABB
void CheckPlayerBounds(glm::vec4& playerPos, float mapScale)
{
    float mapMin = -mapScale/2;
    float mapMax = mapScale/2;

    if (playerPos.x < mapMin) playerPos.x = mapMin;
    if (playerPos.x > mapMax) playerPos.x = mapMax;
    if (playerPos.z < mapMin) playerPos.z = mapMin;
    if (playerPos.z > mapMax) playerPos.z = mapMax;
}

bool CheckPlayerEnemyCollision() {
    return CheckCollision(playerBox, enemyBox);
}

bool CheckPlayerTreeCollision() {
    for (const auto& treeBox : treeBoxes) {
        if (CheckCollision(playerBox, treeBox)) {
            return true; // Colidiu com pelo menos uma árvore
        }
    }
    return false;
}

void CreateTreeBoundingBoxes(const std::list<std::pair<float, float>>& treePositions, const glm::vec3& treeDimensions) {
    for (const auto& pos : treePositions) {
        float x = pos.first;  // x coordenada
        float z = pos.second; // z coordenada
        
        // Cria uma box delimitadora para o objeto árvore
        BoundingBox bbox;
        bbox.min = glm::vec3(x, 0.0f, z); // A partir da altura zero da árvore
        bbox.max = bbox.min + treeDimensions; // Coloca as dimensões do objeto árvore
        
        // Adiciona a caixa delimitadora à lista de caixas (supondo que você tenha uma estrutura para armazenar)
        treeBoxes.push_back(bbox);
    }
}

void UpdateBoundingBox(BoundingBox &box, glm::vec3 position, glm::vec3 scale) {
            box.min = position - scale * 0.5f;
            box.max = position + scale * 0.5f;
        }
        