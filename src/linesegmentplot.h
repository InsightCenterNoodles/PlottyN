#ifndef LINESEGMENTPLOT_H
#define LINESEGMENTPLOT_H

#include "plot.h"
#include "plotty.h"

class LineSegmentPlot : public Plot {

    std::vector<glm::vec3> m_points;
    std::vector<glm::vec3> m_colors;
    std::vector<glm::vec2> m_scales;

    std::vector<glm::mat4> m_instances;
    void                   rebuild_instances();

public:
    LineSegmentPlot(Plotty&                  host,
                    int64_t                  id,
                    std::span<double const>  px,
                    std::span<double const>  py,
                    std::span<double const>  pz,
                    std::vector<glm::vec3>&& colors,
                    std::vector<glm::vec2>&& scales);

    ~LineSegmentPlot() override;

    void domain_updated(Domain const&) override;
};

#endif // LINESEGMENTPLOT_H
