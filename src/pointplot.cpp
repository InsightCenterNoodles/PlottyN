#include "pointplot.h"

#include "glyphs.h"
#include "utility.h"
#include "variant_tools.h"

#include <glm/gtx/intersect.hpp>
#include <glm/gtx/norm.hpp>

#include <QDebug>

enum { PX, PY, PZ, CR, CG, CB, SX, SY, SZ, ANNO };

void PointPlot::rebuild_instances() {
    auto d = m_host->domain()->current_domain();

    auto px = m_data_source.column<PX>();
    auto py = m_data_source.column<PY>();
    auto pz = m_data_source.column<PZ>();

    m_scatter_instances.build_instances(
        {
            .px = px,
            .py = py,
            .pz = pz,

            .cr = m_data_source.column<CR>(),
            .cg = m_data_source.column<CG>(),
            .cb = m_data_source.column<CB>(),

            .sx = m_data_source.column<SX>(),
            .sy = m_data_source.column<SY>(),
            .sz = m_data_source.column<SZ>(),
        },
        d);

    update_instances(
        m_scatter_instances.instances(), m_host->document(), m_obj, m_mesh);

    auto* sd = m_host->domain();

    if (px.size() and sd->domain_auto_updates()) {
        auto [l, h] = min_max_of(px, py, pz);
        m_host->domain()->ask_update_input_bounds(l, h);
    }
}

const QString brush_selection_name = "brushed";

template <class DS, class Function>
std::vector<int64_t> build_select_keys(DS const& source, Function&& function) {
    std::vector<int64_t> keys;

    auto const& t = source.table();

    auto all_keys = t.get_all_keys();

    // auto const& map = source.table().get_row_to_key_map();

    // auto const& inst = instances.instances();

    for (int key : all_keys) {


        auto p = glm::vec3(t.template get_column_at_key<PX>(key),
                           t.template get_column_at_key<PY>(key),
                           t.template get_column_at_key<PZ>(key));

        if (function(p)) { keys.push_back(key); }
    }

    return keys;
}

void PointPlot::select(SelectRegion const& sel) {

    std::vector<int64_t> keys =
        build_select_keys(m_data_source, [&sel](glm::vec3 const& p) {
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

    std::vector<int64_t> keys = build_select_keys(m_data_source, test);

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

    std::vector<int64_t> keys = build_select_keys(m_data_source, test);

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

    std::vector<int64_t> keys = build_select_keys(m_data_source, test);

    m_data_source.table().modify_selection(
        brush_selection_name, keys, sel.select);
}

PointPlot::PointPlot(Plotty&                  host,
                     int64_t                  id,
                     std::span<float const>   px,
                     std::span<float const>   py,
                     std::span<float const>   pz,
                     std::vector<glm::vec3>&& colors,
                     std::vector<glm::vec3>&& scales,
                     QStringList&&            strings)
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

        auto tbl = std::make_shared<SpecType>(QString("Point Table %1").arg(id),
                                              std::move(columns));

        m_data_source = DataSource(m_doc, tbl);
    }

    auto str = QString("Spheres %1").arg(m_plot_id);

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
            &SpecType::table_row_deleted,
            this,
            &PointPlot::on_table_updated);

    connect(&m_data_source.table(),
            &SpecType::table_row_updated,
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

    auto const& t = m_data_source.table();

    auto all_keys = t.get_all_keys();

    int64_t   best_key     = -1;
    float     best_dist_sq = cutoff_dist_sq; // others must be less than this...
    glm::vec3 best_point;

    for (size_t i = 0; i < all_keys.size(); i++) {
        auto key = all_keys[i];

        auto p = glm::vec3(t.template get_column_at_key<PX>(key),
                           t.template get_column_at_key<PY>(key),
                           t.template get_column_at_key<PZ>(key));

        auto dist_sq = glm::distance2(p, probe_point);

        if (best_dist_sq <= dist_sq) continue;

        best_key     = i;
        best_dist_sq = dist_sq;
        best_point   = p;
    }


    if (best_key < 0) return {};

    QString text = QString("Key: %1").arg(best_key);

    QString anno = t.template get_column_at_key<ANNO>(best_key);

    if (anno.isEmpty()) { // if there is an annotation field, use it.
        text += ": " + anno;
    }

    return {
        .text  = std::move(text),
        .place = best_point,
    };
}

void PointPlot::on_table_updated() {
    rebuild_instances();
}
