#ifndef __CAMERA_HPP__
#define __CAMERA_HPP__

#include "core/math/Glm.hpp"

namespace Soldank
{
class Camera
{
public:
    Camera();

    glm::mat4 GetView() const;

    void ZoomIn();
    void ZoomOut();
    void ResetZoom();
    void Move(float new_x, float new_y);

    float GetX() const { return camera_position_.x; }
    float GetY() const { return camera_position_.y; }

    float GetWidth() const;
    float GetHeight() const;

    float GetZoom() const { return zoom_; }

    void UpdateWindowDimensions(const glm::vec2& window_dimensions);

private:
    static constexpr const float ZOOM_INCREMENT_FACTOR = 2.0;
    static constexpr const float ZOOM_INITIAL_VALUE = 1.0;
    static constexpr const float ZOOM_MIN_VALUE = ZOOM_INITIAL_VALUE / 16;
    static constexpr const float ZOOM_MAX_VALUE = ZOOM_INITIAL_VALUE * 32;

    float GetSoldierPOVZoomFactor() const;

    glm::vec2 camera_position_;
    float zoom_;
    glm::vec2 window_dimensions_;
};
} // namespace Soldank

#endif
