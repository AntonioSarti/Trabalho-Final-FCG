# Trabalho Final de Fundamentos de Computação Gráfica (INF01047)
O Trabalho Final da disciplina consistiu no desenvolvimento de um jogo tridimensional de arrancada, intitulado ARRANCADÃO 2025 – Desafio dos 400 metros, utilizando as linguagens C++ e a biblioteca gráfica OpenGL. O objetivo principal do jogo é simular uma disputa de arrancada entre dois veículos: um controlado pelo usuário por meio do teclado e outro conduzido por uma inteligência artificial com lógica de movimento simplificada. Vence o carro que alcançar primeiro a marca dos 400 metros. O projeto contempla a construção de uma pista linear em um ambiente 3D, incluindo elementos visuais como texturização, iluminação dinâmica e sistema de câmera ajustável. Também foram integrados recursos de interface gráfica para exibição de informações relevantes, como a contagem regressiva para a largada e a velocidade instantânea do veículo. O desenvolvimento visa consolidar, por meio da prática, os conceitos teóricos abordados ao longo da disciplina.
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
# Tabela 1 - das atividades desenvolvidas/em desenvolvimento
| Critérios Técnicos | Concluído (Sim/Não) | Desenvolvidos por |
| :--- | :---: | :---: |
| Malhas poligonais complexas                         | Sim | Antonio |
| Transformações geométricas controladas pelo usuário | Sim | Antonio |
| Câmera livre e câmera look-at                       | Sim | Antonio |           
| Instâncias de objetos                               | Sim | Antonio |           
| Três tipos de testes de intersecção                 | Não | - |           
| Modelos de Iluminação Difusa e Blinn-Phong          | Sim | Antonio |   
| Modelos de Interpolação de Phong e Gouraud          | Sim | Antonio |
| Mapeamento de texturas em todos os objetos          | Sim | Antonio |
| Movimentação com curva Bézier cúbica                | Sim | Antonio |
| Animações baseadas no tempo ($\Delta t$)            | Sim | Antonio |
# Tabela 2 - Recursos empregados no desenvolvimento do game
| Implementação | Concluído (Sim/Não) | Desenvolvidos por |
| :--- | :---: | :---: |
| Física básica: gravidade, atrito, velocidade, aceleração           | Sim | Antonio |
| Lógica do movimento do adversário (IA)                             | Sim | Antonio |
| Velocímetro simplificado (em forma de texto) na tela               | Sim | Antonio |           
| Lógica de contagem regressiva (countdown) para o início da corrida | Sim | Antonio | 

