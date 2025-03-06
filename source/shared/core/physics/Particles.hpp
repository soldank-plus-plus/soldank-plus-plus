#ifndef __PARTICLES_HPP__
#define __PARTICLES_HPP__

#include "core/data/FileReader.hpp"
#include "core/data/IFileReader.hpp"

#include "core/math/Glm.hpp"

#include <memory>
#include <vector>
#include <string>

namespace Soldank
{
class Particle
{
public:
    Particle() = default;

    Particle(bool _active,
             glm::vec2 _position,
             glm::vec2 _old_position,
             glm::vec2 _velocity,
             glm::vec2 force,
             float one_over_mass,
             float timestep,
             float gravity,
             float e_damping,
             float v_damping);

    void Euler();
    void Verlet();

    glm::vec2 GetVelocity() const { return velocity_; }
    void SetVelocity(glm::vec2 new_velocity) { velocity_ = new_velocity; }
    glm::vec2 GetForce() const { return force_; }
    void SetForce(glm::vec2 new_force) { force_ = new_force; }
    float GetOneOverMass() const { return one_over_mass_; }

    bool active{ false };
    glm::vec2 position{};
    glm::vec2 old_position{};
    glm::vec2 velocity_{};

private:
    glm::vec2 force_{};
    float one_over_mass_{};
    float timestep_{};
    float gravity_{};
    float e_damping_{};
    float v_damping_{};
};

struct Constraint
{
    bool active;
    glm::uvec2 particle_num;
    float rest_length;
};

enum class ParticleSystemType : unsigned int
{
    Soldier = 0,
    Flag,
    Weapon,
    Kit,
    Parachute,
    StationaryGun
};

class ParticleSystem
{
public:
    ParticleSystem(const std::vector<Particle>& particles,
                   const std::vector<Constraint>& constraints);

    void DoVerletTimestep();
    void DoVerletTimestepFor(unsigned int particle_num, unsigned int constraint_num);
    void DoEulerTimestep();
    void DoEulerTimestepFor(unsigned int particle_num);
    void SatisfyConstraints();
    void SatisfyConstraintFor(unsigned int constraint_num);
    static void SatisfyConstraint(const Constraint& constraint, std::vector<Particle>& particles);

    static std::shared_ptr<ParticleSystem> Load(ParticleSystemType particle_system_type,
                                                float scale = 4.5F,
                                                const IFileReader& file_reader = FileReader());

    bool GetActive(unsigned int particle_num) const
    {
        // TODO: indexes are from 1 like in pascal, we need to change it at loading time to be
        // indexed from 0
        return particles_[particle_num - 1].active;
    }

    const glm::vec2& GetPos(unsigned int particle_num) const
    {
        return particles_[particle_num - 1].position;
    }

    void SetPos(unsigned int particle_num, glm::vec2 new_pos)
    {
        particles_[particle_num - 1].position = new_pos;
    }

    const glm::vec2& GetOldPos(unsigned int particle_num) const
    {
        return particles_[particle_num - 1].old_position;
    }

    void SetOldPos(unsigned int particle_num, glm::vec2 new_old_pos)
    {
        particles_[particle_num - 1].old_position = new_old_pos;
    }

    glm::vec2 GetForce(unsigned int particle_num)
    {
        return particles_[particle_num - 1].GetForce();
    }

    void SetForce(unsigned int particle_num, glm::vec2 new_force)
    {
        particles_[particle_num - 1].SetForce(new_force);
    }

    const std::vector<Particle>& GetParticles() const { return particles_; }

    const std::vector<Constraint>& GetConstraints() const { return constraints_; }

private:
    static void ScaleParticles(ParticleSystem* particle_system, float scale);
    static std::shared_ptr<ParticleSystem> LoadFromFile(
      const std::string& file_name,
      float scale,
      float timestep,
      float gravity,
      float e_damping,
      float v_damping,
      const IFileReader& file_reader = FileReader());

    std::vector<Particle> particles_;
    std::vector<Constraint> constraints_;
};
} // namespace Soldank

#endif
