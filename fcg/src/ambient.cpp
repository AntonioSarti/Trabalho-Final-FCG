#include "ambient.hpp"

#include <unordered_map>
#include <iostream>
#include <vector>
#include <cmath>
#include <set>


float GetRandomFloat(float min, float max) {
    return min + (max - min) * (rand() / (double) RAND_MAX);
}

std::pair<float, float> GetPointInAnnulus(std::pair<float, float> &center, float r) {
    float angle = GetRandomFloat(0.0f, 2 * M_PI); 

    // Generate a random radius between r and 2r (with correct distribution)
    float radius = sqrt(GetRandomFloat(r * r, (2 * r) * (2 * r)));

    // Convert polar coordinates to Cartesian coordinates
    double x = center.first + radius * cos(angle);
    double y = center.second + radius * sin(angle);

    return std::make_pair(x, y);
}

std::vector<std::vector<int>> MakeGrid(int m, int n) {
    std::vector<std::vector<int>> grid;

    for (int i = 0; i < m; i++) {
        grid.push_back(std::vector<int>());
        for (int j = 0; j < n; j++) {
            grid[i].push_back(-1);
        }
    }

    return grid;
}

bool HasPointTooClose(
        std::pair<float, float> &new_point,
        std::pair<int, int> &new_point_pos,
        std::vector<std::vector<int>> &grid,
        std::unordered_map<int, std::pair<float, float>> &coord_map,
        float r) {
    for (int i = new_point_pos.first-1; i <= new_point_pos.first+1; i++) {
        for (int j = new_point_pos.second-1; j <= new_point_pos.second+1; j++) {
            if (i >= 0 && i < grid.size() && j >= 0 && j < grid.size()) {
                int coord_id = grid[i][j];
                if (coord_id != -1) {
                    auto coord = coord_map[coord_id];
                    float dist = (float) sqrt(pow(new_point_pos.first - coord.first, 2) + pow(new_point_pos.second - coord.second, 2)); 
                    if (dist > r) return true;
                }
            }
        }
    }

    return false;
}

std::list<std::pair<float, float>> MakePointList(
        std::unordered_map<int, std::pair<float, float>> &coord_map,
        float dimension_length) {
    std::list<std::pair<float, float>> point_list;

    // Iteração sobre cada ponto no mapa de coordenadas
    for (const auto& point : coord_map) {
        // A coordenada do ponto
        auto coord = point.second;
        
        // Ajuste para centralizar o espaço em torno de (0, 0)
        point_list.emplace_back(coord.first - dimension_length / 2, coord.second - dimension_length / 2);
    }

    return point_list;
}

std::list<std::pair<float, float>> CalculatePoisson(float side, float r, int k) {
    int current_index = 0, attempt;
    std::set<int> active_points;
    std::unordered_map<int, std::pair<float, float>> coord_map;
    std::vector<std::vector<int>> point_grid; 
    
    // Determinação do tamanho das células e do número de células por dimensão
    float cell_size = r / sqrt(2);
    int cells_per_dimension = static_cast<int>(floor(side / cell_size));

    // Criação da grade de pontos
    point_grid = MakeGrid(cells_per_dimension, cells_per_dimension);

    float dimension_length = cell_size * cells_per_dimension;

    // Geração do primeiro ponto aleatório
    float first_i = GetRandomFloat(0, dimension_length);
    float first_j = GetRandomFloat(0, dimension_length);

    active_points.insert(current_index);
    coord_map[current_index] = std::pair<float, float>(first_i, first_j);
    point_grid[static_cast<int>(floor(first_i/cell_size))][static_cast<int>(floor(first_j/cell_size))] = current_index;
    current_index++;

    // Processamento de pontos ativos
    while (!active_points.empty()) {
        // Seleciona um ponto ativo aleatoriamente
        auto it = coord_map.begin();
        std::advance(it, rand() % coord_map.size());

        for (attempt = 0; attempt < k; attempt++) {
            auto coords = it->second;
            auto new_point = GetPointInAnnulus(coords, r);

            int grid_x = static_cast<int>(floor(new_point.first / cell_size));
            int grid_y = static_cast<int>(floor(new_point.second / cell_size));
            std::pair<int, int> grid_pos(grid_x, grid_y);

            if (grid_x >= 0 && grid_x < point_grid.size() && grid_y >= 0 && grid_y < point_grid.size()) {
                if (!HasPointTooClose(new_point, grid_pos, point_grid, coord_map, r)) {
                    active_points.insert(current_index);
                    coord_map[current_index] = new_point;
                    point_grid[static_cast<int>(floor(new_point.first / cell_size))][static_cast<int>(floor(new_point.second / cell_size))] = current_index;
                    current_index++;
                    break;
                }
            }
        }

        // Se nenhuma tentativa foi bem-sucedida, remove o ponto da lista ativa
        if (attempt == k) {
            active_points.erase(it->first);
        }
    }

    return MakePointList(coord_map, dimension_length);
}

void RemoveFromCentralRadius(std::list<std::pair<float, float>> &points_list, float radius) {
    // Calcula o quadrado do raio para comparação mais eficiente
    float radius_squared = radius * radius;

    // Usamos a função remove_if para remover os pontos dentro do raio
    points_list.remove_if([radius_squared](const std::pair<float, float>& point) {
        // Calcula o quadrado da distância até a origem
        float dist_squared = point.first * point.first + point.second * point.second;
        // Retorna verdadeiro se o ponto estiver dentro do raio, removendo-o da lista
        return dist_squared <= radius_squared;
    });
}
