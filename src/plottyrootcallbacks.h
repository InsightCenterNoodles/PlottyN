#ifndef PLOTTYROOTCALLBACKS_H
#define PLOTTYROOTCALLBACKS_H

#include "noo_server_interface.h"

class Plotty;

class PlottyRootCallbacks : public noo::ObjectCallbacks {
    Plotty* m_plotty;

public:
    PlottyRootCallbacks(Plotty* plotty, noo::ObjectT*);

    void select_region(glm::vec3 min, glm::vec3 max, SelAction select) override;
    void
    select_sphere(glm::vec3 point, float distance, SelAction select) override;
    void
    select_plane(glm::vec3 point, glm::vec3 normal, SelAction select) override;
    void select_hull(std::span<glm::vec3 const> hull,
                     std::span<int64_t const>   index,
                     SelAction                  select) override;

    std::pair<std::string, glm::vec3> probe_at(glm::vec3) override;
};

#endif // PLOTTYROOTCALLBACKS_H
