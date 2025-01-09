//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                   LABORATÓRIO 5
//

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include <list> // Necessário para std::list
// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

//Bouding box para teste de colisão
struct BoundingBox {
    glm::vec3 min;
    glm::vec3 max;
};

// Declaração das caixas delimitadoras
extern BoundingBox playerBox;
extern BoundingBox enemyBox;
extern std::vector<BoundingBox> treeBoxes;

// Função que calcula o tempo que irá levar para um plano e um AABB colidirem.
float CalculateAABBToPlaneCollisionTime(float plane_height, float bbox_min_height, float vel);

// Função que calcula se um raio intersecta um AABB
bool RayIntersectsAABB(glm::vec4 &ray_origin, glm::vec4 &ray_direction, glm::vec4 &aabb_min, glm::vec4 &aabb_max);

// Verifica uma colisão entre 2 bounding boxes
bool CheckCollision(const BoundingBox &box1, const BoundingBox &box2);

// Verifica se o jogador está entre os limites do mapa
void CheckPlayerBounds(glm::vec4& playerPos, float mapScale);

// Faz update do bounding box do inimigo pra teste de colisão
void UpdateBoundingBox(BoundingBox &box, glm::vec3 position, glm::vec3 scale);

// Função para verificar colisão com inimigo
bool CheckPlayerEnemyCollision();

void CreateTreeBoundingBoxes(const std::list<std::pair<float, float>>& treePositions, const glm::vec3& treeDimensions);

bool CheckPlayerTreeCollision() ;

