#include "../Source/PianoModel.h"
#include <cmath>
#include <cstdlib>

static void require(bool condition)
{
    if (! condition) std::exit(1);
}

static float energy(piano::Model& model, int samples)
{
    float total = 0;
    for (int i = 0; i < samples; ++i)
    {
        float l = 0, r = 0;
        model.tick(l, r);
        require(std::isfinite(l) && std::isfinite(r));
        total += l * l + r * r;
    }
    return total;
}

int main()
{
    piano::Model model;
    model.prepare(48000);
    piano::LayerSettings main, dual;
    dual.zone = 3;
    model.noteOn(60, 0.8f, 60, main, dual);
    require(energy(model, 4800) > 0.001f);

    model.setSustain(true);
    model.noteOff(60);
    require(energy(model, 4800) > 0.001f);
    model.setSustain(false);
    energy(model, 48000);

    model.prepare(48000);
    main.zone = dual.zone = 3;
    model.noteOn(60, 0.8f, 60, main, dual);
    require(energy(model, 4800) == 0.0f);
    main.zone = 2;
    dual.zone = 1;
    dual.bass = true;
    model.noteOn(48, 0.7f, 60, main, dual);
    require(energy(model, 4800) > 0.001f);
    model.noteOff(48);
    model.noteOn(72, 0.7f, 60, main, dual);
    require(energy(model, 4800) > 0.001f);
}
