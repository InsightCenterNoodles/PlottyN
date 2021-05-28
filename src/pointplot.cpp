#include "pointplot.h"

#include "glyphs.h"
#include "utility.h"

#include <QDebug>

void PointPlot::rebuild_instances() {
    auto d = m_host->domain()->current_domain();

    auto px = m_data_source.get_doubles_at(m_column_mapping[PX]);
    auto py = m_data_source.get_doubles_at(m_column_mapping[PY]);
    auto pz = m_data_source.get_doubles_at(m_column_mapping[PZ]);

    m_scatter_instances.build_instances(
        {
            .px = px,
            .py = py,
            .pz = pz,

            .cr = m_data_source.get_doubles_at(m_column_mapping[CR]),
            .cg = m_data_source.get_doubles_at(m_column_mapping[CG]),
            .cb = m_data_source.get_doubles_at(m_column_mapping[CB]),

            .sx = m_data_source.get_doubles_at(m_column_mapping[SX]),
            .sy = m_data_source.get_doubles_at(m_column_mapping[SY]),
            .sz = m_data_source.get_doubles_at(m_column_mapping[SZ]),
        },
        d);

    noo::ObjectUpdateData up;
    up.instances = m_scatter_instances.instances();

    noo::update_object(m_obj, up);

    auto* sd = m_host->domain();

    if (px.size() and sd->domain_auto_updates()) {
        auto [l, h] = min_max_of(px, py, pz);
        m_host->domain()->ask_update_input_bounds(l, h);
    }
}

PointPlot::PointPlot(Plotty&                  host,
                     int64_t                  id,
                     std::span<double const>  px,
                     std::span<double const>  py,
                     std::span<double const>  pz,
                     std::vector<glm::vec3>&& colors,
                     std::vector<glm::vec3>&& scales)
    : Plot(host, id) {

    {
        auto num_points = px.size();

        std::vector<LoadTableColumn> columns;

        columns.emplace_back("x", px);
        columns.emplace_back("y", py);
        columns.emplace_back("z", pz);

        m_column_mapping[PX] = 0;
        m_column_mapping[PY] = 1;
        m_column_mapping[PZ] = 2;

        int next_column = 3;

        {
            if (colors.empty()) { colors.push_back({ 1, 1, 1 }); }

            std::vector<double> r, g, b;

            r.resize(num_points);
            g.resize(num_points);
            b.resize(num_points);

            for (size_t i = 0; i < num_points; i++) {
                r[i] = colors[i % colors.size()].x;
                g[i] = colors[i % colors.size()].y;
                b[i] = colors[i % colors.size()].z;
            }

            columns.emplace_back("r", std::move(r));
            columns.emplace_back("g", std::move(g));
            columns.emplace_back("b", std::move(b));

            m_column_mapping[CR] = next_column++;
            m_column_mapping[CG] = next_column++;
            m_column_mapping[CB] = next_column++;
        }

        {
            if (scales.empty()) { scales.push_back({ .02, .02, .02 }); }

            // so we are duplicating the values here to make the table a bit
            // more sane for viewers
            std::vector<double> sx, sy, sz;

            sx.resize(num_points);
            sy.resize(num_points);
            sz.resize(num_points);

            for (size_t i = 0; i < num_points; i++) {
                sx[i] = scales[i % scales.size()].x;
                sy[i] = scales[i % scales.size()].y;
                sz[i] = scales[i % scales.size()].z;
            }

            columns.emplace_back("sx", std::move(sx));
            columns.emplace_back("sy", std::move(sy));
            columns.emplace_back("sz", std::move(sz));

            m_column_mapping[SX] = next_column++;
            m_column_mapping[SY] = next_column++;
            m_column_mapping[SZ] = next_column++;
        }

        auto tbl = std::make_shared<SimpleTable>(
            "Point Table " + std::to_string(id), std::move(columns));

        m_data_source = DataSource(m_doc, tbl);
    }

    auto [pmat, pmesh, pobj] = build_common_sphere(m_doc);

    m_mat  = pmat;
    m_mesh = pmesh;
    m_obj  = pobj;

    if (px.size()) {
        auto [l, h] = min_max_of(px, py, pz);
        host.domain()->ask_update_input_bounds(l, h);
    }

    rebuild_instances();

    connect(&m_data_source.table(),
            &SimpleTable::table_row_deleted,
            this,
            &PointPlot::on_table_updated);

    connect(&m_data_source.table(),
            &SimpleTable::table_row_updated,
            this,
            &PointPlot::on_table_updated);
}

PointPlot::~PointPlot() { }

// void PointPlot::append(std::span<glm::vec3> points,
//                       std::span<glm::vec3> colors,
//                       std::span<glm::vec3> scales) {

//    if (points.empty()) return;


//    {
//        auto [l, h] = min_max_of(points);
//        m_host->domain()->ask_update_input_bounds(l, h);
//    }

//    m_points.insert(m_points.end(), points.begin(), points.end());
//    m_colors.insert(m_colors.end(), colors.begin(), colors.end());
//    m_scales.insert(m_scales.end(), scales.begin(), scales.end());

//    rebuild_instances();
//}

// inline void replace(std::span<glm::vec3> source, std::vector<glm::vec3>&
// dest) {
//    dest.resize(source.size());

//    std::copy(source.begin(), source.end(), dest.begin());
//}

// void PointPlot::replace_with(std::span<glm::vec3> points,
//                             std::span<glm::vec3> colors,
//                             std::span<glm::vec3> scales) {

//    replace(points, m_points);
//    replace(colors, m_colors);
//    replace(scales, m_scales);

//    if (m_points.size()) {
//        auto [l, h] = min_max_of(m_points);
//        m_host->domain()->ask_update_input_bounds(l, h);
//    }

//    rebuild_instances();
//}

void PointPlot::domain_updated(Domain const&) {
    rebuild_instances();
}

void PointPlot::on_table_updated() {
    rebuild_instances();
}
