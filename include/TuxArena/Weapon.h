#ifndef TUXARENA_WEAPON_H
#define TUXARENA_WEAPON_H

#include <string>
#include <vector>
#include "TuxArena/Entity.h" // For EntityContext

namespace TuxArena
{

    class Player; // Forward declaration

    enum class WeaponType
    {
        PISTOL,
        SHOTGUN,
        // Add more types like ROCKET_LAUNCHER, RAILGUN, etc.
    };

    struct WeaponDef
    {
        WeaponType type = WeaponType::PISTOL;
        std::string name = "Pistol";
        float fireRate = 0.5f;       // Shots per second
        int projectilesPerShot = 1;  // How many bullets per shot (1 for pistol, >1 for shotgun)
        float projectileSpeed = 600.0f;
        float projectileDamage = 10.0f;
        float projectileLifetime = 2.0f; // In seconds
        float spreadAngle = 5.0f;      // Cone of fire in degrees. 0 for perfect accuracy.
        int ammoCost = 1;
        // Sound effects, muzzle flash texture, etc. can be added here
    };

    class Weapon
    {
    public:
        Weapon(const WeaponDef& def, Player* owner);

        void update(const EntityContext& context);
        bool shoot(const EntityContext& context);

        const WeaponDef& getDefinition() const { return m_definition; }
        bool isOnCooldown() const { return m_shootTimer > 0; }

    private:
        WeaponDef m_definition;
        Player* m_owner; // The player who owns this weapon
        float m_shootTimer;
    };

} // namespace TuxArena

#endif // TUXARENA_WEAPON_H
