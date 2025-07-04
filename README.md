# Trabalho Final de Fundamentos de Computação Gráfica (INF01047)
O Trabalho Final da disciplina será desenvolver um jogo 3D de corrida de carros utilizando C++ e OpenGL. O projeto incluirá a criação de uma pista tridimensional, veículos controláveis, cenário com obstáculos e elementos visuais como texturas, iluminação e câmera dinâmica. O jogador poderá controlar um carro em tempo real, com sistema de voltas e cronometragem. A aplicação servirá para demonstrar na prática os conceitos estudados em aula.
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
# Tabela das atividades desenvolvidas/em desenvolvimento
| Critérios Técnicos | Concluído (Sim/Não) | Desenvolvidos por |
| :--- | :---: | :---: |
| Malhas poligonais complexas                         | Sim | Antonio |
| Transformações geométricas controladas pelo usuário | Sim | Antonio |
| Câmera livre e câmera look-at                       | Sim | Antonio |           
| Instâncias de objetos                               | Sim | Antonio |           
| Três tipos de testes de intersecção                 | Não | - |           
| Modelos de Iluminação Difusa e Blinn-Phong          | Não | - |   
| Modelos de Interpolação de Phong e Gouraud          | Não | - |
| Mapeamento de texturas em todos os objetos          | Sim | Antonio |
| Movimentação com curva Bézier cúbica                | Sim | Antonio |
| Animações baseadas no tempo ($\Delta t$)            | Sim | Antonio |
