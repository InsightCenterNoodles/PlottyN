#ifndef UTILITY_H
#define UTILITY_H


#include "noo_include_glm.h"

#include <span>

std::pair<glm::vec3, glm::vec3> min_max_of(std::span<glm::vec3 const>);

std::pair<glm::vec3, glm::vec3> min_max_of(std::span<double const> x,
                                           std::span<double const> y,
                                           std::span<double const> z);


#endif // UTILITY_H
