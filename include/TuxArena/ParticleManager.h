#ifndef TUXARENA_PARTICLEMANAGER_H
#define TUXARENA_PARTICLEMANAGER_H

#include <vector>
#include "TuxArena/Particle.h"

namespace TuxArena
{
    class Renderer;

    class ParticleManager
    {
    public:
        void update(float deltaTime);
        void render(Renderer& renderer);

        void emitBlood(float x, float y, int count);
        void emitBulletTrail(float x, float y, float dirX, float dirY);

    private:
        std::vector<Particle> m_particles;
    };

} // namespace TuxArena

#endif // TUXARENA_PARTICLEMANAGER_H
