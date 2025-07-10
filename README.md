# Trabalho Final de Fundamentos de Computação Gráfica (INF01047)
O Trabalho Final da disciplina consistiu no desenvolvimento de um jogo tridimensional de arrancada, intitulado Arrancadão 2025 – Desafio dos 400 metros, utilizando as linguagens C++ e a biblioteca gráfica OpenGL. O objetivo principal do jogo é simular uma disputa de arrancada entre dois veículos: um controlado pelo usuário por meio do teclado e outro conduzido por uma inteligência artificial com lógica de movimento simplificada. Vence o carro que alcançar primeiro a marca dos 400 metros. O projeto contempla a construção de uma pista linear em um ambiente 3D, incluindo elementos visuais como texturização, iluminação dinâmica e sistema de câmera ajustável. Também foram integrados recursos de interface gráfica para exibição de informações relevantes, como a contagem regressiva para a largada e a velocidade instantânea do veículo e para a escolha do nível de dificuldade. 

O desenvolvimento visou consolidar, por meio da prática, os conceitos teóricos abordados ao longo da disciplina, como o pipeline gráfico , modelagem geométrica 3D, transformações geométricas, modelos de iluminação, mapeamento de texturas, animação e detecção de colisão.
O projeto exigia diversos critérios técnicos que deveriam ser atingidos,com base nos assuntos abordados da disciplina. Eles estão listados a seguir junto com a descrição de onde foram aplicados:
- **Malhas poligonais complexas**: Se encontram em objetos como os carros e as pessoas, múltiplos modelos ".obj" complexos;
- **Transformações geométricas controladas pelo usuário**: A movimentação do carro e troca de câmeras são feitas pelo usuário através do teclado;
- **Câmera Livre e Look-At**: A Câmera Livre foi aplicada como uma visão do retrovisor do carro do jogador. A Câmera Look-At é a câmera padrão de terceira pessoa do jogo, além tem disso também desenvolvemos uma Câmera Lateral que se movimenta com uma curva Bézier cúbica; 
- **Instâncias de objetos**: Cones e paredes foram instanciados;
- **Três tipos de testes de intersecção**: Aplicamos quatro  tipos de intersecção, sendo elas:
 1. **AABB vs AABB**: entre o carro do jogador e as barreiras e cones; 
 2. **Esfera vs Esfera**: entre os carros; 
 3. **AABB vs Plano**: entre os carros e o chão;
 4. **AABB vs Ponto**: entre o carro e as bordas do mapa;
- **Animações baseadas no tempo**: A movimentação dos carros e das câmeras são baseadas no tempo,utilizando deltaTime;
- **Movimentação com curva Bézier cúbica**: A movimentação da câmera lateral utiliza curva Bézier cúbica;
- **Modelos de Iluminação Difusa** e **Blinn-Phong e Modelos de Interpolação de Phong e Gouraud**: os objetos do jogo utilizam os modelos de forma combinada alternadamente, mais precisamente:
1. **Pista**: Interpolação Gouraud e Iluminação Lambert;
2. **Carro**: Interpolação Phong e Iluminação Blinn-Phong;
3. **Barreira**: Interpolação Gouraud Iluminação Blinn-Phong;
4. **Arcos**:  Interpolação Phong e Iluminação Lambert;
- **Mapeamento de Texturas em todos os objetos**: Todos os objetos possuem mapeamento de textura via glUniform1i e TextureImage#;

Além dos requisitos básicos, foram desenvolvidos para o projeto uma lógica de física básica com gravidade, atrito, velocidade e aceleração. Uma IA simplificada com a lógica de movimento do adversário,um sistema de dificuldade baseado na aceleração e velocidade do adversário além da exibição na tela de uma lógica de contagem regressiva, um velocímetro simplificado, a escolha do nível de dificuldade e textos ao finalizar a corrida (“you win”/”you lose”).

Em relação as questões do desenvolvimento, a maior dificuldade encontrada pela dupla foi em relação ao desenvolvimento das colisões, mais especificamente da colisão cubo-cubo (AABB vs AABB), não necessariamente pela dificuldade do conceito, mas pela dificuldade de fazer o tratamento de forma correta, que demorou muito tempo pra ser desenvolvido e só foi resolvido de fato após inspiração de um grupo que apresentou um jogo semelhante na segunda-feira (a idéia que nos inspiramos é que o carro deveria ir para trás com uma força maior do que somente a distância que ele percorreu em direção ao obstáculo, uma espécie de "empurrão" na direção contrária). As outras colisões foram desenvolvidas de uma forma mais tranquila, assim como os outros requisitos técnicos do projeto, exceto os modelos de interpolação e iluminação, que foram mais trabalhosos que os demais, mas nada comparado ao desenvolvimento das colisões.  

## Como compilar o projeto com o CMake (Windows)
1. Abra um terminal (Prompt de comando);
2. No terminal, vá para o diretório onde o repositório foi clonado (`git clone`);
   
      E digite:
      ```
      cmake -G "MinGW Makefiles" -B build
      ```
      ```
      cd build
      ```
      ```
      run.exe
      ```
# Integrantes
- Antonio Carlos G. Sarti 
- Leandro Reis Boniatti
# Manual do Jogo
Ao abrir o jogo, você deve escolher entre um dos níveis de dificuldade:
- **Nível Fácil** (Easy): Tecla ‘1’
- **Nível Médio** (Medium): Tecla ‘2’
- **Nível Difícil** (Hard): Tecla ‘3’
A movimentação do carro acontece com **‘W’,’A’,’S’,’D’**.
- **‘W’**: acelera o carro;
- **‘A’**: movimenta para a esquerda;
- **‘S’**: movimenta para a direita;
- **‘D’**: desacelera move o carro para trás;
As Câmeras Livre e Look-At são alternadas com a tecla **F5**, a Câmera Lateral é acessível com a tecla **ESPAÇO**;

# Divisão de Tarefas

Inicialmente, a dupla cogitou a divisão de tarefas do requisitos técnicos por número de requisitos, divididos igualitariamente, no entanto, o desenvolvimento das colisões foi tão mais difícil do que o imaginado que a divisão acabou ficando majoritariamente entre desenvolvimento das colisões para um membro da dupla (Leandro)  e “o resto” (Antonio), apesar de que ambos os membros contribuíram um pouco na parte do outro também. 
Leandro desenvolveu 3 dos testes de colisão (AABB vs Plano,Esfera vs Esfera e AABB vs AABB) e Antonio desenvolveu o teste de colisão AABB vs Ponto.
Os outros requisitos foram desenvolvidos por Antonio, mas Leandro contribuiu em parte com modificações na parte da lógica física do jogo (muito por necessidade, para fazer os testes de colisão) e inicialmente na parte de importações de objetos (que foi demonstrada na apresentação parcial do projeto). 
Em relação a outras tarefas relacionadas ao envio do projeto, Leandro ficou responsável pelo relatório e Antonio pelo vídeo.
# Utilização de Inteligência Artificial

Um dos membros da dupla (Antonio) optou por praticamente não utilizar ferramentas de inteligência artificial após testes com o Gemini, por achar que as implementações tinham um nível muito baixo.
Para o desenvolvimento das colisões, o ChatGPT foi uma ferramenta importante. Leandro utilizou-o inicialmente para acelerar o entendimento do código, que já havia sido iniciado por Antonio. Também utilizou-o para debugs (economiza muito tempo colocar os erros de compilação nele, pois ele vai indicar rapidamente o que está errado, ajuda muito em casos clássicos como faltar um ‘{‘ ou ‘}’ em algum lugar do código)  e ideias de implementação (por exemplo, a sugestão de fazer uma função que desenha as bounding boxes para entender os erros que estavam acontecendo), o que se provou ser muito útil. No geral, ele não conseguiu resolver problemas complexos (como desenvolver uma função que faz o teste de colisão AABB vs AABB perfeitamente) mas a utilização da ferramenta foi satisfatória. 

# Imagens de demonstração

<img width="617" height="468" alt="Image" src="https://github.com/user-attachments/assets/8740319d-64a9-498c-849f-181b057ec5f8" />
Tela inicial do jogo

<img width="608" height="448" alt="Image" src="https://github.com/user-attachments/assets/798eb940-233a-428f-b78a-57d8363e8724" />
Visão da câmera lateral



