#include "imageplot.h"

#include "plotty.h"

void ImagePlot::rebuild(Domain const& d) {
    auto center = (m_top_left + m_bottom_right) / 2.0f;

    auto o = -(m_bottom_left - center);

    if (!m_image_texture) {
        m_image_texture = noo::create_texture(m_doc, m_image_data);
    }

    if (!m_image_mat) {
        noo::MaterialData mat;

        mat.pbr_info.base_color = Qt::white;
        mat.pbr_info.base_color_texture =
            noo::TextureRef { .source = m_image_texture };


        mat.pbr_info.metallic = 0;

        m_image_mat = noo::create_material(m_doc, mat);
    }

    noo::MeshTPtr mesh;

    {
        // create a plane

        std::array<glm::vec3, 4> positions = {
            m_top_left, m_bottom_left, m_bottom_right, o
        };

        for (auto& p : positions) {
            p = d.transform(p);
        }

        std::array<glm::vec3, 4> normals;

        for (auto& n : normals) {
            n = glm::cross(m_top_left - m_bottom_left,
                           m_bottom_right - m_bottom_left);
            n = glm::normalize(n);
        }

        std::array<glm::u16vec3, 2> index = {
            glm::u16vec3 { 0, 1, 2 },
            glm::u16vec3 { 3, 1, 2 },
        };

        noo::MeshSource ref;

        ref.positions    = positions;
        ref.normals      = normals;
        ref.indices      = std::as_bytes(std::span(index));
        ref.index_format = noo::Format::U16;
        ref.type         = noo::MeshSource::TRIANGLE;


        auto key = QStringLiteral("mesh");

        auto packed = noo::create_directory(m_doc, { { key, ref } });

        mesh = std::get<noo::MeshTPtr>(packed[key]);
    }


    noo::ObjectData object_data;
    object_data.definition = noo::ObjectRenderableDefinition { .mesh = mesh };
    object_data.transform  = glm::mat4(1);

    m_obj = create_object(m_doc, object_data);
}

ImagePlot::ImagePlot(Plotty&    host,
                     int64_t    id,
                     QByteArray image_data,
                     glm::vec3  top_left,
                     glm::vec3  bottom_left,
                     glm::vec3  bottom_right)
    : Plot(host, id),
      m_image_data(image_data),
      m_top_left(top_left),
      m_bottom_left(bottom_left),
      m_bottom_right(bottom_right) {
    rebuild(host.domain()->current_domain());
}

ImagePlot::~ImagePlot() { }

void ImagePlot::domain_updated(Domain const& d) {
    rebuild(d);
}
