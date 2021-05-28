#include "utility.h"

std::pair<glm::vec3, glm::vec3> min_max_of(std::span<glm::vec3 const> v) {

    if (v.empty()) return { {}, {} };

    glm::vec3 lmin = v[0];
    glm::vec3 lmax = v[0];

    for (auto const& lv : v) {
        lmin = glm::min(lv, lmin);
        lmax = glm::max(lv, lmax);
    }

    return { lmin, lmax };
}


std::pair<glm::vec3, glm::vec3> min_max_of(std::span<double const> x,
                                           std::span<double const> y,
                                           std::span<double const> z) {
    if (x.empty() or y.empty() or z.empty()) return { {}, {} };

    glm::vec3 lmin(x[0], y[0], z[0]);
    glm::vec3 lmax(x[0], y[0], z[0]);

    for (size_t i = 0; i < x.size(); i++) {
        lmin = glm::min(glm::vec3 { x[i], y[i], z[i] }, lmin);
        lmax = glm::max(glm::vec3 { x[i], y[i], z[i] }, lmax);
    }

    return { lmin, lmax };
}
