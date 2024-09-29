#include "Camera.hpp"

namespace Soldank
{
Camera::Camera()
    : camera_position_({ 0.0, 0.0 })
    , zoom_(ZOOM_INITIAL_VALUE)
{
}

glm::mat4 Camera::GetView() const
{
    return glm::ortho(camera_position_.x - GetWidth() / 2,
                      camera_position_.x + GetWidth() / 2,
                      camera_position_.y - GetHeight() / 2,
                      camera_position_.y + GetHeight() / 2);
}

void Camera::ZoomIn()
{
    zoom_ += ZOOM_INCREMENT;
    if (zoom_ >= ZOOM_MAX_VALUE) {
        zoom_ = ZOOM_MAX_VALUE;
    }
}

void Camera::ZoomOut()
{
    zoom_ -= ZOOM_INCREMENT;
    if (zoom_ <= ZOOM_MIN_VALUE) {
        zoom_ = ZOOM_MIN_VALUE;
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
    float canvas_width = WIDTH;
    // TODO: fix magic number.
    return canvas_width - canvas_width * zoom_ * ZOOM_POWER;
}

float Camera::GetHeight() const
{
    float canvas_height = HEIGHT;
    return canvas_height - canvas_height * zoom_ * ZOOM_POWER;
}
} // namespace Soldank
