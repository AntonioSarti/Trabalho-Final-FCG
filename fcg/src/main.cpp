//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//          - TRABALHO FINAL DA DISCIPLINA -
//

//
// Código fonte baseado no source do laboratório 5 (obtido via moodle) 
// Todos os modelos utilizados foram obtidos, através do site (FONTE): https://rigmodels.com/
//      

#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>
#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "ambient.hpp"
#include "collisions.hpp"

// Constantes
#define WEAPON 0
#define BODY   1
#define PLANE  2
#define TREE   3
#define ENEMY  4
#define MOV_SPEED 6.0f
#define GRAVITY 80.0f
#define SENSITIVITY 0.0025f // Ajuste da sensibilidade do mouse
#define LOOKAT_CAMERA_DIST 10.0f
#define ANIMATION_DURATION 0.45f
#define SPAWN_MIN_RADIUS 25.0f // Raio mínimo do jogador ao centro do mapa, para o inimigo surgir
#define ATTACK_DISTANCE 15.0f
#define ATTACK_DAMAGE 25
#define FLASH_DURATION 0.2f
#define ENEMY_MOV_SPEED 5.0f
#define ENEMY_ATTACK_DAMAGE 20
#define ENEMY_ATTACK_RANGE 5.0f
#define ENEMY_ATTACK_COOLDOWN 1.0f

#define PI 3.14159265358979323846f

// Indica se o jogo está em estado de "Game Over"
bool g_GameOver = false; 

// Se o inimigo atacou recentemente
bool g_EnemyAttacked = false;

// Tempo da última vez que o inimigo atacou
float g_LastEnemyAttackTime = -std::numeric_limits<float>::infinity();

// Adicionando pontos de vida do jogador
int g_PlayerLifePoints = 100;

// Criando jogador
bool g_PlayerAlive = true;

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, então setamos basepath como o dirname do
        // filename, para que os arquivos MTL sejam corretamente carregados caso
        // estejam no mesmo diretório dos arquivos OBJ.
        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i+1);
                basepath = dirname.c_str();
            }
        }

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                    filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        printf("OK.\n");
    }
};

// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void ScalePlaneModelAndTexCoords(ObjModel* model); // Escala as coordenadas da malha e da textura do plano
void UpdatePlayerMovementSpeed(); // Atualiza a velocidade do jogador
void UpdatePlayerPosition(float delta_t); // Atualiza a posição do jogador, após computada a velocidade
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void ComputeGravity(glm::vec4& pos, glm::vec4& vel, float delta_t); // Computa os efeitos da gravidade
glm::vec4 CalculateWeaponAnimationPosition(float t); // Calcula a posição da lança normalizado da animação
glm::vec3 CalculateWeaponAnimationRotation(float t); // Calcula a rotação da lança normalizado da animação
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Escala da árvore
float g_TreeScaleX = 5.0f;
float g_TreeScaleY = 5.0f;
float g_TreeScaleZ = 5.0f;

// Posição do plano
float g_PlaneX = 0.0f;
float g_PlaneY = 0.0f;
float g_PlaneZ = 0.0f;

// Escala do plano
float g_PlaneScale = 5.0f;

// Posição do jogador
glm::vec4 g_PlayerPos;

// Velocidade do jogador
glm::vec4 g_PlayerSpeed = {0.0f, 0.0f, 0.0f, 0.0f};

// Se o jogador atacou
bool g_PlayerAttacked = false;

// Último timestamp que o jogador desferiu um golpe
float g_LastAttackTime = -std::numeric_limits<float>::infinity();

// Posição do adversário
glm::vec4 g_EnemyPos = {25.0f, 0.0f, 0.0f, 1.0f};

// Rotação do adversário
glm::vec3 g_EnemyRot = {0.0f, 0.0f, 0.0f};

// Rot. jogador
glm::vec3 g_PlayerRot = {0.0f, 0.0f, 0.0f};

// Velocidade do adversário
glm::vec4 g_EnemySpeed = {0.0f, 0.0f, 0.0f, 0.0f};

// Vida restante do adversário
int g_EnemyLifePoints = 100;

// Último timestamp que o adversário recebeu um dano
float g_LastHitTime = -std::numeric_limits<float>::infinity();

// Se a câmera atual é look-at
bool g_CameraLookAt = false;

// Parâmetros yaw e pitch da câmera 
float g_Yaw = 0.0f;
float g_Pitch = 0.0f;

// Se o usuário estiver com teclas do teclado pressionadas
bool g_WKeyPressed  = false;
bool g_AKeyPressed  = false;
bool g_SKeyPressed  = false;
bool g_DKeyPressed  = false;
bool g_ShiftPressed = false;

// Se o adversário já tiver surgido
bool g_EnemySpawned = false;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;
GLint g_flash_green_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "Trabalho Final da Disciplina de FCG", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Não exibe o cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Carrega texturas para o guerreiro (jogador)
    LoadTextureImage("data/W6ZJUN6TEFIF56TSWEZ1WE9J7_obj/f82f7c5f.jpg"); // lança (weapon)
    LoadTextureImage("data/W6ZJUN6TEFIF56TSWEZ1WE9J7_obj/915bd724.jpg"); // corpo

    // Carrega texturas para o plano do chão
    LoadTextureImage("data/3d0113f9558947579a5d7926f087cc6a_obj/RGB_3bc2fe2dd4be4268b9e34fed3c0d7db1_ground_grass.jpeg");

    // Carrega textura para as árvores
    LoadTextureImage("data/N6TIYZO5D41STEOW4SQBKWRRG_obj/farm_trees_rocks_flowers_D.jpg");

    // Carrega texturas para o enemy
    LoadTextureImage("data/VAS0DALXV8R4VFWDRPP05V2OH_obj/boss_zombie_01_d.jpg");

    // Rasterização em triang.
    ObjModel warriormodel("data/W6ZJUN6TEFIF56TSWEZ1WE9J7_obj/W6ZJUN6TEFIF56TSWEZ1WE9J7.obj");
    ComputeNormals(&warriormodel);
    BuildTrianglesAndAddToVirtualScene(&warriormodel);

    ObjModel planemodel("data/3d0113f9558947579a5d7926f087cc6a_obj/model.obj");
    ScalePlaneModelAndTexCoords(&planemodel);
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel treemodel("data/N6TIYZO5D41STEOW4SQBKWRRG_obj/N6TIYZO5D41STEOW4SQBKWRRG.obj");
    ComputeNormals(&treemodel);
    BuildTrianglesAndAddToVirtualScene(&treemodel);

    ObjModel enemymodel("data/VAS0DALXV8R4VFWDRPP05V2OH_obj/VAS0DALXV8R4VFWDRPP05V2OH.obj");
    ComputeNormals(&enemymodel);
    BuildTrianglesAndAddToVirtualScene(&enemymodel);


    // Contruímos a estrutura que armazena as posições das árvores no plano
    srand(30); // Seed para geração das árvores
    float plane_size = g_VirtualScene["object_0"].bbox_max.x - g_VirtualScene["object_0"].bbox_min.x;
    auto tree_pos = CalculatePoisson(plane_size, 5.5f, 50);
    RemoveFromCentralRadius(tree_pos, 5.0f);

    glm::vec3 treeDimensions(1.0f, g_TreeScaleY, 1.0f);
    CreateTreeBoundingBoxes(tree_pos, treeDimensions);

    // Configura posição inicial do jogador
    float playerSize = g_VirtualScene["Object_915bd724.jpg"].bbox_max.y - g_VirtualScene["Object_915bd724.jpg"].bbox_min.y;
    float enemySize = g_VirtualScene["Object_boss_zombie_01_d.jpg"].bbox_max.y - g_VirtualScene["Object_boss_zombie_01_d.jpg"].bbox_min.y;
    g_PlayerPos = { 0.0f, 0.0f + playerSize/2, 0.0f, 1.0f };

    // Calculamos a altura das árvores para que fiquem logo acima do plano do chão (grama)
    float tree_y = g_PlaneY - g_TreeScaleY * g_VirtualScene["Object_farm_trees_rocks_flowers_D.jpg"].bbox_min.y;

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Atualizamos o timestamp atual
    float current_time;
    float prev_time = (float) glfwGetTime();
    float delta_t;

    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //             R      G      B     A
        glClearColor(54.0f / 255.0f, 69.0f / 255.0f, 79.0f / 255.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);

        // Parâmetros que definem a câmera
        glm::vec4 camera_position_c;
        glm::vec4 camera_view_vector;

        // Calculamos o vetor "up", fixado para apontar para o "céu" (eixo Y global)
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f);

        // Testamos se a câmera é look-at ou livre
        if (g_CameraLookAt) {
            // Aplicamos as mesmas transformações do personagem para determinar o ponto "c"
            glm::vec4 reference_point = glm::vec4(7.5f, 3.0f, 0.0f, 1.0f);
            glm::mat4 transform_matrix = Matrix_Translate(g_PlayerPos.x, g_PlayerPos.y, g_PlayerPos.z)
                                         * Matrix_Rotate_X(g_Pitch)
                                         * Matrix_Rotate_Y(g_Yaw);

            glm::vec4 camera_lookat_l = g_PlayerPos; // Ponto "l", para onde a câmera (look-at) estará sempre olhando
            camera_position_c = transform_matrix * reference_point; // Ponto "c", centro da câmera
            camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
        } else {
            // Calculamos as rotações da câmera livre
            float x = cos(g_Pitch) * cos(g_Yaw);
            float y = sin(g_Pitch);
            float z = cos(g_Pitch) * sin(g_Yaw);

            camera_position_c = g_PlayerPos + glm::vec4(0.0f, playerSize*0.35, 0.0f, 0.0f);
            camera_view_vector = glm::vec4(-x, -y, z, 0.0f);
        }

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -1000.0f; // Posição do "far plane"

        // Projeção Perspectiva.
        // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
        float field_of_view = 3.141592 / 3.0f;
        projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));
        
        // Atualizamos o timestamp atual
        current_time = (float) glfwGetTime();
        delta_t = current_time - prev_time;
        prev_time = current_time;

        // Atualizamos a velocidade do jogador
        UpdatePlayerMovementSpeed();
        glm::vec4 previousPlayerPos = g_PlayerPos; // Salva a posição anterior do jogador

        if (g_PlayerAlive && g_EnemyLifePoints > 0 && g_EnemySpawned) {
            // Movimento do inimigo em direção ao jogador
            glm::vec4 direction_to_player = glm::normalize(g_PlayerPos - g_EnemyPos);
            g_EnemySpeed = direction_to_player * ENEMY_MOV_SPEED;
            g_EnemyPos += g_EnemySpeed * delta_t;

            if (CheckPlayerEnemyCollision()) {
                // Reverte a posição do jogador se houver colisão com o inimigo
                g_PlayerPos = previousPlayerPos;
            }

            // Verificação de Ataque
            float distance_to_player = glm::length(g_PlayerPos - g_EnemyPos);

            if (distance_to_player <= ENEMY_ATTACK_RANGE) {
                if (current_time - g_LastEnemyAttackTime >= ENEMY_ATTACK_COOLDOWN) {
                    g_EnemyAttacked = true;
                    g_LastEnemyAttackTime = current_time;
                    printf("Jogador Atingido! Dano Causado: %d\n", ENEMY_ATTACK_DAMAGE);
                    g_PlayerLifePoints -= ENEMY_ATTACK_DAMAGE; // Aplica o dano ao jogador
                    
                    if (g_PlayerLifePoints <= 0) {
                        printf("You Are Dead!!!\n");
                        g_PlayerAlive = false;
                    }
                }
            }
        }

        if (!g_PlayerAlive) {
            model = Matrix_Translate(g_PlayerPos.x, g_PlayerPos.y, g_PlayerPos.z) 
            * Matrix_Rotate_Y(g_PlayerRot.y);
             // Aplicamos transformações para o jogador cair
            model *= Matrix_Translate(0.0f, -0.6f, 0.0f)
                     * Matrix_Rotate_Z(-PI/2);

            // Desenhamos o jogador morto!
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_flash_green_uniform, 0);            
            glUniform1i(g_object_id_uniform, BODY);
            DrawVirtualObject("Object_915bd724.jpg");
            g_GameOver = true;
        }


        if (g_GameOver == true) {
            glfwPollEvents(); // Ainda processa eventos de teclado/mouse
            continue; // Ignora outras atualizações e vai direto para a próxima iteração
        }


        if (g_EnemyLifePoints <= 0) {
            //Aplicamos as transformações base
            model = Matrix_Translate(g_EnemyPos.x, g_EnemyPos.y, g_EnemyPos.z)
                    * Matrix_Rotate_Y(g_EnemyRot.y);

             // Aplicamos transformações para o adversário cair
            model *= Matrix_Translate(0.0f, -0.6f, 0.0f)
                     * Matrix_Rotate_Z(-PI/2);

            // Desenhamos o adversário morto!
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_flash_green_uniform, 0);            
            glUniform1i(g_object_id_uniform, ENEMY);
            DrawVirtualObject("Object_boss_zombie_01_d.jpg");
            //printf("You Win!!!\n");
            //g_GameOver = true;
        } else if (g_EnemySpawned) {
            // Computamos o efeito da gravidade com colisões no adversário
            ComputeGravity(g_EnemyPos, g_EnemySpeed, delta_t);

            // Calculamos o vetor da linha de visão do adversário
            glm::vec4 view = g_PlayerPos - g_EnemyPos;
            view.y = 0.0f;

            // Calculamos a rotação do adversário
            view = glm::normalize(view);
            float dot_product = glm::dot(view, { -1.0f, 0.0f, 0.0f, 0.0f });
            g_EnemyRot.y = (view.z >= 0) ? acos(dot_product) : - acos(dot_product);

            // Calculamos a aabb do adversário
            glm::vec4 aabb_min = Matrix_Translate(g_EnemyPos.x, g_EnemyPos.y, g_EnemyPos.z) * glm::vec4(g_VirtualScene["Object_915bd724.jpg"].bbox_min, 1.0f);
            glm::vec4 aabb_max = Matrix_Translate(g_EnemyPos.x, g_EnemyPos.y, g_EnemyPos.z) * glm::vec4(g_VirtualScene["Object_915bd724.jpg"].bbox_max, 1.0f);

            glm::vec4 ray_direction = glm::normalize(camera_view_vector) * ATTACK_DISTANCE;

            if (g_PlayerAttacked && RayIntersectsAABB(camera_position_c, ray_direction, aabb_min, aabb_max)) {
                g_LastHitTime = current_time;
                g_EnemyLifePoints -= ATTACK_DAMAGE;
            }

            if (current_time - g_LastHitTime <= FLASH_DURATION) {
                glUniform1i(g_flash_green_uniform, 1);            
            } else {
                glUniform1i(g_flash_green_uniform, 0);            
            }

            // Aplicamos as transformações e desenhamos
            model = Matrix_Translate(g_EnemyPos.x, g_EnemyPos.y, g_EnemyPos.z)
                    * Matrix_Rotate_Y(g_EnemyRot.y);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, ENEMY);
            DrawVirtualObject("Object_boss_zombie_01_d.jpg");
        }

        // Atualizamos a velocidade do jogador
        UpdatePlayerMovementSpeed();

        // Atualizamos a posição do jogador, após computada a velocidade
        UpdatePlayerPosition(delta_t);

        CheckPlayerBounds(g_PlayerPos, plane_size);

        // Atualize as caixas delimitadoras
        glm::vec3 playerScale = glm::vec3(1.0f, playerSize, 1.0f); // Tamanho aproximado do jogador
        glm::vec3 enemyScale = glm::vec3(1.0f, enemySize, 1.0f); // Tamanho aproximado do inimigo
        UpdateBoundingBox(playerBox, g_PlayerPos, playerScale);
        UpdateBoundingBox(enemyBox, g_EnemyPos, enemyScale);

        if (g_EnemyLifePoints > 0) {
            // Verifique colisão com o inimigo
            if (CheckPlayerEnemyCollision()) {
                // Reverte a posição do jogador se houver colisão com o inimigo
                g_PlayerPos = previousPlayerPos;
            }
        }

        // Verifique colisão com as árvores
        if (CheckPlayerTreeCollision()) {
            // Reverte a posição do jogador se houver colisão com uma árvore
            g_PlayerPos = previousPlayerPos;
        }

        // Calculamos a distância do jogador para o centro do mapa
        float player_radial_dist = sqrt(g_PlayerPos.x * g_PlayerPos.x + g_PlayerPos.z * g_PlayerPos.z);

        // Testamos se a distância do jogador para o centro foi suficiente para fazer nascer o adversário
        if (player_radial_dist > SPAWN_MIN_RADIUS) {
            g_EnemySpawned = true;
        }

        if (g_CameraLookAt) {
            // Aplicamos as transformações e desenhamos o jogador
            model = Matrix_Translate(g_PlayerPos.x, g_PlayerPos.y, g_PlayerPos.z)
                    * Matrix_Rotate_Y(g_Yaw);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, WEAPON);
            DrawVirtualObject("Object_f82f7c5f.jpg");
            glUniform1i(g_object_id_uniform, BODY);          
            DrawVirtualObject("Object_915bd724.jpg");
        } else {
            // Aplicamos as transformações da câmera
            model = Matrix_Translate(camera_position_c.x, camera_position_c.y, camera_position_c.z)
                    * Matrix_Rotate_Y(g_Yaw)
                    * Matrix_Rotate_Z(g_Pitch);

            // Aplicamos as transformações dos objetos
            model *= Matrix_Translate(-0.9f, -0.25f, 0.0f)
                     * Matrix_Rotate_Z(-PI / 2);

            if (current_time - g_LastAttackTime <= ANIMATION_DURATION) {
                float normalized_time = (current_time - g_LastAttackTime) / ANIMATION_DURATION;
                glm::vec4 position = CalculateWeaponAnimationPosition(normalized_time);
                glm::vec3 rotation = CalculateWeaponAnimationRotation(normalized_time);
                model *= Matrix_Translate(position.x, position.y, position.z)
                         * Matrix_Rotate_X(rotation.x)
                         * Matrix_Rotate_Y(rotation.y)
                         * Matrix_Rotate_Z(rotation.z);
            }

            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, WEAPON);
            DrawVirtualObject("Object_f82f7c5f.jpg");
        }

        // Desenhamos os modelos das árvores
        glDisable(GL_CULL_FACE);
        for (const auto &pos : tree_pos) {
            model = Matrix_Translate(pos.first, tree_y, pos.second) 
            * Matrix_Scale(g_TreeScaleX, g_TreeScaleY, g_TreeScaleZ);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, TREE);
            DrawVirtualObject("Object_farm_trees_rocks_flowers_D.jpg");
        }
        glEnable(GL_CULL_FACE);

        // Desenhamos o plano do chão
        model = Matrix_Translate(g_PlaneX, g_PlaneY, g_PlaneZ);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PLANE);
        DrawVirtualObject("object_0");

        // Reseta a detecção do ataque do jogador
        g_PlayerAttacked = false;

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que escala o modelo do plano e suas coordenadas de textura de acordo com o parâmetro
// g_PlaneScale
void ScalePlaneModelAndTexCoords(ObjModel* model) {
    for (auto& vertex : model->attrib.vertices) {
        vertex *= g_PlaneScale;
    }

    for (auto& texcoord : model->attrib.texcoords) {
        texcoord *= g_PlaneScale;
    }
}

// Atualiza a velocidade do jogador baseado nas teclas do movimentação pressionadas
void UpdatePlayerMovementSpeed() {
    g_PlayerSpeed = {0.0f, 0.0f, 0.0f, 0.0f};

    glm::mat4 rotation_matrix = Matrix_Rotate_Y(g_Yaw);

    glm::vec4 forward_direction = rotation_matrix * glm::vec4( -1.0f, 0.0f, 0.0f, 0.0f );
    glm::vec4 right_direction = rotation_matrix * glm::vec4( 0.0f, 0.0f, -1.0f, 0.0f );

    if (g_WKeyPressed) {
        g_PlayerSpeed += forward_direction; 
    }

    if (g_AKeyPressed) {
        g_PlayerSpeed -= right_direction;
    }

    if (g_SKeyPressed) {
        g_PlayerSpeed -= forward_direction;
    }

    if (g_DKeyPressed) {
        g_PlayerSpeed += right_direction;
    }

    if (g_PlayerSpeed != glm::vec4(0.0f)) {
        g_PlayerSpeed = glm::normalize(g_PlayerSpeed) * MOV_SPEED;
    }

    if (g_ShiftPressed) {
        g_PlayerSpeed = (g_PlayerSpeed) * MOV_SPEED * 0.3f;
    }
}

// Atualiza a posição do jogador, após computada a velocidade
void UpdatePlayerPosition(float delta_t) {
    g_PlayerPos += g_PlayerSpeed * delta_t;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    GLuint vertex_shader_id = LoadShader_Vertex("data/shaders/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("data/shaders/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    g_flash_green_uniform = glGetUniformLocation(g_GpuProgramID, "flash_green");
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage4"), 4);
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados 
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Função que aplica os efeitos da gravidade e testes de colisão sobre o jogador
void ComputeGravity(glm::vec4& pos, glm::vec4& vel, float delta_t) {
    // Calculamos a potencial velocidade do jogador no fim do intervalo de tempo
    float end_vel = vel.y - delta_t * GRAVITY;

    // Calculamos a potencial velocidade média do jogador no intervalo de tempo
    float avg_vel = (vel.y + end_vel) / 2;

    // Calculamos o efeito da gravidade sobre o jogador
    float time_to_collision = CalculateAABBToPlaneCollisionTime(
        g_VirtualScene["object_0"].bbox_max.y,
        g_VirtualScene["Object_915bd724.jpg"].bbox_min.y + pos.y, avg_vel
    );
    float fall_time = std::min(time_to_collision, delta_t);
    pos.y += fall_time * avg_vel;

    // Atualizamos a nova velocidade do cavaleiro
    if (fall_time == delta_t) {
        vel.y = end_vel;
    } else {
        vel.y = 0.0f;
    }
}

glm::vec4 CalculateWeaponAnimationPosition(float t) {
    // Ajustando os pontos de controle para que a lança comece vertical e se mova para horizontal
    glm::vec4 p1 = {0.0f, 0.0f, 0.0f, 1.0f};  // Ponto inicial (início quase vertical)
    glm::vec4 p2 = {0.0f, 1.5f, 0.0f, 1.0f};  // Controle para o movimento inicial (direção vertical)
    glm::vec4 p3 = {-0.5f, 1.0f, 0.0f, 1.0f}; // Controle para começar a transição para horizontal
    glm::vec4 p4 = {-1.5f, 0.0f, 0.0f, 1.0f}; // Ponto final (movimento horizontal)

    // Cálculo da posição da lança (weapon) no instante t com a curva de Bézier Cúbica
    glm::vec4 c = (float)pow((1 - t), 3) * p1
                + 3 * t * (float)pow((1 - t), 2) * p2
                + 3 * t * t * (1 - t) * p3
                + t * t * t * p4;

    return c;
}

glm::vec3 CalculateWeaponAnimationRotation(float t) {
    // Aumentando a amplitude da rotação para maior movimento
    float rotation_y = PI / 2 * t;  // Aumentando o fator de rotação ao longo do eixo Y
    float rotation_z = PI / 1 * t;  // Aumentando o fator de rotação ao longo do eixo Z
    return glm::vec3(0.0f, rotation_y, rotation_z);
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        float current_time = (float) glfwGetTime();
        if (current_time - g_LastAttackTime > ANIMATION_DURATION) {
            g_PlayerAttacked = true;
            g_LastAttackTime = current_time;
        }
    }
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    // Atualizamos parâmetros da câmera com os deslocamentos
    g_Yaw -= SENSITIVITY * dx;
    g_Pitch   += SENSITIVITY * dy;

    // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
    float phimax = 3.141592f/2;
    float phimin = -phimax;

    if (g_Pitch > phimax)
        g_Pitch = phimax;

    if (g_Pitch < phimin)
        g_Pitch = phimin;

    // Atualizamos as variáveis globais para armazenar a posição atual do
    // cursor como sendo a última posição conhecida do cursor.
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;
}


// Movimentação do personagem pelo cenário virtual
// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // Registra se o usuário pressionou ou soltou W
    if (key == GLFW_KEY_W)
    {
        switch (action) {
            case GLFW_PRESS:
                g_WKeyPressed = true;
                break;
            case GLFW_RELEASE:
                g_WKeyPressed = false;
                break;
        }
    }

    // Registra se o usuário pressionou ou soltou A
    if (key == GLFW_KEY_A)
    {
        switch (action) {
            case GLFW_PRESS:
                g_AKeyPressed = true;
                break;
            case GLFW_RELEASE:
                g_AKeyPressed = false;
                break;
        }
    }

    // Registra se o usuário pressionou ou soltou S
    if (key == GLFW_KEY_S)
    {
        switch (action) {
            case GLFW_PRESS:
                g_SKeyPressed = true;
                break;
            case GLFW_RELEASE:
                g_SKeyPressed = false;
                break;
        }
    }

    // Registra se o usuário pressionou ou soltou D
    if (key == GLFW_KEY_D)
    {
        switch (action) {
            case GLFW_PRESS:
                g_DKeyPressed = true;
                break;
            case GLFW_RELEASE:
                g_DKeyPressed = false;
                break;
        }
    }

// Registra se o usuário pressionou ou soltou a tecla Shift da esquerda
    if (key == GLFW_KEY_LEFT_SHIFT)
    {
        switch (action) {
            case GLFW_PRESS:
                g_ShiftPressed = true;
                break;
            case GLFW_RELEASE:
                g_ShiftPressed = false;
                break;
        }
    }

    // Alterna câmera livre e look-at ao pressionar a tecla "F5"
    if (key == GLFW_KEY_F5 && action == GLFW_PRESS)
    {
        g_CameraLookAt = !g_CameraLookAt; 
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
    const tinyobj::attrib_t                & attrib    = model->attrib;
    const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
    const std::vector<tinyobj::material_t> & materials = model->materials;

    printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
    printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
    printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
    printf("# of shapes    : %d\n", (int)shapes.size());
    printf("# of materials : %d\n", (int)materials.size());

    for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
        printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
    }

    for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
    }

    for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
    }

  // For each shape
    for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
        shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
        static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() == shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
        static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
        size_t fnum = shapes[i].mesh.num_face_vertices[f];
    
    printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
        static_cast<unsigned long>(fnum));

      // For each vertex in the face
    for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
            static_cast<long>(v), idx.vertex_index, idx.normal_index,
            idx.texcoord_index);
    }

    printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
        shapes[i].mesh.material_ids[f]);    
        index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
        static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
        printf("  tag[%ld] = %s ", static_cast<long>(t),
        shapes[i].mesh.tags[t].name.c_str());
        printf(" ints: [");
    for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
            printf(", ");
        }
    }
    printf("]");

    printf(" floats: [");
    for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double> (shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
        printf(", ");
        }
    }
    printf("]");

    printf(" strings: [");
        for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
        printf(", ");
        }
    }
    printf("]");
    printf("\n");
    }
    }

    for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
    materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
        static_cast<const double>(materials[i].ambient[0]),
        static_cast<const double>(materials[i].ambient[1]),
        static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
        static_cast<const double>(materials[i].diffuse[0]),
        static_cast<const double>(materials[i].diffuse[1]),
        static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
        static_cast<const double>(materials[i].specular[0]),
        static_cast<const double>(materials[i].specular[1]),
        static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
        static_cast<const double>(materials[i].transmittance[0]),
        static_cast<const double>(materials[i].transmittance[1]),
        static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n", 
        static_cast<const double>(materials[i].emission[0]),
        static_cast<const double>(materials[i].emission[1]),
        static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n", static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n", static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n", materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
        printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
    }
}
