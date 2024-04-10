
#include "PostProcessingCamera.h"

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/Containers/StringStl.h>
#include <Corrade/Utility/FormatStl.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Version.h>

namespace Magnum { namespace Examples {

PostProcessingCamera::PostProcessingCamera(SceneGraph::AbstractObject3D& object): SceneGraph::Camera3D(object), framebuffer(PixelFormat::RGB8Unorm), canvas(frame) {
    (frame = new GL::Texture2D)
        ->setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMinificationFilter(GL::SamplerFilter::Nearest)
        .setMagnificationFilter(GL::SamplerFilter::Nearest);
}

PostProcessingCamera::~PostProcessingCamera() {
    delete frame;
}

void PostProcessingCamera::setViewport(const Vector2i& size) {
    SceneGraph::Camera3D::setViewport(size);

    /* Initialize previous frames with black color */
    std::size_t textureSize = size.product()*framebuffer.pixelSize();
    Containers::Array<UnsignedByte> texture{ValueInit, textureSize};
    framebuffer.setData(PixelFormat::RGB8Unorm, size, texture, GL::BufferUsage::DynamicDraw);

    frame->setImage(0, GL::TextureFormat::RGB8, framebuffer);
}

void PostProcessingCamera::draw(SceneGraph::DrawableGroup3D& group) {
    SceneGraph::Camera3D::draw(group);

    GL::defaultFramebuffer.read({{}, viewport()}, framebuffer, GL::BufferUsage::DynamicDraw);

    frame->setImage(0, GL::TextureFormat::RGB8, framebuffer);

    canvas.draw();
}

PostProcessingCamera::PostProcessingShader::PostProcessingShader() {
    Utility::Resource rs("shaders");

    GL::Shader vert(GL::Version::GL330, GL::Shader::Type::Vertex);
    GL::Shader frag(GL::Version::GL330, GL::Shader::Type::Fragment);

    vert.addSource(rs.getString("LUTPostProcessingShader.vert"));
    frag.addSource(rs.getString("LUTPostProcessingShader.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile() && frag.compile());

    attachShaders({vert, frag});

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    setUniform(uniformLocation("frame"), 0);
}

PostProcessingCamera::PostProcessingCanvas::PostProcessingCanvas(GL::Texture2D* frame, Object3D* parent): Object3D(parent), frame(frame) {
    
    // Canvas = a quad covering screen.
    const Vector2 vertices[] = {
        {1.0f, -1.0f},
        {1.0f, 1.0f},
        {0.0f, -1.0f},
        {0.0f, 1.0f}
    };
    buffer.setData(vertices, GL::BufferUsage::StaticDraw);
    screenFillingMesh.setPrimitive(GL::MeshPrimitive::TriangleStrip)
        .setCount(4)
        .addVertexBuffer(buffer, 0, PostProcessingShader::Position());
}

void PostProcessingCamera::PostProcessingCanvas::draw() {
    frame->bind(0); // TODO: Make sure that this is  OK.
    shader.draw(screenFillingMesh);
}

}}