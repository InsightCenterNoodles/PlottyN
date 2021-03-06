#ifndef GLYPHS_H
#define GLYPHS_H

#include "plotty.h"

struct GlyphInfo {
    noo::MaterialTPtr material;
    noo::MeshTPtr     mesh;
    noo::ObjectTPtr   obj;
};

GlyphInfo build_common_sphere(QString           name,
                              noo::DocumentTPtr doc,
                              noo::TableTPtr    table = {});

#endif // GLYPHS_H
