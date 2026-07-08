module;

export module Camera;

import Extern.Glm;

export namespace Soldank
{
class Camera
{
private:
    static constexpr const float ZOOM_INCREMENT_FACTOR = 2.0;
    static constexpr const float ZOOM_INITIAL_VALUE = 1.0;
    static constexpr const float ZOOM_MIN_VALUE = ZOOM_INITIAL_VALUE / 16;
    static constexpr const float ZOOM_MAX_VALUE = ZOOM_INITIAL_VALUE * 32;

public:
    Camera()
        : camera_position_({ 0.0, 0.0 })
        , zoom_(ZOOM_INITIAL_VALUE)
        , window_dimensions_({ 640.0F, 480.0F })
    {
    }

    glm::mat4 GetView() const
    {
        float left = camera_position_.x - GetWidth() / 2;
        float right = camera_position_.x + GetWidth() / 2;
        float bottom = camera_position_.y - GetHeight() / 2;
        float top = camera_position_.y + GetHeight() / 2;
        return glm::ortho(left, right, bottom, top);
    }

    void ZoomIn()
    {
        zoom_ /= ZOOM_INCREMENT_FACTOR;
        if (zoom_ <= ZOOM_MIN_VALUE) {
            zoom_ = ZOOM_MIN_VALUE;
        }
    }

    void ZoomOut()
    {
        zoom_ *= ZOOM_INCREMENT_FACTOR;
        if (zoom_ >= ZOOM_MAX_VALUE) {
            zoom_ = ZOOM_MAX_VALUE;
        }
    }

    void ResetZoom() { zoom_ = ZOOM_INITIAL_VALUE; }

    float GetZoom() const { return zoom_; }

    void Move(float new_x, float new_y)
    {
        camera_position_.x = new_x;
        camera_position_.y = new_y;
    }

    float GetX() const { return camera_position_.x; }
    float GetY() const { return camera_position_.y; }

    float GetWidth() const { return window_dimensions_.x * zoom_ / GetSoldierPOVZoomFactor(); }

    float GetHeight() const { return window_dimensions_.y * zoom_ / GetSoldierPOVZoomFactor(); }

    void UpdateWindowDimensions(const glm::vec2& window_dimensions)
    {
        window_dimensions_ = window_dimensions;
    }

private:
    float GetSoldierPOVZoomFactor() const { return window_dimensions_.x / 800.0F; }

    glm::vec2 camera_position_;
    float zoom_;
    glm::vec2 window_dimensions_;
};
} // namespace Soldank
