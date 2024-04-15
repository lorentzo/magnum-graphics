
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

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/DebugStl.h>
#include <Magnum/ImageView.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/Trade/PhongMaterialData.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Trade/TextureData.h>

#include "PostProcessingCamera.h"
#include "Types.h"

namespace Magnum { namespace Examples {

using namespace Math::Literals;

class LUTPostprocessingExample: public Platform::Application {
    public:
        explicit LUTPostprocessingExample(const Arguments& arguments);

        void viewportEvent(ViewportEvent& event) override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;
        Vector3 positionOnSphere(const Vector2i& position) const;

    private:
        void drawEvent() override;

        Scene3D _scene;

        Shaders::PhongGL _coloredShader;
        Containers::Array<Containers::Optional<GL::Mesh>> _meshes;
        SceneGraph::DrawableGroup3D _drawables;

        Object3D *_cameraObject;
        Object3D _manipulator;
        PostProcessingCamera* _camera;
        
        Vector3 _previousPosition;
};

class ColoredDrawable: public SceneGraph::Drawable3D {
    public:
        explicit ColoredDrawable(Object3D& object, Shaders::PhongGL& shader, GL::Mesh& mesh, const Color4& color, SceneGraph::DrawableGroup3D& group): SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _color{color} {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Shaders::PhongGL& _shader;
        GL::Mesh& _mesh;
        Color4 _color;
};

LUTPostprocessingExample::LUTPostprocessingExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("Magnum LUT Example")}
{
    // Input.
    Utility::Arguments args;
    args.addArgument("file").setHelp("file", "file to load")
        .addOption("importer", "AnySceneImporter")
            .setHelp("importer", "importer plugin to use")
        .addSkippedPrefix("magnum", "engine-specific options")
        .setGlobalHelp("Displays a 3D scene file provided on command line.")
        .parse(arguments.argc, arguments.argv);

    // Camera.

    // Post processing camera.
    _cameraObject = new Object3D(&_scene);
    _cameraObject->translate(Vector3::zAxis(3.0f));
    (_camera = new PostProcessingCamera(*_cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.001f, 100))
        .setViewport(GL::defaultFramebuffer.viewport().size());

    // Manipulator.
    _manipulator.setParent(&_scene);

    // GL state.
    GL::Renderer::setClearColor({0.1f, 0.1f, 0.1f});
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    // Shaders (Magnum built-in).
    _coloredShader
        .setAmbientColor(0x111111_rgbf)
        .setSpecularColor(0xffffff_rgbf)
        .setShininess(80.0f);

    // Importer plugin.
    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer =
        manager.loadAndInstantiate(args.value("importer"));

    if(!importer || !importer->openFile(args.value("file")))
        std::exit(1);

    // Create drawable objects.
    _meshes = Containers::Array<Containers::Optional<GL::Mesh>>{importer->meshCount()};
    for(UnsignedInt i = 0; i != importer->meshCount(); ++i) 
    {
        // Import mesh.
        Containers::Optional<Trade::MeshData> meshData;
        if(!(meshData = importer->mesh(i))) 
        {
            Warning{} << "Cannot load mesh" << i << importer->meshName(i);
            continue;
        }

        // Mesh flags: compute normals.
        MeshTools::CompileFlags flags;
        if(!meshData->hasAttribute(Trade::MeshAttribute::Normal))
        {
            flags |= MeshTools::CompileFlag::GenerateFlatNormals;
        }
        _meshes[i] = MeshTools::compile(*meshData, flags);

        // Assign shader to mesh.
        new ColoredDrawable{_manipulator, _coloredShader, *_meshes[i], 0xffffff_rgbf, _drawables};
    }
    
}

void ColoredDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setDiffuseColor(_color)
        .setLightPositions({
            {camera.cameraMatrix().transformPoint({-3.0f, 10.0f, 10.0f}), 0.0f}
        })
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .draw(_mesh);
}

void LUTPostprocessingExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);
    _camera->draw(_drawables);

    swapBuffers();

    redraw();
}

void LUTPostprocessingExample::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
    _camera->setViewport(event.windowSize());
}

void LUTPostprocessingExample::mousePressEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = positionOnSphere(event.position());
}

void LUTPostprocessingExample::mouseReleaseEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = Vector3();
}

void LUTPostprocessingExample::mouseScrollEvent(MouseScrollEvent& event) {
    if(!event.offset().y()) return;

    /* Distance to origin */
    const Float distance = _cameraObject->transformation().translation().z();

    /* Move 15% of the distance back or forward */
    _cameraObject->translate(Vector3::zAxis(
        distance*(1.0f - (event.offset().y() > 0 ? 1/0.85f : 0.85f))));

    redraw();
}

Vector3 LUTPostprocessingExample::positionOnSphere(const Vector2i& position) const {
    const Vector2 positionNormalized = Vector2{position}/Vector2{_camera->viewport()} - Vector2{0.5f};
    const Float length = positionNormalized.length();
    const Vector3 result(length > 1.0f ? Vector3(positionNormalized, 0.0f) : Vector3(positionNormalized, 1.0f - length));
    return (result*Vector3::yScale(-1.0f)).normalized();
}

void LUTPostprocessingExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    const Vector3 currentPosition = positionOnSphere(event.position());
    const Vector3 axis = Math::cross(_previousPosition, currentPosition);

    if(_previousPosition.length() < 0.001f || axis.length() < 0.001f) return;

    _manipulator.rotate(Math::angle(_previousPosition, currentPosition), axis.normalized());
    _previousPosition = currentPosition;

    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::LUTPostprocessingExample)