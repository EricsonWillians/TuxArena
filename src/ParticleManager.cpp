#include "TuxArena/ParticleManager.h"
#include "TuxArena/Renderer.h"
#include <cmath>

namespace TuxArena
{

    void ParticleManager::update(float deltaTime)
    {
        for (auto it = m_particles.begin(); it != m_particles.end();)
        {
            it->lifetime -= deltaTime;
            if (it->lifetime <= 0)
            {
                it = m_particles.erase(it);
            }
            else
            {
                it->position.x += it->velocity.x * deltaTime;
                it->position.y += it->velocity.y * deltaTime;
                // Fade out
                it->color.a = static_cast<uint8_t>(255 * (it->lifetime / it->initialLifetime));
                ++it;
            }
        }
    }

    void ParticleManager::render(Renderer& renderer)
    {
        for (const auto& p : m_particles)
        {
            renderer.drawCircle(p.position.x, p.position.y, p.size, p.color, true); // Draw as filled circle
        }
    }

    void ParticleManager::emitBlood(float x, float y, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            Particle p;
            p.position = { x, y };
            float angle = (float)(rand() % 360) * M_PI / 180.0f;
            float speed = (float)(rand() % 150 + 50);
            p.velocity = { std::cos(angle) * speed, std::sin(angle) * speed };
            // Darker, more desaturated blood color
            p.color = { (uint8_t)(rand() % 30 + 100), (uint8_t)(rand() % 20), (uint8_t)(rand() % 20), 255 };
            p.lifetime = (float)(rand() % 100) / 100.0f + 0.5f;
            p.initialLifetime = p.lifetime;
            p.size = (float)(rand() % 2 + 1); // Smaller size for point-like particles
            p.type = ParticleType::Blood;
            m_particles.push_back(p);
        }
    }

    void ParticleManager::emitBulletTrail(float x, float y, float dirX, float dirY)
    {
        Particle p;
        p.position = { x, y };
        // Bullet trail particles should move slightly in the direction of the bullet
        // and then dissipate quickly.
        float speed = 50.0f; // Slower speed for trails
        p.velocity = { dirX * speed, dirY * speed };
        p.color = { 255, 255, 200, 200 }; // Faint yellow/white for bullet trails
        p.lifetime = 0.2f; // Short lifetime
        p.initialLifetime = p.lifetime;
        p.size = 1.0f; // Small size
        p.type = ParticleType::BulletTrail;
        m_particles.push_back(p);
    }

} // namespace TuxArena
