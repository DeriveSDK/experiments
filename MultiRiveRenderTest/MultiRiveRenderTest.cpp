// MultiRiveRenderTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "juiceriv.h"
#include "thorvg.h"
#include "RiveRenderer.h"
#include "artboard.hpp"
#include "animation/linear_animation_instance.hpp"
#include "core/binary_reader.hpp"
#include "file.hpp"
#include <thread>
#include <iostream>
#include <chrono>

class Rive {
public:
    Rive(const unsigned char* data, const int len, tvg::SwCanvas* canvas);
    ~Rive();

    void position(float x, float y, float r);
    void update(double dt, tvg::SwCanvas* canvas);

protected:
    std::unique_ptr<tvg::Scene> _scene;
    tvg::Scene* _sceneRef = nullptr;
    rive::File* _file = nullptr;
    rive::Artboard* _artboard = nullptr;
    rive::LinearAnimationInstance* _animation = nullptr;
    double _rotation;
    float _x;
    float _y;
    RiveRenderer* _renderer;
};

Rive::Rive(const unsigned char* data, const int len, tvg::SwCanvas* canvas) {
    _scene = tvg::Scene::gen();
    _sceneRef = _scene.get();
    _renderer = new RiveRenderer(_sceneRef);

    // _sceneRef is a "Scene*". 
    // I overloaded Canvas::push to allow this.
    // How can I achieve this example code without this overload?
    canvas->push(_sceneRef);

    auto reader = rive::BinaryReader((uint8_t*)data, len);
    auto result = rive::File::import(reader, &_file);
    _artboard = _file->artboard();
    _artboard->advance(0.0f);
    _animation = new rive::LinearAnimationInstance(_artboard->animation(0));
}

Rive::~Rive() {
    delete _renderer;
}

void Rive::position(float x, float y, float r) {
    _x = x;
    _y = y;
    _rotation = r;
}

void Rive::update(double dt, tvg::SwCanvas* canvas) {
    if (_artboard) {
        if (_animation) {
            _animation->advance(dt);
            _animation->apply(_artboard);
        }

        rive::Mat2D m;
        rive::Mat2D::fromRotation(m, _rotation);
        m[4] = _x; // tx
        m[5] = _y; // ty

        _artboard->advance(dt);
        _sceneRef->clear();
        _renderer->save();
        _renderer->transform(m);
        _artboard->draw(_renderer);
        _renderer->restore();

        canvas->update(_sceneRef);
    }
}

int main()
{
    // Initialise thorvg
    tvg::Initializer::init(tvg::CanvasEngine::Sw, std::thread::hardware_concurrency());

    // Create a buffer and SwCanvas (and attach)
    uint32_t buffer = uint32_t(1000*1000);
    std::unique_ptr<tvg::SwCanvas> canvas = tvg::SwCanvas::gen();
    canvas->target(&buffer, 1000, 100, 100, tvg::SwCanvas::ARGB8888);

    // Create three rive files and position them
    Rive* rive1 = new Rive(juiceriv_data, juiceriv_data_len, canvas.get());
    rive1->position(400, 400, 0);
    Rive* rive2 = new Rive(juiceriv_data, juiceriv_data_len, canvas.get());
    rive2->position(700, 600, 2);
    Rive* rive3 = new Rive(juiceriv_data, juiceriv_data_len, canvas.get());
    rive3->position(500, 800, 4);
    
    // Now animate in a loop
    double r = 0;
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto end = std::chrono::steady_clock::now();
        double dt = (double)(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000.0f);
        start = end;

        canvas->clear(false);
        rive1->update(dt, canvas.get());
        rive2->update(dt, canvas.get());
        rive3->update(dt, canvas.get());
    }
}
