#include "plottyrootcallbacks.h"

#include "plot.h"
#include "plotty.h"

#include <QDebug>

#include <sstream>

static noo::ObjectCallbacks::EnableCallback flags = {
    .selection = true,
    .probing   = true,
};

PlottyRootCallbacks::PlottyRootCallbacks(Plotty* plotty, noo::ObjectT* t)
    : noo::ObjectCallbacks(t, flags), m_plotty(plotty) {
    qDebug() << Q_FUNC_INFO << this;
}


void PlottyRootCallbacks::select_region(glm::vec3 min,
                                        glm::vec3 max,
                                        SelAction select) {
    SpatialSelection sel =
        SelectRegion { .min = min, .max = max, .select = (int)select };

    for (auto& [k, v] : *m_plotty) {
        v->handle_selection(sel);
    }
}

void PlottyRootCallbacks::select_sphere(glm::vec3 point,
                                        float     distance,
                                        SelAction select) {

    SpatialSelection sel = SelectSphere { .point  = point,
                                          .radius = distance,
                                          .select = (int)select };
    for (auto& [k, v] : *m_plotty) {
        v->handle_selection(sel);
    }
}

void PlottyRootCallbacks::select_plane(glm::vec3 point,
                                       glm::vec3 normal,
                                       SelAction select) {

    SpatialSelection sel =
        SelectPlane { .point = point, .normal = normal, .select = (int)select };
    for (auto& [k, v] : *m_plotty) {
        v->handle_selection(sel);
    }
}

void PlottyRootCallbacks::select_hull(std::span<glm::vec3 const> hull,
                                      std::span<int64_t const>   index,
                                      SelAction                  select) {

    SpatialSelection sel =
        SelectHull { .points = hull, .index = index, .select = (int)select };
    for (auto& [k, v] : *m_plotty) {
        v->handle_selection(sel);
    }
}


std::pair<std::string, glm::vec3> PlottyRootCallbacks::probe_at(glm::vec3 p) {
    // point is in the physical domain, not the data domain.

    std::stringstream ss;

    glm::vec3 place;
    int       place_count = 0;

    for (auto& [k, v] : *m_plotty) {

        auto result = v->handle_probe(p);

        if (result.text.empty()) continue;

        ss << "Plt " << k << ": " << result.text << "\n";

        if (result.place) {
            if (place_count > 0) {
                place = place + (*result.place - place) / (float)place_count;
                place_count += 1;
            } else {
                place       = *result.place;
                place_count = 1;
            }
        }
    }

    return { ss.str(), place };
}
