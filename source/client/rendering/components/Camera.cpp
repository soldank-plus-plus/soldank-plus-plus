#include "Camera.hpp"
#include <glm/ext/matrix_clip_space.hpp>

namespace Soldank
{
Camera::Camera()
    : camera_position_({ 0.0, 0.0 })
    , zoom_(ZOOM_INITIAL_VALUE)
    , window_dimensions_({ 640.0F, 480.0F })
{
}

glm::mat4 Camera::GetView() const
{
    float left = camera_position_.x - GetWidth() / 2;
    float right = camera_position_.x + GetWidth() / 2;
    float bottom = camera_position_.y - GetHeight() / 2;
    float top = camera_position_.y + GetHeight() / 2;
    return glm::ortho(left, right, bottom, top);
}

void Camera::ZoomIn()
{
    zoom_ /= ZOOM_INCREMENT_FACTOR;
    if (zoom_ <= ZOOM_MIN_VALUE) {
        zoom_ = ZOOM_MIN_VALUE;
    }
}

void Camera::ZoomOut()
{
    zoom_ *= ZOOM_INCREMENT_FACTOR;
    if (zoom_ >= ZOOM_MAX_VALUE) {
        zoom_ = ZOOM_MAX_VALUE;
    }
}

void Camera::ResetZoom()
{
    zoom_ = ZOOM_INITIAL_VALUE;
}

void Camera::Move(float new_x, float new_y)
{
    camera_position_.x = new_x;
    camera_position_.y = new_y;
}

float Camera::GetWidth() const
{
    return window_dimensions_.x * zoom_ / GetSoldierPOVZoomFactor();
}

float Camera::GetHeight() const
{
    return window_dimensions_.y * zoom_ / GetSoldierPOVZoomFactor();
}

void Camera::UpdateWindowDimensions(const glm::vec2& window_dimensions)
{
    window_dimensions_ = window_dimensions;
}

float Camera::GetSoldierPOVZoomFactor() const
{
    return window_dimensions_.x / 800.0F;
}
} // namespace Soldank
