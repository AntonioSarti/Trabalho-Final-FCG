#pragma once

#include <list>

std::list<std::pair<float, float>> CalculatePoisson(float side, float r, int k);
void RemoveFromCentralRadius(std::list<std::pair<float, float>> &points_list, float radius);

