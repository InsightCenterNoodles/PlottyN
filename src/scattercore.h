#ifndef SCATTERCORE_H
#define SCATTERCORE_H

#include "plotty.h"

class ScatterCore {
    std::vector<glm::mat4> m_instances;

public:
    ScatterCore();

    void build_instances(std::span<glm::vec3> points,
                         std::span<glm::vec3> colors,
                         std::span<glm::vec3> scales,
                         Domain const&);

    struct ArrayRef {
        std::span<double const> px, py, pz;
        std::span<double const> cr, cg, cb;
        std::span<double const> sx, sy, sz;
    };

    void build_instances(ArrayRef const& ref, Domain const&);

    auto const& instances() const { return m_instances; }

    bool empty() const { return m_instances.empty(); }
};

#endif // SCATTERCORE_H
