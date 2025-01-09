#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Cor interpolada pelos vértices. Para ser utilizada no modelo de Gouraud.
in vec3 vertex_color;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define WEAPON 0
#define BODY   1
#define PLANE  2
#define TREE   3
#define ENEMY  4

uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;
uniform sampler2D TextureImage4;

// Variável que controla o efeito de dano
uniform bool flash_green;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Vetor que define o meio do caminho entre v e l.
    vec4 h = normalize(v + l);

    // Coordenadas de textura U e V
    float U = texcoords.x;
    float V = texcoords.y;

    // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
    vec3 Kd;
    vec3 Ka;
    vec3 Ks;
    float q;

    // Calcula o fator da intensidade do efeito de dano
    float flash_factor = flash_green ? 0.25f : 0.0f;

    if (object_id == WEAPON) {
        Kd = texture(TextureImage0, vec2(U,V)).rgb;
        Ka = Kd * 0.5f;
        Ks = vec3(0.1f, 0.1f, 0.1f);
        q = 40.0f;
    } else if (object_id == BODY) {
        Kd = texture(TextureImage1, vec2(U,V)).rgb;
        Ka = Kd * 0.5f;
        Ks = vec3(0.1f, 0.1f, 0.1f);
        q = 40.0f;
    } else if (object_id == ENEMY) {
        Kd = mix(texture(TextureImage4, vec2(U,V)).rgb, vec3(0.0f, 1.0f, 0.0f), flash_factor);
        Ka = Kd * 0.5f;
        Ks = vec3(0.1f, 0.1f, 0.1f);
        q = 40.0f;
    } else if (object_id == PLANE) {
        Kd = texture(TextureImage2, vec2(U,V)).rgb;
        Ka = vec3(0.0f, 0.0f, 0.0f);
        Ks = vec3(0.0f, 0.0f, 0.0f);
        q = 1.0f;
    } else {
        Kd = vec3(0.0f, 0.0f, 0.0f);
        Ka = vec3(0.0f, 0.0f, 0.0f);
        Ks = vec3(0.0f, 0.0f, 0.0f);
        q = 1.0f;
    }

    if (object_id == TREE) {
        color.rgb = vertex_color;
    } else {
        // Espectro da fonte de iluminação
        vec3 I = vec3(0.98f, 1.00f, 0.92f);

        // Espectro da luz ambiente
        vec3 Ia = vec3(0.98f, 1.00f, 0.92f);

        // Equação de Iluminação
        vec3 lambert_diffuse_term = Kd * I * max(0, dot(n, l));
        vec3 ambient_term = Ka * Ia;
        vec3 phong_specular_term  = Ks * I * pow(max(0, dot(n, h)), q);

        color.rgb = lambert_diffuse_term + ambient_term + phong_specular_term;
    }

    // NOTE: Se você quiser fazer o rendering de objetos transparentes, é
    // necessário:
    // 1) Habilitar a operação de "blending" de OpenGL logo antes de realizar o
    //    desenho dos objetos transparentes, com os comandos abaixo no código C++:
    //      glEnable(GL_BLEND);
    //      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // 2) Realizar o desenho de todos objetos transparentes *após* ter desenhado
    //    todos os objetos opacos; e
    // 3) Realizar o desenho de objetos transparentes ordenados de acordo com
    //    suas distâncias para a câmera (desenhando primeiro objetos
    //    transparentes que estão mais longe da câmera).
    // Alpha default = 1 = 100% opaco = 0% transparente
    color.a = 1;

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);
} 
