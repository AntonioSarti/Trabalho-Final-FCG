
# Trabalho Final de FCG

O trabalho desenvolvido na disciplina Fundamentos de Computação Gráfica (FCG) 
- INF01047, da UFRGS, consiste na implementação de uma fase de um jogo inspirado 
no gênero *Zombie Survival Game*. O jogo apresenta mecânicas intuitivas de 
movimentação e combate, onde o jogador enfrenta o *Zombie Boss*. O principal 
objetivo é sobreviver aos ataques constantes do inimigo, revidando de forma 
estratégica para infligir danos suficientes e, assim, derrotá-lo.


## Compilação do código fonte

Utilizamos para desenvolver o trabalho final o `CMake` e o `MinGW` como 
sistema de compilação, além das bibliotecas `OpenGL` e da linguagem `C++`. 
Para compilar e executar o projeto (no Windows):

1. Clone o repositório localmente em seu computador; 
2. Na pasta onde o repositório foi clonado, execute o comando via terminal: 
   `> cmake -G "MinGW Makefiles" -B build`, para gerar a nova pasta `build` 
   e suas dependências;
3. Em seguida, vá para a pasta criada, através do comando: `> cd build`; 
4. Então, execute o comando `> make` para gerar os arquivos de build;
5. Após a criação do programa executável, basta rodá-lo: `> fcg.exe`


## Como jogar

O personagem do jogo é um guerreiro habilidoso, armado com uma lança afiada 
e protegido por uma armadura leve, cuja jornada se torna uma luta pela sobrevivência 
ao enfrentar um chefe temível, o *Zombie Boss*, que fará de tudo para derrotá-lo. 
Para navegar pelo ambiente hostil, o jogador utiliza as teclas *"W", "A", "S" e "D"* 
para movimentação, enquanto a tecla *Left Shift* permite ao guerreiro correr. Os ataques 
com a lança são executados através do botão esquerdo do mouse, enquanto a tecla F5 
possibilita alternar entre as visões em primeira e terceira pessoa. O F4 é utilizado 
para encerrar o jogo. Tanto o Boss, quanto o jogador partem com 100 HP de vida e quem 
conseguir resistir aos ataques e diminuir a vida do seu oponente primeiro, ganha a fase!!!


## Relatório

Abaixo, descrevemos um breve relatório, apresentando os principais critérios técnicos 
empregados no desenvolvimento do trabalho final. 


### Malhas poligonais complexas

No desenvolvimento do projeto, utilizamos três modelos 3D principais, todos compostos 
por malhas poligonais complexas: o Guerreiro (personagem jogável), o Zombie Boss 
(inimigo) e as árvores que integraram o cenário da batalha.


### Transformações geométricas pelo usuário, câmera livre e câmera look-at 

As interações do usuário com o teclado e o mouse foram fundamentais para o 
controle do personagem no jogo, permitindo a movimentação estratégica pelo mapa 
e a realização tanto de ataques ao adversário, quanto de esquivas. Para aprimorar 
a experiência de imersão, implementou-se uma câmera livre em primeira pessoa, 
proporcionando uma visão direta e detalhada do ambiente, e uma câmera look-at 
em terceira pessoa, que permite uma visão panorâmica do cenário e do personagem.
A alternância entre essas duas perspectivas por meio da tecla F5 oferece ao jogador 
a flexibilidade de escolher a visão mais adequada à situação de combate no jogo. 
Essa implementação visa aumentar o dinamismo e a interação, proporcionando uma 
experiência visual mais envolvente e adaptável.


### Instâncias de objetos

Foram utilizadas instâncias para representar os personagens e elementos do cenário. 
O modelo do personagem principal, o guerreiro, e o modelo do inimigo, o Zombie Boss, 
são instanciados em duas ocorrências distintas, cada uma com suas características 
específicas. Além disso, as árvores que compõem o cenário foram modeladas a partir 
de um único modelo base, sendo replicadas ao longo do mapa. A fim de garantir uma 
distribuição natural das árvores no ambiente, utilizamos o algoritmo de geração 
procedural *"Fast Poisson Disk Sampling"* (<https://dl.acm.org/doi/pdf/10.1145/7529.8927>). 
Este algoritmo, adaptado ao presente trabalho, permite o posicionamento dos modelos 
das árvores de maneira não uniforme, evitando aglomerações excessivas e proporcionando 
uma distribuição mais orgânica e visualmente agradável.


###  Testes de intersecção utilizados 

Foi utilizado o teste de intersecção *AABB-plano* para detectar a colisão do adversário 
com o chão, a partir do momento em que o mesmo surge na cena do game. Para verificar se 
o ataque do jogador atingiu o adversário, emprego-se o teste *raio-AABB*, com o raio partindo 
do ponto central da câmera e se estendendo na direção do vetor de visão.Além disso, foram 
aplicados testes de intersecção *ponto-AABB* e *AABB-AABB* para restringir o movimento do jogador 
dentro do mapa e para garantir as colisões com as árvores e com o adversário.


### Modelos de iluminação Difusa e Blinn-Phong

Em relação aos modelos de iluminação, o plano do chão e as árvores foram iluminados utilizando o 
modelo difuso, que oferece uma distribuição uniforme da luz sobre as superfícies, criando 
uma aparência mais natural para objetos que não apresentam brilho. Já os personagens foram 
iluminados com o modelo Blinn-Phong, que adiciona um efeito de brilho especular, proporcionando 
um acabamento mais detalhado e realista para superfícies refletivas, como as de personagens em 
movimento. No que diz respeito aos métodos de interpolação, o modelo de Phong foi empregado 
para o plano do chão e para os personagens, permitindo um sombreamento suave e uma transição 
mais precisa de luz nas superfícies desses elementos. Por outro lado, as árvores utilizaram o
modelo de Gouraud, que realiza a interpolação de luz nas vértices, oferecendo uma solução mais 
simples para elementos com menos complexidade de sombreamento.


### Mapeamento de texturas

Todos os objetos no cenário fazem uso de texturas, embora a interpolação *Gouraud* suavize o 
efeito visual da aplicação dessas texturas, tornando-as menos perceptíveis. O plano do chão foi 
o único elemento que exigiu a repetição da textura para cobrir sua área extensa, criando uma 
aparência uniforme e natural em toda a superfície.


### Movimentação com curva de Bézier cúbica

A animação de ataque do jogador foi aprimorada com o uso de uma curva de Bézier cúbica, 
permitindo que sua lança siga uma trajetória suave e controlada. Essa curva matemática 
foi empregada para gerar um movimento suave, garantindo que a lança se desloque de forma 
fluida, com controle sobre os pontos de início, fim e os dois pontos de controle intermediários,
na tentativa de proporcionar uma dinâmica mais realista à ação do combate.


### Animações baseadas no tempo

Os movimentos na aplicação foram calculados com base na diferença de *timestamp* 
entre a atualização atual e a anterior. Isso inclui a movimentação do jogador, 
a queda do adversário e a animação de ataque.


## Imagens

No início da fase, quando o jogador se afasta do centro do mapa, surge o *Zombie Boss*.  

![Início da fase](data/imagens/img01.jpeg)

Ao surgir o *Zombie Boss*, imediatamente começa a perseguir o jogador e a atacá-lo.  

![Run for your life](data/imagens/img02.jpeg)

Ao derrotar o chefe da fase, o mesmo aparece caido ao plano do chão.

![Combat](data/imagens/img03.jpeg)

Se o jogador sofrer tantos danos que sua vida atinja 0 HP, então ele será derrotado!

![Defeat](data/imagens/img04.jpeg)
