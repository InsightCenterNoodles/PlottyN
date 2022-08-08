#include "plotty.h"

#include "imageplot.h"
#include "linesegmentplot.h"
#include "plottyrootcallbacks.h"
#include "pointplot.h"
#include "simpletable.h"
#include "tableplot.h"

#include "variant_tools.h"

#include <glm/gtx/quaternion.hpp>

#include <string>
#include <string_view>

#include <QColor>
#include <QDebug>

using namespace std::literals;

struct FloatListArg {
    QVector<float> list;

    FloatListArg() = default;
    FloatListArg(QCborValue const& a) {
        for (auto v : a.toArray()) {
            list << v.toDouble();
        }
    }
};

struct ColorListArgument {
    std::vector<glm::vec3> colors;

    glm::vec3 decode_color(QCborValue v) {

        switch (v.type()) {
        case QCborValue::Type::String: {
            auto str = v.toString();

            auto c = QColor(str);

            return glm::vec3(c.redF(), c.greenF(), c.blueF());
        }
        case QCborValue::Type::Array: {
            auto this_arr = v.toArray();
            return glm::vec3(this_arr.at(0).toDouble(),
                             this_arr.at(1).toDouble(),
                             this_arr.at(2).toDouble());
        }
        case QCborValue::Type::Integer: {
            return glm::vec3(v.toInteger()) / 255.0f;
        }
        case QCborValue::Type::Double: {
            return glm::vec3(v.toDouble());
        }
        default: return glm::vec3(1);
        }
    }

    ColorListArgument() = default;
    ColorListArgument(QCborValue av) {

        if (!av.isArray()) {
            colors.push_back(decode_color(av));
            return;
        }

        auto local_arr = av.toArray();

        colors.reserve(local_arr.size());

        for (auto const& c : local_arr) {
            colors.push_back(decode_color(c));
        }
    }
};

struct Scale3DListArgument {
    std::vector<float> scales;

    Scale3DListArgument() = default;
    Scale3DListArgument(QCborValue av) {
        auto reals = noo::coerce_to_real_list(av);

        // we will let the users of this figure out how to handle this list
        if (reals.size()) {
            scales = std::vector<float>(reals.begin(), reals.end());
            return;
        }
    }

    std::vector<glm::vec3> make_scale_vector(std::span<float> source,
                                             size_t           num_points) {

        if (source.empty()) return {};

        std::vector<glm::vec3> ret;

        ret.resize(num_points);

        if (num_points == source.size() / 3) {
            for (size_t i = 0; i < source.size(); i += 3) {
                ret[i] = { source[i + 0], source[i + 1], source[i + 2] };
            }
        } else {

            for (size_t i = 0; i < source.size(); i++) {
                ret[i] = glm::vec3(source[i]);
            }
        }

        return ret;
    }
};

struct Scale2DListArgument {
    std::vector<glm::vec2> scales;

    Scale2DListArgument() = default;
    Scale2DListArgument(QCborValue av) {
        auto reals     = noo::coerce_to_real_list(av);
        auto real_span = std::span(reals);

        if (reals.size()) {

            scales.resize(reals.size() / 2);

            for (size_t i = 0; i < scales.size(); i += 2) {
                scales[i] = { real_span[i + 0], real_span[i + 1] };
            }
            return;
        }
    }
};


// Domain ======================================================================

glm::vec3 Domain::transform(glm::vec3 a) const {
    return lerp(a, input_min, input_max, output_min, output_max);
}

SharedDomain::SharedDomain(QObject* p) : QObject(p) { }

Domain const& SharedDomain::current_domain() const {
    return m_current_domain;
}
bool SharedDomain::domain_auto_updates() const {
    return m_domain_auto;
}

void SharedDomain::set_domain_auto_updates(bool b) {
    m_domain_auto = b;
}

void SharedDomain::ask_set_domain(Domain d) {
    if (!m_domain_auto) return;
    m_current_domain = d;
    emit domain_updated();
}

void SharedDomain::ask_update_input_bounds(glm::vec3 l, glm::vec3 h) {
    if (!m_domain_auto) return;

    bool l_ok = glm::distance(m_current_domain.input_min, l) <=
                std::numeric_limits<float>::epsilon();
    bool h_ok = glm::distance(m_current_domain.input_max, h) <=
                std::numeric_limits<float>::epsilon();

    if (l_ok and h_ok) return;

    qDebug() << "Bounds updated" << l.x << l.y << l.z << h.x << h.y << h.z;

    m_current_domain.input_min = l;
    m_current_domain.input_max = h;

    emit domain_updated();
}

void SharedDomain::set_axis_labels(QString x_axis_title,
                                   QString y_axis_title,
                                   QString z_axis_title) {
    m_x_axis_title = std::move(x_axis_title);
    m_y_axis_title = std::move(y_axis_title);
    m_z_axis_title = std::move(z_axis_title);
    emit labels_updated();
}

QString SharedDomain::x_axis_title() const {
    return m_x_axis_title;
}
QString SharedDomain::y_axis_title() const {
    return m_y_axis_title;
}
QString SharedDomain::z_axis_title() const {
    return m_z_axis_title;
}

// =============================================================================

void Plotty::make_box() {

    auto d = m_shared_domain->current_domain();

    auto select = [d](float x, float y, float z) {
        return glm::mix(d.output_min, d.output_max, glm::vec3(x, y, z));
    };

    std::array<glm::vec3, 8> const box_p = {
        select(0, 0, 0), select(0, 0, 1), select(0, 1, 0), select(0, 1, 1),
        select(1, 0, 0), select(1, 0, 1), select(1, 1, 0), select(1, 1, 1),
    };

    std::array<glm::u8vec4, 8> box_c;
    box_c.fill({ 255, 255, 255, 255 });

    static std::vector<glm::u16vec2> const box_i = {
        { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 }, { 0, 2 }, { 1, 3 },
        { 4, 6 }, { 5, 7 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
    };

    if (!m_box_mat) {
        noo::MaterialData md;

        md.pbr_info.base_color = Qt::white;

        m_box_mat = noo::create_material(m_doc, md);
    }


    if (!m_box_mesh) {
        noo::MeshSource md;

        md.material     = m_box_mat;
        md.positions    = box_p;
        md.colors       = box_c;
        md.indices      = std::as_bytes(std::span(box_i));
        md.index_format = noo::Format::U16;
        md.type         = noo::MeshSource::LINE;

        m_box_mesh = noo::create_mesh(m_doc, md);
    }


    noo::ObjectData nd;
    nd.parent     = plot_root();
    nd.definition = noo::ObjectRenderableDefinition { .mesh = m_box_mesh };
    nd.tags       = QStringList() << noo::names::tag_user_hidden;


    m_box = noo::create_object(m_doc, nd);
}

void Plotty::rebuild_axis() {

    m_axis_x = nullptr;
    m_axis_y = nullptr;
    m_axis_z = nullptr;


    auto make_axis =
        [&](QString const& t, glm::vec3 pos, glm::vec3 orientation) {
            auto rot =
                glm::rotation(glm::vec3(1, 0, 0), glm::normalize(orientation));

            auto mat = glm::mat4_cast(rot);

            auto tf = glm::translate(glm::mat4(1), pos) * mat;

            noo::ObjectTextDefinition otd;
            otd.font   = "Arial";
            otd.text   = t;
            otd.height = .05;

            auto name = "Axis " + t;

            noo::ObjectData nd;
            nd.name       = name;
            nd.parent     = plot_root();
            nd.transform  = tf;
            nd.definition = otd;

            return noo::create_object(m_doc, nd);
        };

    m_axis_x =
        make_axis(m_shared_domain->x_axis_title(), { .5, 0, 0 }, { 0, 1, 1 });
    m_axis_y =
        make_axis(m_shared_domain->y_axis_title(), { 0, .5, 0 }, { 1, 0, 1 });
    m_axis_z =
        make_axis(m_shared_domain->z_axis_title(), { 0, 0, .5 }, { 1, 1, 0 });
}

// Set domain ==================================================================

struct ForceToGLMVec3 {
    glm::vec3 v;

    ForceToGLMVec3() = default;
    ForceToGLMVec3(QCborValue rv) {
        auto reals = noo::coerce_to_real_list(rv);

        v.x = reals.value(0);
        v.y = reals.value(1);
        v.z = reals.value(2);
    }
};

auto make_set_domain_method(Plotty& p) {
    noo::MethodData m;
    m.method_name            = "set_domain";
    m.documentation          = "Set table bounds";
    m.argument_documentation = {
        { "input_min", "3-tuple input bb min", noo::names::hint_reallist },
        { "input_max", "3-tuple input bb max", noo::names::hint_reallist },
        { "output_min", "3-tuple output bb min", noo::names::hint_reallist },
        { "output_max", "3-tuple output bb max", noo::names::hint_reallist },
        { "axis_names", "3-tuple strings for axis names", "[text]" },
    };
    m.return_documentation = "An integer plot id";

    m.set_code([&p](noo::MethodContext const&,
                    ForceToGLMVec3     input_min,
                    ForceToGLMVec3     input_max,
                    ForceToGLMVec3     output_min,
                    ForceToGLMVec3     output_max,
                    noo::StringListArg strings) {
        // auto clist = args.steal_vector();

        std::vector<std::string> axis_names;

        Domain d;
        d.input_min  = input_min.v;
        d.input_max  = input_max.v;
        d.output_min = output_min.v;
        d.output_max = output_max.v;

        p.domain()->set_domain_auto_updates(false);
        p.domain()->ask_set_domain(d);
        p.domain()->set_axis_labels(strings.list.value(0),
                                    strings.list.value(1),
                                    strings.list.value(2));

        return QCborValue {};
    });

    return noo::create_method(p.document().get(), m);
}

// Add points ==================================================================

auto make_new_point_plot_method(Plotty& p) {


    noo::MethodData m;
    m.method_name            = "new_point_plot";
    m.documentation          = "Create a new point plot";
    m.argument_documentation = {
        { "xvals", "A list of point x values.", noo::names::hint_reallist },
        { "yvals", "A list of point y values.", noo::names::hint_reallist },
        { "zvals", "A list of point z values.", noo::names::hint_reallist },
        { "colors",
          "An optional list of colors. Can be a 1D array of 3-stride floats "
          "for RGB, or a list of hex strings. Can be null to skip",
          "[string] | reallist" },
        { "scales",
          "An optional list of 3D scales, laid out in a 1D array. Can be null "
          "to skip.",
          "reallist" },
        { "annos",
          "An optional list of string annotations. Must be the same length as "
          "the list of x values.",
          "[string]" },
    };
    m.return_documentation = "An integer plot id";


    m.set_code([&p](noo::MethodContext const&,
                    FloatListArg        xs,
                    FloatListArg        ys,
                    FloatListArg        zs,
                    ColorListArgument   cols,
                    Scale3DListArgument scales,
                    noo::StringListArg  strings) -> QCborValue {
        if (xs.list.size() != ys.list.size() or
            xs.list.size() != zs.list.size()) {
            throw noo::MethodException(
                noo::ErrorCodes::INVALID_PARAMS,
                "Coordinate arrays must be the same length");
        }

        return p.append<PointPlot>(
            -1,
            std::span(xs.list),
            std::span(ys.list),
            std::span(zs.list),
            std::move(cols.colors),
            scales.make_scale_vector(scales.scales, xs.list.size()),
            std::move(strings.list));
    });

    return noo::create_method(p.document().get(), m);
}

// Add lines ===================================================================

auto make_new_line_segment_plot_method(Plotty& p) {
    noo::MethodData m;
    m.method_name = "new_line_segment_plot";
    m.documentation =
        "Create a new line segment plot. Points given are connected in pairs "
        "to create disconnected lines: a <-> b,  c <-> d";
    m.argument_documentation = {
        { "xvals", "A list of point x values.", "reallist" },
        { "yvals", "A list of point y values.", "reallist" },
        { "zvals", "A list of point z values.", "reallist" },
        { "colors",
          "A list of colors, one color for each point. Can be a 1D array of "
          "3-tuple floats for RGB, or a list of hex strings",
          "[string] | reallist" },
        { "scales", "A list of 2D scales, laid out in a 1D array.", "reallist" }
    };
    m.return_documentation = "An integer plot id";

    m.set_code([&p](noo::MethodContext const&,
                    noo::RealListArg    xs,
                    noo::RealListArg    ys,
                    noo::RealListArg    zs,
                    ColorListArgument   cols,
                    Scale2DListArgument scales) -> QCborValue {
        if (xs.list.size() != ys.list.size() or
            xs.list.size() != zs.list.size()) {
            throw noo::MethodException(
                noo::ErrorCodes::INVALID_PARAMS,
                "Coordinate arrays must be the same length");
        }

        return p.append<LineSegmentPlot>(-1,
                                         std::span(xs.list),
                                         std::span(ys.list),
                                         std::span(zs.list),
                                         std::move(cols.colors),
                                         std::move(scales.scales));
    });

    return noo::create_method(p.document().get(), m);
}

// Add image ===================================================================

auto make_new_image_plot_method(Plotty& p) {
    noo::MethodData m;
    m.method_name            = "new_image_plot";
    m.documentation          = "Create a new image plane";
    m.argument_documentation = {
        { "image", "Bytes of the on-disk image.", "data" },
        { "top_left", "Point for the top-left of image plane", "reallist" },
        { "bottom_left",
          "Point for the bottom-left of image plane",
          "reallist" },
        { "bottom_right",
          "Point for the bottom-right of image plane",
          "reallist" }
    };
    m.return_documentation = "An integer plot id";

    m.set_code([&](noo::MethodContext const&,
                   QByteArray     data,
                   ForceToGLMVec3 pa,
                   ForceToGLMVec3 pb,
                   ForceToGLMVec3 pc) {
        return p.append<ImagePlot>(-1, data, pa.v, pb.v, pc.v);
    });

    return noo::create_method(p.document().get(), m);
}

// Update Table ================================================================

// auto make_update_table_method(Plotty& p) {

//    noo::MethodData update_table;
//    update_table.method_name            = "update_table_plot";
//    update_table.documentation          = "Update table plot settings";
//    update_table.argument_documentation = {
//        { "plot_id", "Table plot identifier" },
//        { "settings_array",
//          "Array (count of 5) of source data columns indicies [ x, y, z, col,
//          " "scale ]. Arg can be nothing to skip. col and scale indicies can
//          be " "negative to not use." },
//        { "color_map",
//          "Color map. A list of [ [real, 'hexcolor'], ... ], where the "
//          "real key is from 0->1. If None, will try to use the given color "
//          "column directly for colors." }
//    };
//    update_table.return_documentation = "None";

//    struct ColorMapArg {
//        std::vector<std::pair<float, glm::vec3>> color_map;

//        ColorMapArg() = default;
//        ColorMapArg(QCborValue a) {
//            auto cmap_list = a.to_vector();

//            cmap_list.for_each([&](auto, auto v) {
//                auto control_p = v.to_vector();

//                auto control_p_size = control_p.size();

//                auto key   = control_p_size >= 1 ? control_p[0].to_real() : 0;
//                auto value = control_p_size >= 2 ? control_p[0].to_string()
//                                                 : std::string_view();

//                auto qt_col = QColor(noo::to_qstring(value));

//                color_map.emplace_back(key,
//                                       glm::vec3 { qt_col.redF(),
//                                                   qt_col.greenF(),
//                                                   qt_col.blueF() });
//            });
//        }
//    };

//    update_table.set_code([&p](noo::MethodContext const&,
//                               int64_t                  plot_id,
//                               std::span<int64_t const> columns,
//                               ColorMapArg              cmap) -> noo::AnyVar {
//        // interpret

//        auto* target = p.get_plot(plot_id);

//        if (!target) throw noo::MethodException("Bad plot id!");

//        auto* table_plot = dynamic_cast<TablePlot*>(target);

//        if (!table_plot)
//            throw noo::MethodException("Plot is not a table plot!");

//        {
//            auto x = get_or_default(columns, 0);
//            auto y = get_or_default(columns, 1);
//            auto z = get_or_default(columns, 2);
//            auto c = get_or_default(columns, 3);
//            auto s = get_or_default(columns, 4);

//            if (x < 0 or y < 0 or z < 0)
//                throw noo::MethodException("Need extant x y and z columns!");

//            table_plot->set_columns(x, y, z, c, s, cmap.color_map);
//        }


//        return true;
//    });

//    return noo::create_method(p.document().get(), update_table);
//}

// Add Plotty ==================================================================

Plotty::Plotty(uint16_t port) {
    m_shared_domain = new SharedDomain(this);

    connect(m_shared_domain,
            &SharedDomain::domain_updated,
            this,
            &Plotty::on_domain_updated);

    noo::ServerOptions options {
        .port = port,
    };

    m_server = noo::create_server(options);

    qInfo() << "Creating server, listening on port" << port;

    Q_ASSERT(m_server);

    m_doc = noo::get_document(m_server.get());

    Q_ASSERT(m_doc);

    noo::DocumentData docup;

    {
        //        auto ptr = make_load_table_method(*this);
        //        docup.method_list.push_back(ptr);

        QVector<noo::MethodTPtr> methods;


        auto ptr = make_new_point_plot_method(*this);
        methods.push_back(ptr);

        ptr = make_new_line_segment_plot_method(*this);
        methods.push_back(ptr);

        ptr = make_new_image_plot_method(*this);
        methods.push_back(ptr);

        ptr = make_set_domain_method(*this);
        methods.push_back(ptr);

        //        ptr = make_update_table_method(*this);
        //        docup.method_list.push_back(ptr);

        docup.method_list = methods;
    }

    noo::update_document(m_doc, docup);

    {
        noo::ObjectData nd;

        nd.name = "Plot Root";

        nd.create_callbacks = [this](noo::ObjectT* t) {
            return std::make_unique<PlottyRootCallbacks>(this, t);
        };


        m_plot_root = noo::create_object(m_doc, nd);
    }

    auto add_light = [this](glm::vec3 p, QColor color, float i) {
        auto& nl = m_lights.emplace_back();

        noo::LightData light_data;
        light_data.color     = color;
        light_data.intensity = i;
        light_data.type      = noo::PointLight {};

        nl.l = noo::create_light(m_doc, light_data);

        noo::ObjectData nd;

        nd.transform = glm::translate(glm::mat4(1), p);

        nd.lights.emplace().push_back(nl.l);

        nd.tags.emplace().emplace_back(noo::names::tag_user_hidden);

        nl.o = noo::create_object(m_doc, nd);
    };

    add_light({ 0, 0, 1 }, Qt::white, 2);
    add_light({ 1, 0, 0 }, QColor(200, 200, 255), 2);
    add_light({ 0, 1, 0 }, QColor(200, 255, 200), 2);

    rebuild_axis();
    make_box();
}

Plotty::~Plotty() { }

std::shared_ptr<noo::DocumentT> Plotty::document() {
    return m_doc;
}

noo::ObjectTPtr Plotty::plot_root() {
    return m_plot_root;
}

SharedDomain* Plotty::domain() const {
    return m_shared_domain;
}

// int64_t Plotty::add_immediate_plot(std::vector<glm::vec3>&& points,
//                                   std::vector<glm::vec3>&& colors,
//                                   std::vector<glm::vec3>&& scales) {

//    return append<PointPlot>(
//        0, std::move(points), std::move(colors), std::move(scales));

//    return 0;
//}

Plot* Plotty::get_plot(size_t i) {
    auto iter = m_plots.find(i);

    if (iter == m_plots.end()) return nullptr;

    return iter->second.get();
}

void Plotty::on_domain_updated() {
    make_box();
}

void Plotty::on_domain_labels_updated() {
    rebuild_axis();
}
