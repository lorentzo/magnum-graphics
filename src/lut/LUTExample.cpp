
/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2024 — Lovro Bosnar <lovrobosnar.work@gmail.com>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/ImageView.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

#include "LUTShader.h"

namespace Magnum { namespace Examples {

class LUTExample: public Platform::Application {
    public:
        explicit LUTExample(const Arguments& arguments);

    private:
        void drawEvent() override;

        GL::Mesh _mesh;
        LUTShader _shader;
        GL::Texture2D _texture;
};

LUTExample::LUTExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("Magnum LUT Example")}
{
    // Mesh data.
    struct QuadVertex {
        Vector2 position;
        Vector2 textureCoordinates;
    };
    const QuadVertex vertices[]{
        {{ 0.5f, -0.5f}, {1.0f, 0.0f}}, /* Bottom right */
        {{ 0.5f,  0.5f}, {1.0f, 1.0f}}, /* Top right */
        {{-0.5f, -0.5f}, {0.0f, 0.0f}}, /* Bottom left */
        {{-0.5f,  0.5f}, {0.0f, 1.0f}}  /* Top left */
    };
    const UnsignedInt indices[]{        /* 3--1 1 */
        0, 1, 2,                        /* | / /| */
        2, 1, 3                         /* |/ / | */
    };                                  /* 2 2--0 */

    // Create mesh.
    _mesh.setCount(Containers::arraySize(indices))
        .addVertexBuffer(GL::Buffer{vertices}, 0,
            LUTShader::Position{},
            LUTShader::TextureCoordinates{})
        .setIndexBuffer(GL::Buffer{indices}, 0,
            GL::MeshIndexType::UnsignedInt);

    // Import image.
    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer =
        manager.loadAndInstantiate("TgaImporter");
    const Utility::Resource rs{"lut-data"};
    if(!importer || !importer->openData(rs.getRaw("texture.tga")))
        std::exit(1);

    // Create texture.
    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    _texture.setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::textureFormat(image->format()), image->size())
        .setSubImage(0, {}, *image);
}

void LUTExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    using namespace Math::Literals;

    _shader
        .setColor(0xffb2b2_rgbf)
        .bindTexture(_texture)
        .draw(_mesh);

    swapBuffers();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::LUTExample)