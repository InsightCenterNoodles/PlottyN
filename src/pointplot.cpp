#include "pointplot.h"

#include "glyphs.h"
#include "utility.h"
#include "variant_tools.h"

#include <glm/gtx/intersect.hpp>
#include <glm/gtx/norm.hpp>

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
    up.definition =
        noo::ObjectRenderableDefinition { .material = m_mat,
                                          .mesh     = m_mesh,
                                          .instances =
                                              m_scatter_instances.instances() };

    noo::update_object(m_obj, up);

    auto* sd = m_host->domain();

    if (px.size() and sd->domain_auto_updates()) {
        auto [l, h] = min_max_of(px, py, pz);
        m_host->domain()->ask_update_input_bounds(l, h);
    }
}

const std::string_view brush_selection_name = "brushed";

template <class Function>
std::vector<int64_t> build_keys(DataSource&        source,
                                ScatterCore const& instances,
                                Function&&         function) {
    std::vector<int64_t> keys;

    auto const& map = source.table().get_row_to_key_map();

    auto const& inst = instances.instances();

    for (size_t i = 0; i < inst.size(); i++) {
        auto const& m = inst[i];
        auto        p = glm::vec3(m[0]);

        if (function(p)) { keys.push_back(map[i]); }
    }

    return keys;
}

void PointPlot::select(SelectRegion const& sel) {

    std::vector<int64_t> keys = build_keys(
        m_data_source, m_scatter_instances, [&sel](glm::vec3 const& p) {
            if (!glm::all(glm::greaterThanEqual(p, sel.min))) return false;
            if (!glm::all(glm::lessThanEqual(p, sel.max))) return false;
            return true;
        });

    m_data_source.table().modify_selection(
        brush_selection_name, keys, sel.select);
}

void PointPlot::select(SelectSphere const& sel) {

    auto radius_sq = sel.radius * sel.radius;

    auto test = [&sel, radius_sq](glm::vec3 const& p) {
        auto to_p = p - sel.point;
        auto d    = glm::dot(to_p, to_p);
        return radius_sq < d;
    };

    std::vector<int64_t> keys =
        build_keys(m_data_source, m_scatter_instances, test);

    m_data_source.table().modify_selection(
        brush_selection_name, keys, sel.select);
}

void PointPlot::select(SelectPlane const& sel) {
    auto n = glm::normalize(sel.normal);

    auto test = [&sel, n](glm::vec3 const& p) {
        auto to_p = glm::normalize(p - sel.point);
        auto d    = glm::dot(to_p, n);
        return d > 0;
    };

    std::vector<int64_t> keys =
        build_keys(m_data_source, m_scatter_instances, test);

    m_data_source.table().modify_selection(
        brush_selection_name, keys, sel.select);
}

static bool is_point_in(glm::vec3 const&           p,
                        std::span<glm::vec3 const> hull,
                        std::span<int64_t const>   index) {

    // pick a point

    glm::vec3 far_p = p + glm::vec3(0, 0, 1000000);

    auto dir = glm::normalize(far_p - p);

    // for each triangle, count intersections

    int isect_count = 0;

    for (size_t i = 0; i < index.size(); i += 3) {
        auto const& a = hull[i];
        auto const& b = hull[i + 1];
        auto const& c = hull[i + 2];

        // if all the points are 'behind' the point, we can skip any testing

        {
            auto da = glm::dot(dir, a - p);
            auto db = glm::dot(dir, b - p);
            auto dc = glm::dot(dir, c - p);

            if (da < 0 and db < 0 and dc < 0) continue;
        }

        glm::vec2 bary_coords;
        float     dist;
        isect_count +=
            glm::intersectRayTriangle(p, dir, a, b, c, bary_coords, dist);
    }


    // if even, outside, if odd, inside.
    return isect_count % 2 != 0;
}

void PointPlot::select(SelectHull const& sel) {
    auto test = [&sel](glm::vec3 const& p) {
        return is_point_in(p, sel.points, sel.index);
    };

    std::vector<int64_t> keys =
        build_keys(m_data_source, m_scatter_instances, test);

    m_data_source.table().modify_selection(
        brush_selection_name, keys, sel.select);
}

PointPlot::PointPlot(Plotty&                    host,
                     int64_t                    id,
                     std::span<double const>    px,
                     std::span<double const>    py,
                     std::span<double const>    pz,
                     std::vector<glm::vec3>&&   colors,
                     std::vector<glm::vec3>&&   scales,
                     std::vector<std::string>&& strings)
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

        if (strings.size()) {
            columns.emplace_back("annotation", std::move(strings));
        }

        auto tbl = std::make_shared<SimpleTable>(
            "Point Table " + std::to_string(id), std::move(columns));

        m_data_source = DataSource(m_doc, tbl);
    }

    std::string str = QString("Spheres %1").arg(m_plot_id).toStdString();

    auto [pmat, pmesh, pobj] = build_common_sphere(str, m_doc);

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


void PointPlot::domain_updated(Domain const&) {
    rebuild_instances();
}


void PointPlot::handle_selection(SpatialSelection const& sel) {
    std::visit([this](auto const& a) { this->select(a); }, sel);
}

Plot::ProbeResult PointPlot::handle_probe(glm::vec3 const& probe_point) {
    float const cutoff_dist = .15;

    auto const cutoff_dist_sq = cutoff_dist * cutoff_dist;

    auto const& map = m_data_source.table().get_row_to_key_map();

    auto const& inst = m_scatter_instances.instances();

    int64_t   best_index   = -1;
    float     best_dist_sq = cutoff_dist_sq; // others must be less than this...
    glm::vec3 best_point;

    for (size_t i = 0; i < inst.size(); i++) {
        auto const& m = inst[i];
        auto        p = glm::vec3(m[0]);

        auto dist_sq = glm::distance2(p, probe_point);

        if (best_dist_sq <= dist_sq) continue;

        best_index   = i;
        best_dist_sq = dist_sq;
        best_point   = p;
    }


    if (best_index < 0) return {};

    std::string text = "Key: " + std::to_string(map[best_index]);

    { // if there is an annotation field, use it.
        auto const& cols = m_data_source.table().get_columns();

        for (auto const& c : cols) {
            if (c.name != "annotation") continue;

            auto strings = c.as_string();

            if (strings.empty()) continue;

            auto anno_string = strings[best_index % strings.size()];

            text += ": " + anno_string;

            break;
        }
    }

    return {
        .text  = std::move(text),
        .place = best_point,
    };
}

void PointPlot::on_table_updated() {
    rebuild_instances();
}
