#ifndef POINTPLOT_H
#define POINTPLOT_H

#include "plot.h"
#include "plotty.h"

#include "datasource.h"
#include "scattercore.h"

class PointPlot : public Plot {
    enum { PX, PY, PZ, CR, CG, CB, SX, SY, SZ };

protected:
    DataSource m_data_source;

    ScatterCore m_scatter_instances;

    void rebuild_instances();

    void select(SelectRegion const&);
    void select(SelectSphere const&);
    void select(SelectPlane const&);
    void select(SelectHull const&);

public:
    PointPlot(Plotty&                    host,
              int64_t                    id,
              std::span<double const>    px,
              std::span<double const>    py,
              std::span<double const>    pz,
              std::vector<glm::vec3>&&   colors,
              std::vector<glm::vec3>&&   scales,
              std::vector<std::string>&& strings);
    ~PointPlot() override;

    void domain_updated(Domain const&) override;

    void handle_selection(SpatialSelection const&) override;

    ProbeResult handle_probe(glm::vec3 const&) override;

private slots:
    void on_table_updated();
};
#endif // POINTPLOT_H
