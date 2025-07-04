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

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define TRACK 0
#define CAR   1
#define WALL  2
#define ARCS  3

uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;

uniform vec3 Ka;  // Ambiente
uniform vec3 Kd;  // Difusa
uniform vec3 Ks;  // Especular
uniform float Ns; // Brilho especular (shininess)




// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

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

    // Coordenadas de textura U e V
    float U = texcoords.x;
    float V = texcoords.y;

    
    
    if ( object_id == 0 ) //TRACK
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

    }
    else if ( object_id == 1 ) //CAR
    {
        U = texcoords.x;
        V = texcoords.y;
    }
        if ( object_id == 2 ) //WALL
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

    }


else if (object_id == 3) // ARCS como toro
{
    vec3 pos = position_model.xyz;

    // --- Parâmetros do toro ---
    float R = 1.0; // Raio do círculo maior (distância do centro ao centro do tubo)
    float r = 0.3; // Raio do tubo (menor círculo)
    
    // ângulo ao redor do toro (major circle) no plano XZ
    float theta = atan(pos.z, pos.x); // [-pi, pi]

    // Posição do centro do tubo mais próximo (projeção no plano XZ)
    float xz_len = length(pos.xz);
    vec3 center = vec3(pos.x, 0.0, pos.z) * (R / xz_len);

    // Vetor do centro do tubo até o ponto (direção do minor circle)
    vec3 tube_vec = pos - center;

    // ângulo ao redor do tubo (minor circle)
    float phi = atan(tube_vec.y, length(tube_vec.xz)); // [-pi, pi]

    // Normalização para [0,1]
    float U = (theta + M_PI) / (2.0 * M_PI);
    float V = (phi + M_PI) / (2.0 * M_PI);

    // Usa coordenadas de textura U e V
}









    // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
    //vec3 Kd0 = texture(TextureImage2, vec2(U,V)).rgb;

    vec3 Kd0;

    if (object_id == 0) //TRACK
        Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
    else if (object_id == 1) //CAR
        Kd0 = texture(TextureImage1, vec2(U,V)).rgb;
    else if (object_id == 2) //WALL
        Kd0 = texture(TextureImage2, vec2(U,V)).rgb;
    else if (object_id == 3) //ARCS
        Kd0 = texture(TextureImage3, vec2(U,V)).rgb;
    else
        Kd0 = vec3(1.0, 1.0, 1.0); // fallback branco



    // Equação de Iluminação
    float lambert = max(0,dot(n,l));

    color.rgb = Kd0 * (lambert + 0.01);


    

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
