
#pragma once

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/BufferImage.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/SceneGraph/Camera.h>

#include "Types.h"

namespace Magnum { namespace Examples {

class PostProcessingCamera: public SceneGraph::Camera3D {
    public:

        explicit PostProcessingCamera(SceneGraph::AbstractObject3D& object);

        ~PostProcessingCamera();

        void setViewport(const Vector2i& size);
        void draw(SceneGraph::DrawableGroup3D& group);

    private:
        class PostProcessingShader: public GL::AbstractShaderProgram {
            public:
                typedef GL::Attribute<0, Vector2> Position;

                PostProcessingShader();

                PostProcessingShader& bindTexture(GL::Texture2D& texture) {
                    texture.bind(TextureUnit);
                    return *this;
                }
            
            private:
                enum: Int { TextureUnit = 0 };
        };

        class PostProcessingCanvas: public Object3D {
            public:
                PostProcessingCanvas(GL::Texture2D* frame, Object3D* parent = nullptr);

                void draw();

            private:
                PostProcessingShader shader;
                GL::Buffer vertexBuffer;
                GL::Mesh screenFillingMesh;
                GL::Texture2D* frame;
        };

        GL::BufferImage2D framebuffer;
        GL::Texture2D* frame;
        PostProcessingCanvas canvas;
};

}}