#ifndef PLOT_H
#define PLOT_H

#include <noo_server_interface.h>

#include <QObject>

#include <memory>

/*!
 * \brief Compute linear interpolation
 * \param x The interpolation source.
 * \param x0 The minimum range of x.
 * \param x1 The maximum range of x.
 * \param y0 The minimum range of y.
 * \param y1 The maximum range of y.
 *
 * Template T and U should support +, -, *, and / operators.
 */
template <class T, class U>
U lerp(T x, T const& x0, T const& x1, U const& y0, U const& y1) {
    return y0 + (y1 - y0) * ((x - x0) / (x1 - x0));
}

struct SelectRegion {
    glm::vec3 min, max;
    int       select;
};

struct SelectSphere {
    glm::vec3 point;
    float     radius;
    int       select;
};

struct SelectPlane {
    glm::vec3 point;
    glm::vec3 normal;
    int       select;
};

struct SelectHull {
    std::span<glm::vec3 const> points;
    std::span<int64_t const>   index;
    int                        select;
};

struct SpatialSelection
    : std::variant<SelectRegion, SelectSphere, SelectPlane, SelectHull> {

    using variant::variant;
};

struct Domain;

class Plotty;

class Plot : public QObject {
    Q_OBJECT
protected:
    Plotty*           m_host;
    noo::DocumentTPtr m_doc;
    int64_t           m_plot_id;

    //
    noo::MethodTPtr m_get_id_method;

    // owned by subclasses
    noo::MaterialTPtr m_mat;
    noo::MeshTPtr     m_mesh;
    noo::ObjectTPtr   m_obj;

    std::unordered_map<int, int> m_column_mapping;

    virtual void domain_updated(Domain const&);

public:
    Plot(Plotty& host, int64_t id);
    ~Plot();

    noo::ObjectTPtr const& object();

    virtual void handle_selection(SpatialSelection const&);

    struct ProbeResult {
        QString                  text;
        std::optional<glm::vec3> place;
    };

    virtual ProbeResult handle_probe(glm::vec3 const&);
};


#endif // PLOT_H
