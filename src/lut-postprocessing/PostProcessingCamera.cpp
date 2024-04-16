
#include "PostProcessingCamera.h"

#include <Corrade/Containers/Optional.h>
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
#include <Magnum/ImageView.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

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
    
    // Render scene.
    SceneGraph::Camera3D::draw(group);
    
    // Store render into framebuffer.
    GL::defaultFramebuffer.read({{}, viewport()}, framebuffer, GL::BufferUsage::DynamicDraw);
    
    // Store framebuffer into texture.
    frame->setImage(0, GL::TextureFormat::RGB8, framebuffer);
    
    // Render quad filling mesh with texture.
    canvas.draw();
}

PostProcessingCamera::PostProcessingShader::PostProcessingShader() {
    Utility::Resource rs("lut-postprocessing-resources");

    GL::Shader vert(GL::Version::GL330, GL::Shader::Type::Vertex);
    GL::Shader frag(GL::Version::GL330, GL::Shader::Type::Fragment);

    vert.addSource(rs.getString("LUTPostProcessingShader.vert"));
    frag.addSource(rs.getString("LUTPostProcessingShader.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile() && frag.compile());

    attachShaders({vert, frag});

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    setUniform(uniformLocation("frame"), TextureUnit);
}

PostProcessingCamera::PostProcessingCanvas::PostProcessingCanvas(GL::Texture2D* frame, Object3D* parent): Object3D(parent), frame(frame) {
    // Canvas = a quad covering screen.
    const Vector2 vertices[] = {
        {1.0f, -1.0f},
        {1.0f, 1.0f},
        {0.0f, -1.0f},
        {0.0f, 1.0f}
    };
    vertexBuffer.setData(vertices, GL::BufferUsage::StaticDraw);
    screenFillingMesh.setPrimitive(GL::MeshPrimitive::TriangleStrip)
        .setCount(4)
        .addVertexBuffer(vertexBuffer, 0, PostProcessingShader::Position());

    // LUT texture.
    // Image importer.
    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer =
        manager.loadAndInstantiate("TgaImporter");
    const Utility::Resource rs{"lut-postprocessing-resources"};
    if(!importer || !importer->openData(rs.getRaw("stone.tga")))
        std::exit(1);
    // Image to texture.
    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    LUTtexture->setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::textureFormat(image->format()), image->size())
        .setSubImage(0, {}, *image);
}

void PostProcessingCamera::PostProcessingCanvas::draw() {
    //printf("OK6\n");
    if(LUTtexture!=NULL)
        shader.bindTexture(LUTtexture); // TODO; problem!
    else
        printf("NULL\n");
    //printf("OK7\n");
    shader.draw(screenFillingMesh);       
    //printf("OK8\n");
}

}}