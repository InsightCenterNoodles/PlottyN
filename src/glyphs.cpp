#include "glyphs.h"


static std::vector<glm::vec3> const sphere_vertex_info = {
    { 0.000000, -1.250544, 0.000000 },   { 0.904894, -0.559262, 0.657436 },
    { -0.345632, -0.559262, 1.063763 },  { -1.118518, -0.559262, 0.000000 },
    { -0.345632, -0.559262, -1.063763 }, { 0.904894, -0.559262, -0.657436 },
    { 0.345632, 0.559262, 1.063763 },    { -0.904894, 0.559262, 0.657436 },
    { -0.904894, 0.559262, -0.657436 },  { 0.345632, 0.559262, -1.063763 },
    { 1.118518, 0.559262, 0.000000 },    { 0.000000, 1.250544, 0.000000 },
};

static std::vector<glm::vec3> const sphere_normal_info = []() {
    std::vector<glm::vec3> ret = sphere_vertex_info;

    for (auto& n : ret) {
        n = glm::normalize(n);
    }

    return ret;
}();

static std::vector<glm::u16vec3> const sphere_index_info = {
    { 0, 1, 2 },   { 1, 0, 5 },  { 0, 2, 3 },  { 0, 3, 4 },  { 0, 4, 5 },
    { 1, 5, 10 },  { 2, 1, 6 },  { 3, 2, 7 },  { 4, 3, 8 },  { 5, 4, 9 },
    { 1, 10, 6 },  { 2, 6, 7 },  { 3, 7, 8 },  { 4, 8, 9 },  { 5, 9, 10 },
    { 6, 10, 11 }, { 7, 6, 11 }, { 8, 7, 11 }, { 9, 8, 11 }, { 10, 9, 11 },
};


GlyphInfo build_common_sphere(noo::DocumentTPtr doc, noo::TableTPtr table) {
    noo::MaterialData mat;
    mat.color        = { 1, 1, 1, 1 };
    mat.metallic     = 0;
    mat.roughness    = 1;
    mat.use_blending = false;

    auto mat_ptr = create_material(doc, mat);

    noo::BufferMeshDataRef mesh_data;

    mesh_data.positions = sphere_vertex_info;
    mesh_data.normals   = sphere_normal_info;
    mesh_data.triangles = sphere_index_info;

    auto mesh = create_mesh(doc, mesh_data);

    noo::ObjectData object_data;
    object_data.material  = mat_ptr;
    object_data.transform = glm::mat4(1);
    object_data.mesh      = mesh;

    if (table) { object_data.tables.push_back(table); }

    auto obj = create_object(doc, object_data);

    return { mat_ptr, mesh, obj };
}
