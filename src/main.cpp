//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                 - Trabalho Final -
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

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
#include <iostream>

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
#include "collisions.h"


const float TRACK_MIN_X = -100.0f;
const float TRACK_MAX_X =  100.0f;
const float TRACK_MIN_Z = -5.0f;
const float TRACK_MAX_Z =  100.0f;

glm::vec4 g_SmoothCameraPos = glm::vec4(0.0f); // Posição suavizada da câmera
float g_CameraSmoothFactor = 4.0f; // Quanto maior, mais rápida a resposta

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

void ComputeGravity(glm::vec4& pos, glm::vec4& vel, float delta_t);

void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

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

// Posição da pista/plano
float TrackPositionX = 0.0f;
float TrackPositionY = 0.0f;
float TrackPositionZ = 0.0f;

// Posição dos arcos no plano
float ArcsPositionX = 0.0f;
float ArcsPositionY = 0.0f;
float ArcsPositionZ = -320.0f;

// Posição das pessoas no plano
float PeoplePositionX = 1.0f;
float PeoplePositionY = 0.8f;
float PeoplePositionZ = -330.0f;

// Posição da Grandma no plano
float GrandmaPositionX = 1.0f;
float GrandmaPositionY = 1.25f;
float GrandmaPositionZ = 404.5f;

// Posição dos guardrails no plano
float GuardPositionX = 0.0f;
float GuardPositionY = 0.0f;
float GuardPositionZ = 0.0f;

// Escala do plano
float g_PlaneScale = 50.0f;

// Posição do Carro
glm::vec4 g_CarPos;

// Posição do Carro 2 player (pc)
glm::vec4 g_CarPos_pc;

//Velocidade do Carro 2 player (pc)
float g_CarSpeed_pc = 0.0f;

// Variável que controla qual câmera usar: falsa para câmera livre, verdadeira para look-at.
bool g_CameraLookAt = true;

// Variáveis para a Câmera Livre
glm::vec4 g_FreeCameraPosition   = glm::vec4(0.0f, 1.0f, 2.5f, 1.0f); // Posição inicial da câmera livre
glm::vec4 g_FreeCameraViewVector = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f); // Vetor "view" da câmera livre
glm::vec4 g_FreeCameraUpVector   = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f); // Vetor "up" da câmera livre

float g_FreeCameraSpeed = 3.0f; // Velocidade de movimento da câmera livre (unidades/s)
float g_FreeCameraPanSpeed = 0.005f; // Velocidade de rotação da câmera livre (rad/pixel)

// Variáveis de estado das teclas para a câmera livre
bool g_WKeyPressedFree    = false;
bool g_SKeyPressedFree    = false;
bool g_AKeyPressedFree    = false;
bool g_DKeyPressedFree    = false;
bool g_BKeyPressedFree    = false;

// Variáveis para simulação de física
float g_Gravity = -9.8f; // Aceleração da gravidade (em unidades/s^2, ajuste conforme necessário)
glm::vec3 g_CarVelocity = glm::vec3(0.0f, 0.0f, 0.0f); // Velocidade do carro
float g_CarYaw = 0.0f; // Rotação do carro em torno do eixo Y (guinada)
float g_CarYaw_pc = 0.0f; // Rotação do carro em torno do eixo Y (guinada)

// Variável para cálculo do tempo entre frames (deltaTime)
double g_LastTime = 0.0;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem

// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis de estado das teclas para movimentação do carro
bool g_WKeyPressed = false;
bool g_AKeyPressed = false;
bool g_SKeyPressed = false;
bool g_DKeyPressed = false;

// Parâmetros de movimento do carro
float g_CarSpeed = 0.0f; // Velocidade atual do carro
float g_CarMaxSpeed = 25.0f; // Velocidade máximas
float g_CarAcceleration = 5.0f; // Aceleração
float g_CarDeceleration = 5.0f; // Desaceleração (freio)
float g_CarRotationSpeed = 1.0f; // Velocidade de rotação (guinada)

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;
bool g_SideCameraActive = false;
float g_BezierTime = 0.0f;
glm::vec3 g_BezierP0, g_BezierP1, g_BezierP2, g_BezierP3;

// Função auxiliar para movimentação com curva Bézier cúbica.
glm::vec3 BezierCubic(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, float t)
{
    float u = 1.0f - t;
    return u*u*u*p0 + 3*u*u*t*p1 + 3*u*t*t*p2 + t*t*t*p3;
}

double g_GameStartTime = 0.0;
bool g_RaceStarted = false;

int g_DifficultyLevel=1;        // 0 = fácil, 1 = médio, 2 = difícil
bool g_DifficultyChosen = false;  // True se o jogador já escolheu a dificuldade


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
    window = glfwCreateWindow(800, 600, "-= Arrancadão 2025 - desafio dos 400 metros =-", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

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

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../data/asphalt.jpg");               // TextureImage0
    LoadTextureImage("../data/tc-car_surface.jpg");        // TextureImage1
    LoadTextureImage("../data/tc-wall.jpg");               // TextureImage2
    LoadTextureImage("../data/arcos.jpg");                 // TextureImage3
    LoadTextureImage("../data/guardRail.jpg");             // TextureImage4
    LoadTextureImage("../data/ruedas.jpg");                // TextureImage5
    LoadTextureImage("../data/ventanas.jpg");              // TextureImage6
    LoadTextureImage("../data/tc-car_surface_pc.jpg");     // TextureImage7
    LoadTextureImage("../data/Tex_6.jpg");                 // TextureImage8
    LoadTextureImage("../data/grandma.jpg");               // TextureImage9

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel trackmodel("../data/track.obj");
    ScalePlaneModelAndTexCoords(&trackmodel);
    ComputeNormals(&trackmodel);
    BuildTrianglesAndAddToVirtualScene(&trackmodel);

    ObjModel carmodel("../data/car.obj");
    ComputeNormals(&carmodel);
    BuildTrianglesAndAddToVirtualScene(&carmodel);

    ObjModel wallmodel("../data/wall.obj");
    ComputeNormals(&wallmodel);
    BuildTrianglesAndAddToVirtualScene(&wallmodel);

    ObjModel arcsmodel("../data/arcos.obj");
    ComputeNormals(&arcsmodel);
    BuildTrianglesAndAddToVirtualScene(&arcsmodel);

    ObjModel guardRailmodel("../data/guardRail.obj");
    ComputeNormals(&guardRailmodel);
    BuildTrianglesAndAddToVirtualScene(&guardRailmodel);

    ObjModel carpcmodel("../data/car_pc.obj");
    ComputeNormals(&carpcmodel);
    BuildTrianglesAndAddToVirtualScene(&carpcmodel);

    ObjModel peoplemodel("../data/people.obj");
    ComputeNormals(&peoplemodel);
    BuildTrianglesAndAddToVirtualScene(&peoplemodel);

    ObjModel grandmamodel("../data/grandma.obj");
    ComputeNormals(&grandmamodel);
    BuildTrianglesAndAddToVirtualScene(&grandmamodel);

    std::vector<glm::vec3> wall_positions = {
    {  -6.0f, 0.0f, 405.0f},
    {  -4.0f, 0.0f, 405.0f},
    {  -2.0f, 0.0f, 405.0f},
    {   0.0f, 0.0f, 405.0f},
    {   2.0f, 0.0f, 405.0f},
    {   4.0f, 0.0f, 405.0f},
    {   6.0f, 0.0f, 405.0f}
};

    std::vector<glm::vec3> guardRail_positions;

    // Inicializa a sequência de posições ao longo do eixo X e Z
    float z_increment = 5.0f; // Incremento do eixo Z
    float x_values[] = {-5.0f, 5.0f}; // Valores de X alternando entre -5.0f e 5.0f

    // Gerar posições com alternância de X
    for (int i = 0; i < 2; ++i) {  // i = 0 -> X = -5.0f, i = 1 -> X = 5.0f
        float x = x_values[i];  // Definir valor de X para a iteração

        // Gerar as posições ao longo do eixo Z 
        for (float z = -320.0f; z <= 400.0f; z += z_increment) {
            // Adiciona a posição no vetor guardRail_positions
            guardRail_positions.push_back(glm::vec3(x, 0.8f, z));
        }
    }

    // Configura posição inicial do Carro. Certifica-se de que a parte inferior do carro está na mesma altura que o plano.
    //float car_min_y = g_VirtualScene["the_car"].bbox_min.y;
    float carSize = g_VirtualScene["tc-car_surface.jpg"].bbox_max.y - g_VirtualScene["tc-car_surface.jpg"].bbox_min.y;
    float carSizepc = g_VirtualScene["tc-car_surface_pc.jpg"].bbox_max.y - g_VirtualScene["tc-car_surface_pc.jpg"].bbox_min.y;

    // TrackPositionY é a coordenada Y do plano. Se o plano estiver em y=0, então TrackPositionY = 0.0f.
    // A posição Y do carro deve ser TrackPositionY menos a coordenada Y mínima do modelo do carro,
    // para que a base do carro coincida com a altura do plano.
    g_CarPos = { 1.0f, 0.0f + carSize, -328.6f, 10.0f };
    g_CarPos_pc = { -1.0f, 0.0f + carSizepc, -328.6f, 0.0f };

    // Inicializa o tempo para o cálculo do deltaTime
    g_LastTime = glfwGetTime(); // Moved to be properly initialized here before the loop

    g_GameStartTime = g_LastTime; // tempo da inicialização


    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Aqui executamos as operações de renderização

        // ===============================================
        // Atualiza deltaTime no início do frame
        // ===============================================
        double current_time = glfwGetTime();
        float deltaTime = (float)(current_time - g_LastTime);
        g_LastTime = current_time;

        //float elapsed = (float)(current_time - g_GameStartTime);
        float elapsed = g_DifficultyChosen ? (float)(current_time - g_GameStartTime) : 0.0f;


        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(0.53f, 0.81f, 0.98f, 1.0f); // Atribuição da cor Azul celeste

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);

        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().
        //float r = g_CameraDistance;
        //float y = r*sin(g_CameraPhi);
        //float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        //float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

        // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
        // Veja slides 195-227 e 229-234 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        //glm::vec4 camera_position_c  = glm::vec4(x,y,z,1.0f); // Ponto "c", centro da câmera
        //glm::vec4 camera_lookat_l    = glm::vec4(0.0f,0.0f,0.0f,1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
        //glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
        //glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        //glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        glm::mat4 view;

        if (g_SideCameraActive)
        {
            glm::vec3 carPos = glm::vec3(g_CarPos);
            glm::vec3 right = glm::vec3(cos(g_CarYaw), 0.0f, -sin(g_CarYaw));  // lado direito
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

            g_BezierP0 = carPos + up * 1.5f;  // Posição inicial da câmera (acima do carro)
            g_BezierP1 = carPos + right * 2.5f + up * 3.0f;
            g_BezierP2 = carPos + right * 5.5f + up * 2.5f;
            g_BezierP3 = carPos + right * 8.0f + up * 2.0f;

            g_BezierTime += deltaTime;
            if (g_BezierTime > 1.0f) g_BezierTime = 1.0f;

            // DEBUG
            // printf("Bezier time: %f\n", g_BezierTime);

            glm::vec3 cam_pos = BezierCubic(g_BezierP0, g_BezierP1, g_BezierP2, g_BezierP3, g_BezierTime);
            glm::vec4 camera_position_c = glm::vec4(cam_pos, 1.0f);
            glm::vec4 look_at = glm::vec4(carPos + up * 1.0f, 1.0f);
            glm::vec4 view_vector = look_at - camera_position_c;
            glm::vec4 up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

            view = Matrix_Camera_View(camera_position_c, view_vector, up_vector);
        }
        else
        {
            g_BezierTime = 0.0f;

            if (g_CameraLookAt)
            {
                glm::vec3 car_direction = glm::vec3(sin(g_CarYaw), 0.0f, cos(g_CarYaw));
                glm::vec3 camera_offset = -3.5f * car_direction + glm::vec3(0.0f, 1.0f, 0.0f);
                glm::vec4 camera_position_c = glm::vec4(glm::vec3(g_CarPos) + camera_offset, 1.0f);
                glm::vec4 camera_lookat_l = glm::vec4(g_CarPos.x, g_CarPos.y + 1.0f, g_CarPos.z, 1.0f);
                glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c;
                glm::vec4 camera_up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
                view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);
            }
            else
            {
                // glm::vec3 car_direction = glm::vec3(sin(g_CarYaw), 0.0f, cos(g_CarYaw));
                // glm::vec3 eye_offset = glm::vec3(0.0f, 1.5f, 0.0f);
                // glm::vec3 camera_position = glm::vec3(g_CarPos) + eye_offset;
                // glm::vec3 camera_target = camera_position + car_direction;

                // -------------------------------------------------------------------------
                glm::vec3 car_direction = glm::vec3(sin(g_CarYaw), 0.0f, cos(g_CarYaw));
                glm::vec3 eye_offset = glm::vec3(0.0f, 1.5f, 0.0f); // altura acima do capô
                glm::vec3 camera_position = glm::vec3(g_CarPos) + eye_offset;

                // Direção levemente inclinada para trás
                glm::vec3 reverse_direction = -car_direction;
                glm::vec3 slight_offset = glm::vec3(0.0f, 0.2f, 0.0f); // um pouco para cima
                glm::vec3 camera_target = camera_position + reverse_direction + slight_offset;

                // ---------------------------------------------------------------------------

                glm::vec4 camera_position_c = glm::vec4(camera_position, 1.0f);
                glm::vec4 camera_lookat_l = glm::vec4(camera_target, 1.0f);
                glm::vec4 view_vector = camera_lookat_l - camera_position_c;
                glm::vec4 up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
                view = Matrix_Camera_View(camera_position_c, view_vector, up_vector);
            }
        }

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -100.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        #define TRACK   0
        #define CAR     1
        #define WALL    2
        #define ARCS    3
        #define GUARD   4
        #define WHEEL   5
        #define WINDOW  6
        #define PC      7
        #define PEOPLE  8
        #define GRANDMA 9

        // Desenhamos o plano do chão
        //model = Matrix_Translate(0.0f,-1.1f,0.0f);
        model = Matrix_Translate(TrackPositionX, TrackPositionY, TrackPositionZ);
        model = model * Matrix_Scale(1.0f, 1.0f, 1.0f); // Aumenta a pista lateral e longitudinalmente
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE ,  glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, TRACK);
        DrawVirtualObject("the_track");

        for (const auto& pos : wall_positions)
        {
            model = Matrix_Translate(pos.x, pos.y, pos.z);
            model = model * Matrix_Scale(1.0f, 1.0f, 1.0f);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, WALL);
            DrawVirtualObject("the_wall");

        }

        model = Matrix_Translate(ArcsPositionX, ArcsPositionY, ArcsPositionZ);
        model = model * Matrix_Scale(1.0f, 1.0f, 1.0f);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, ARCS);
            DrawVirtualObject("the_arcs");

        for (const auto& pos : guardRail_positions)
        {
            model = Matrix_Translate(pos.x, pos.y, pos.z);
            model = model * Matrix_Scale(0.8f, 0.8f, 0.8f); // Mudar a escala dos cones
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, GUARD);
            DrawVirtualObject("the_guardRail");

        }

        model = Matrix_Translate(PeoplePositionX, PeoplePositionY, PeoplePositionZ);
        model = model * Matrix_Rotate_Y(0.707 * PeoplePositionX);
        model = model * Matrix_Scale(2.0f, 2.0f, 2.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PEOPLE);
        DrawVirtualObject("Object_casualMan_28_0");

        model = Matrix_Translate(GrandmaPositionX, GrandmaPositionY, GrandmaPositionZ);
        model = model * Matrix_Rotate_Y(-1.4 * GrandmaPositionX);
        model = model * Matrix_Scale(1.0f, 1.0f, 1.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, GRANDMA);
        DrawVirtualObject("Object_TexMap_0");


    //  ===============================================
    //  Lógica de Física (Implementação da Gravidade)
    //  ===============================================

        // Calcula o tempo decorrido desde o último frame
        //double current_time = glfwGetTime();
        //float deltaTime = (float)(current_time - g_LastTime);
        g_LastTime = current_time;

        // Aplica gravidade na velocidade vertical
        g_CarVelocity.y += g_Gravity * deltaTime;

        // Atualiza a posição do carro com base na velocidade
        g_CarPos.y += g_CarVelocity.y * deltaTime;


    //  ====================================================================================================
    //  Teste de Colisão (Ponto para AABB)
    //
        float plane_size = g_VirtualScene["the_track"].bbox_max.x - g_VirtualScene["the_track"].bbox_min.x;
        CheckCarbyBounds(g_CarPos, plane_size);
     //  ====================================================================================================

       // Obtém a altura mínima do modelo dos carros para detecção de colisão com o plano
       // float car_min_y = g_VirtualScene["the_car"].bbox_min.y;
       // float car_pc_min_y = g_VirtualScene["the_car_pc"].bbox_min.y;
        // Calcula a altura da base do carro em relação ao seu ponto de origem
      //  float car_base_offset = car_min_y;

        // Calcula a altura da base do carro do player 2 (pc) em relação ao seu ponto de origem
       // float car_pc_base_offset = car_pc_min_y;

        // Posição Y do chão/plano
      //  float plane_y_position = TrackPositionY;

        // Detecção de colisão com o plano
        // Se a base do carro (g_CarPos.y + car_base_offset) estiver abaixo do plano,
        // ajusta a posição para que fique em cima do plano e zera a velocidade vertical.
       // if (g_CarPos.y + car_base_offset < plane_y_position)
       // {

       // g_CarPos.y = plane_y_position - car_base_offset;  // Subtrai o offset aqui
       // g_CarVelocity.y = 0.0f; // Para o carro ao atingir o chão
       // }

        // Garante que o carro esteja sempre no plano ou acima dele, eliminando o "flutuar"
      //  g_CarPos.y = glm::max(g_CarPos.y, plane_y_position - car_base_offset);

        // Garante que o carro 2 esteja sempre no plano ou acima dele, eliminando o "flutuar"
      //  g_CarPos_pc.y = glm::max(g_CarPos.y, plane_y_position - car_pc_base_offset);
// ===============================================
// Colisão com o plano
// ===============================================
// Obtem informações da cena
glm::vec3 bbox_min_car = g_VirtualScene["the_car"].bbox_min;
glm::vec3 bbox_max_car = g_VirtualScene["the_car"].bbox_max;

glm::vec3 bbox_min_car_pc = g_VirtualScene["the_car_pc"].bbox_min;
glm::vec3 bbox_max_car_pc = g_VirtualScene["the_car_pc"].bbox_max;

glm::vec3 plane_bbox_min = g_VirtualScene["the_track"].bbox_min;
glm::vec3 plane_bbox_max = g_VirtualScene["the_track"].bbox_max;
glm::vec3 plane_position = glm::vec3(TrackPositionX, TrackPositionY, TrackPositionZ);
glm::vec3 plane_scale = glm::vec3(1.0f);
glm::vec3 plane_min = plane_position + plane_bbox_min * plane_scale;
glm::vec3 plane_max = plane_position + plane_bbox_max * plane_scale;

   ResolveCarGroundCollision(g_CarPos, bbox_min_car, plane_position.y);
   ResolveCarGroundCollision(g_CarPos_pc, bbox_min_car_pc, plane_position.y);
// ===============================================

    // ===============================================
    // Lógica de Movimento do Carro
    // ===============================================

        if (elapsed >= 5.0f)
        {
            g_RaceStarted = true;

        float current_acceleration = g_CarAcceleration;

        // Aceleração/Desaceleração
        if (g_WKeyPressed) {
            g_CarSpeed += current_acceleration * deltaTime;
        } else if (g_SKeyPressed) {
            g_CarSpeed -= g_CarDeceleration * deltaTime;
        } else {
            // Aplica atrito/desaceleração natural quando nenhuma tecla de movimento está pressionada
            if (g_CarSpeed > 0) {
                g_CarSpeed -= g_CarDeceleration * deltaTime;
                if (g_CarSpeed < 0) g_CarSpeed = 0;
            } else if (g_CarSpeed < 0) {
                g_CarSpeed += g_CarDeceleration * deltaTime;
                if (g_CarSpeed > 0) g_CarSpeed = 0;
            }
        }

        // Limita a velocidade máxima
        g_CarSpeed = glm::clamp(g_CarSpeed, -g_CarMaxSpeed, g_CarMaxSpeed);

        // Rotação (guinada)
        if (g_AKeyPressed && g_CarSpeed != 0.0f) { // Só vira se estiver em movimento
            g_CarYaw += g_CarRotationSpeed * deltaTime * (g_CarSpeed > 0 ? 1.0f : -1.0f); // Inverte a rotação se estiver dando ré
        }
        if (g_DKeyPressed && g_CarSpeed != 0.0f) { // Só vira se estiver em movimento
            g_CarYaw -= g_CarRotationSpeed * deltaTime * (g_CarSpeed > 0 ? 1.0f : -1.0f); // Inverte a rotação se estiver dando ré
        }

        // Atualiza a posição do carro com base na velocidade e direção
        glm::vec3 direction_vector = glm::vec3(sin(g_CarYaw), 0.0f, cos(g_CarYaw));
        g_CarPos.x += direction_vector.x * g_CarSpeed * deltaTime;
        g_CarPos.z += direction_vector.z * g_CarSpeed * deltaTime;


    // ===============================================
    // Fim da Lógica de Movimento do Carro
    // ===============================================

    // ==================================================================
    // Lógica bem simples do Movimento do Carro do player 2 - IA (car_pc)
    // ==================================================================

    //static float g_CarSpeed_pc = 0.0f;
    float g_CarMaxSpeed_pc; // velocidade máxima da IA
    float g_CarAcceleration_pc; // aceleração IA

    switch (g_DifficultyLevel)
    {
        case 0: // Fácil
            g_CarMaxSpeed_pc = 15.0f;
            g_CarAcceleration_pc = 3.0f;
            break;
        case 1: // Médio
            g_CarMaxSpeed_pc = 25.0f;
            g_CarAcceleration_pc = 4.5f;
            break;
        case 2: // Difícil
            g_CarMaxSpeed_pc = 50.0f;
            g_CarAcceleration_pc = 9.5f;
            break;
        default: // Medio
            g_CarMaxSpeed_pc = 25.0f;
            g_CarAcceleration_pc = 4.0f;
            break;
    }

    if (g_CarPos_pc.z < 400.0f)
    {
        // Aumenta a velocidade até o máximo
        g_CarSpeed_pc += g_CarAcceleration_pc * deltaTime;
        g_CarSpeed_pc = glm::min(g_CarSpeed_pc, g_CarMaxSpeed_pc);

        // Atualiza posição da IA (sempre para frente, sem rotação)
        glm::vec3 dir_pc = glm::vec3(sin(g_CarYaw_pc), 0.0f, cos(g_CarYaw_pc));
        g_CarPos_pc.x += dir_pc.x * g_CarSpeed_pc * deltaTime;
        g_CarPos_pc.z += dir_pc.z * g_CarSpeed_pc * deltaTime;
    }
    else
    {
        // Parou ao final da pista
        g_CarSpeed_pc = 0.0f;
    }
}

// ===============================================
// Colisão Esfera vs Esfera entre os dois carros
// ===============================================

// Centro das esferas = posição dos carros
glm::vec3 center_player = glm::vec3(g_CarPos);
glm::vec3 center_pc     = glm::vec3(g_CarPos_pc);

// Raio estimado dos carros
float radius_player = 1.0f;
float radius_pc     = 1.0f;

//float radius_player = glm::length(g_VirtualScene["the_car"].bbox_max - g_VirtualScene["the_car"].bbox_min) / 2.5f;
//float radius_pc     = glm::length(g_VirtualScene["the_car_pc"].bbox_max - g_VirtualScene["the_car_pc"].bbox_min) / 2.5f;

//if (CheckSphereCollision(center_player, radius_player, center_pc, radius_pc)) {
    //std::cout << "Colisão entre carros detectada (esfera vs esfera)!" << std::endl;

    // Resposta simples: anula as velocidades
   // g_CarSpeed = 0.0f;
   // g_CarSpeed_pc = 0.0f;

    // Separa os carros suavemente para não ficarem sobrepostos
   // glm::vec3 direction = glm::normalize(center_player - center_pc);
   // float overlap = (radius_player + radius_pc) - glm::distance(center_player, center_pc);

    // Aplica deslocamento de recuo proporcional
   // g_CarPos     += glm::vec4(direction * (overlap * 0.5f), 0.0f);
   // g_CarPos_pc  -= glm::vec4(direction * (overlap * 0.5f), 0.0f);
//}

if (ResolveSphereCollision(g_CarPos, radius_player, g_CarSpeed,
                           g_CarPos_pc, radius_pc, g_CarSpeed_pc)) {
    //std::cout << "Colisão entre carros detectada (esfera vs esfera)!" << std::endl;
}

// ===============================================
// FIM da Colisão Esfera vs Esfera entre os dois carros
// ===============================================
// ===============================================
// Colisão com paredes visíveis
// ===============================================
// Obtem informações da cena
//glm::vec3 bbox_min_car = g_VirtualScene["the_car"].bbox_min;
//glm::vec3 bbox_max_car = g_VirtualScene["the_car"].bbox_max;

glm::vec3 direction = glm::vec3(sin(g_CarYaw), 0.0f, cos(g_CarYaw));
glm::vec3 move = direction * g_CarSpeed * deltaTime;
glm::vec4 tentativeCarPos = g_CarPos + glm::vec4(move, 0.0f);
BoundingBox tentativeBox = ComputeCarAABB(tentativeCarPos, bbox_min_car, bbox_max_car);

bool hitWall = false;

if (!hitWall) {
    hitWall = ResolveCarWallCollision(
        tentativeCarPos,
        g_CarPos,
        bbox_min_car,
        bbox_max_car,
        g_CarSpeed,
        wall_positions,
        g_VirtualScene["the_wall"].bbox_min,
        g_VirtualScene["the_wall"].bbox_max,
        glm::vec3(0.01f)
    );
}
// ===============================================
// Colisão com os guard rails (cubo vs cubo)
// ===============================================
if (!hitWall) {
    hitWall = ResolveCarWallCollision(
        tentativeCarPos,
        g_CarPos,
        bbox_min_car,
        bbox_max_car,
        g_CarSpeed,
        guardRail_positions,
        g_VirtualScene["the_guardRail"].bbox_min,
        g_VirtualScene["the_guardRail"].bbox_max,
        glm::vec3(0.8f) // Escala aplicada na renderização
    );
}
// ===============================================
// Aplicação do movimento e tratamento de colisão
// ===============================================
if (!hitWall) {
    g_CarPos = tentativeCarPos; // Movimento aceito
} else {
    g_CarPos = tentativeCarPos; // Aplicar posição corrigida com empurrão
    g_CarSpeed = 0.0f;
}
// ===============================================
// Saída do Plano/Paredes invisíveis
// ===============================================
//BoundingBox finalBox = ComputeCarAABB(g_CarPos, bbox_min_car, bbox_max_car);

//bool insidePlane =
//    finalBox.min.x >= plane_min.x && finalBox.max.x <= plane_max.x &&
//    finalBox.min.z >= plane_min.z && finalBox.max.z <= plane_max.z;

//if (insidePlane) {
//    ResolveCarGroundCollision(g_CarPos, bbox_min_car, plane_position.y);
//} else {
//    std::cout << "Carro saiu do plano. Vai cair!" << std::endl;
//}
// ===============================================
// FIM DA LÓGICA DE COLISÃO
// ===============================================

        // Desenhamos o modelo do carro usando a posição e rotação atualizadas
        model = Matrix_Translate(g_CarPos.x, g_CarPos.y + 0.075f, g_CarPos.z);
        //model = model * Matrix_Scale(0.5f, 0.5f, 0.5f); // reduz o carro pela metade
        model = model * Matrix_Rotate_Y(g_CarYaw); // Aplica a rotação do carro
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, CAR);
        DrawVirtualObject("the_car");       
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WHEEL);
        DrawVirtualObject("ruedas");
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WINDOW);
        DrawVirtualObject("ventanas");

        // Desenhamos o modelo do carro usando a posição e rotação atualizadas
        model = Matrix_Translate(g_CarPos_pc.x, g_CarPos_pc.y, g_CarPos_pc.z);
        //model = model * Matrix_Scale(0.5f, 0.5f, 0.5f); // reduz o carro pela metade
        model = model * Matrix_Rotate_Y(g_CarYaw_pc); // Aplica a rotação do carro
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PC);
        DrawVirtualObject("the_car_pc");

        //g_CarPos.x = std::max(TRACK_MIN_X, std::min(g_CarPos.x, TRACK_MAX_X));
        //g_CarPos.z = std::max(TRACK_MIN_Z, std::min(g_CarPos.z, TRACK_MAX_Z));

        // ===============================================
        // Fim da Lógica de Física
        // ===============================================

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        if (!g_RaceStarted)
        {
            if (!g_DifficultyChosen)
            {
                TextRendering_PrintString(window, "Escolha a dificuldade para iniciar:", -0.35f, 0.4f, 1.52f);
                TextRendering_PrintString(window, "1 - Easy",    -0.35f, 0.3f, 1.2f);
                TextRendering_PrintString(window, "2 - Medium",  -0.35f, 0.2f, 1.2f);
                TextRendering_PrintString(window, "3 - Hard",    -0.35f, 0.1f, 1.2f);
            }
            else
            {
                int countdown = 5 - (int)elapsed;
                char countdown_text[32];
                snprintf(countdown_text, sizeof(countdown_text), "Arrancada em: %d", countdown);
                TextRendering_PrintString(window, countdown_text, -0.30f, 0.5f, 2.0f);

                const char* dificuldade_textos[] = {"Easy", "Medium", "Hard"};
                char difftxt[64];
                snprintf(difftxt, sizeof(difftxt), "Dificuldade: %s", dificuldade_textos[g_DifficultyLevel]);
                TextRendering_PrintString(window, difftxt, -0.65f, 0.0f, 1.2f);
            }
        }

        // Mostra a velocidade em tempo real
        float speed_kmh = g_CarSpeed * 3.6f *3.f;
        char velocimetro_texto[64];
        snprintf(velocimetro_texto, sizeof(velocimetro_texto), "Velocidade: %.1f km/h", speed_kmh);
        TextRendering_PrintString(window, velocimetro_texto, -0.95f, 0.9f, 1.0f);

        if (g_CarPos.z >= 400.0f)
        {
            TextRendering_PrintString(window, "YOU WON!", -0.2f, 0.8f, 2.0f);
        }
        else if (g_CarPos_pc.z >= 400.0f)
        {
            TextRendering_PrintString(window, "YOU LOST!", -0.2f, 0.8f, 2.0f);
        }

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
    GLuint vertex_shader_id = LoadShader_Vertex("../data/shaders/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../data/shaders/shader_fragment.glsl");

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
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage5"), 5);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage6"), 6);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage7"), 7);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage8"), 8);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage9"), 9);
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

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    if (g_LeftMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da câmera com os deslocamentos
        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   += 0.01f*dy;

        if (!g_CameraLookAt)
        {
            glm::mat4 yaw = Matrix_Rotate_Y(-g_FreeCameraPanSpeed * dx);
            glm::mat4 pitch = Matrix_Rotate_X(-g_FreeCameraPanSpeed * dy);
            glm::vec4 direction = pitch * yaw * g_FreeCameraViewVector;
            g_FreeCameraViewVector = glm::normalize(direction);
            return;
        }

        // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
        float phimax = 3.141592f/2;
        float phimin = -phimax;

        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;

        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_RightMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_ForearmAngleZ -= 0.01f*dx;
        g_ForearmAngleX += 0.01f*dy;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_MiddleMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_TorsoPositionX += 0.01f*dx;
        g_TorsoPositionY -= 0.01f*dy;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    if (action == GLFW_PRESS)
    {
        if (!g_DifficultyChosen)
        {
            if (key == GLFW_KEY_1)
            {
                g_DifficultyLevel = 0;
                g_DifficultyChosen = true;
                g_GameStartTime = glfwGetTime();
            }
            else if (key == GLFW_KEY_2)
            {
                g_DifficultyLevel = 1;
                g_DifficultyChosen = true;
                g_GameStartTime = glfwGetTime();
            }
            else if (key == GLFW_KEY_3)
            {
                g_DifficultyLevel = 2;
                g_DifficultyChosen = true;
                g_GameStartTime = glfwGetTime();
            }
        }
    }

    if (key == GLFW_KEY_F5 && action == GLFW_PRESS)
    {
                g_CameraLookAt = !g_CameraLookAt;
    }

    if (key == GLFW_KEY_SPACE)
    {
        if (action == GLFW_PRESS)
            g_SideCameraActive = true;
        else if (action == GLFW_RELEASE)
            g_SideCameraActive = false;
    }

 // Acelera para frente, se a tecla W for pressionada
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

    // Se a tecla W for pressionada junto com a tedcla A, o carro vai para a esquerda
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

    // Da a ré se o usuário pressionar a tecla S
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

    // Se a tecla W for pressionada junto com a tedcla D, o carro vai para a direita
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

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}


// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
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

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

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
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
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
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
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
