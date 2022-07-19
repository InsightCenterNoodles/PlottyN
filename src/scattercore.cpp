#include "scattercore.h"

ScatterCore::ScatterCore() { }


void ScatterCore::build_instances(std::span<glm::vec3> points,
                                  std::span<glm::vec3> colors,
                                  std::span<glm::vec3> scales,
                                  Domain const&        domain) {

    auto count = points.size();

    if (count == 0) {
        m_instances.clear();
        return;
    }

    m_instances.resize(count);

    if (m_instances.empty()) return;

    glm::vec3 default_col(1);
    glm::vec3 default_scale(1);

    std::span<glm::vec3> col_array =
        colors.size() ? std::span<glm::vec3>(colors)
                      : std::span<glm::vec3>(&default_col, 1);

    std::span<glm::vec3> scale_array =
        scales.size() ? std::span<glm::vec3>(scales)
                      : std::span<glm::vec3>(&default_scale, 1);

    for (size_t i = 0; i < count; i++) {
        glm::mat4& m = m_instances[i];

        m[0] = glm::vec4(domain.transform(points[i]), 1);
        m[1] = glm::vec4(col_array[i % col_array.size()], 1);
        m[2] = glm::vec4(0, 0, 0, 1);
        m[3] = glm::vec4(scale_array[i % scale_array.size()], 1);
    };
}

template <class T>
T const& modulus_indexed(std::span<T> t, size_t index) {
    return t[index % t.size()];
}

template <class T>
std::span<T const> seat_span(std::span<T const> source, T const& def) {
    return source.size() ? source : std::span<T const>(&def, 1);
}


void ScatterCore::build_instances(ArrayRef const& ref, Domain const& domain) {
    auto count = ref.px.size();

    if (count == 0) {
        m_instances.clear();
        glm::mat4& m = m_instances.emplace_back();

        m[0] = glm::vec4(0, 0, 0, 1);
        m[1] = glm::vec4(0, 0, 0, 1);
        m[2] = glm::vec4(0, 0, 0, 1);
        m[3] = glm::vec4(0, 0, 0, 1);
        return;
    }

    m_instances.resize(count);

    if (m_instances.empty()) return;

    glm::vec3 default_col(1);
    glm::vec3 default_scale(.05);

    auto col_r = seat_span(ref.cr, default_col.r);
    auto col_g = seat_span(ref.cg, default_col.g);
    auto col_b = seat_span(ref.cb, default_col.b);

    auto scale_x = seat_span(ref.sx, default_scale.x);
    auto scale_y = seat_span(ref.sy, default_scale.y);
    auto scale_z = seat_span(ref.sz, default_scale.z);

    for (size_t i = 0; i < count; i++) {
        glm::mat4& m = m_instances[i];

        auto p = glm::vec3 { ref.px[i], ref.py[i], ref.pz[i] };
        auto c = glm::vec3 {
            modulus_indexed(col_r, i),
            modulus_indexed(col_g, i),
            modulus_indexed(col_b, i),
        };

        auto s = glm::vec3 {
            modulus_indexed(scale_x, i),
            modulus_indexed(scale_y, i),
            modulus_indexed(scale_z, i),
        };

        m[0] = glm::vec4(domain.transform(p), 1);
        m[1] = glm::vec4(c, 1);
        m[2] = glm::vec4(0, 0, 0, 1);
        m[3] = glm::vec4(s, 1);
    };
}
