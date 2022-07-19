#ifndef POINTPLOT_H
#define POINTPLOT_H

#include "plot.h"
#include "plotty.h"

#include "datasource.h"
#include "scattercore.h"

class PointPlot : public Plot {

protected:
    using SpecType = SpecificTable<float, // position
                                   float,
                                   float,
                                   float, // colors
                                   float,
                                   float,
                                   float, // scales
                                   float,
                                   float,
                                   QString // anno
                                   >;

    DataSource<SpecType> m_data_source;

    ScatterCore m_scatter_instances;

    void rebuild_instances();

    void select(SelectRegion const&);
    void select(SelectSphere const&);
    void select(SelectPlane const&);
    void select(SelectHull const&);

public:
    PointPlot(Plotty&                  host,
              int64_t                  id,
              std::span<float const>   px,
              std::span<float const>   py,
              std::span<float const>   pz,
              std::vector<glm::vec3>&& colors,
              std::vector<glm::vec3>&& scales,
              QStringList&&            strings);
    ~PointPlot() override;

    void domain_updated(Domain const&) override;

    void handle_selection(SpatialSelection const&) override;

    ProbeResult handle_probe(glm::vec3 const&) override;

private slots:
    void on_table_updated();
};
#endif // POINTPLOT_H
