module;

#include <glad/glad.h>

#include <utility>
#include <vector>

export module Rendering.Gpu.GpuBuffer;

export namespace Soldank
{
enum class GpuBufferTarget {
    Array,
    ElementArray,
};

class GpuBuffer
{
public:
    GpuBuffer() = default;

    GpuBuffer(GpuBufferTarget target, const void* data, long long size, GLenum usage)
        : target_(target)
    {
        glGenBuffers(1, &id_);
        glBindBuffer(GetGlTarget(), id_);
        glBufferData(GetGlTarget(), size, data, usage);
    }

    static GpuBuffer CreateArrayBuffer(const std::vector<float>& vertices, GLenum usage)
    {
        return GpuBuffer(GpuBufferTarget::Array,
                         vertices.data(),
                         (long long)vertices.size() * (long long)sizeof(float),
                         usage);
    }

    ~GpuBuffer() { Reset(); }

    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;

    GpuBuffer(GpuBuffer&& other) noexcept
        : id_(std::exchange(other.id_, 0))
        , target_(other.target_)
    {
    }

    GpuBuffer& operator=(GpuBuffer&& other) noexcept
    {
        if (this != &other) {
            Reset();
            id_ = std::exchange(other.id_, 0);
            target_ = other.target_;
        }
        return *this;
    }

    unsigned int GetId() const { return id_; }

    void Update(const void* data, long long size, long long offset = 0) const
    {
        glBindBuffer(GetGlTarget(), id_);
        glBufferSubData(GetGlTarget(), offset, size, data);
    }

    void UpdateVertices(const std::vector<float>& vertices, long long offset = 0) const
    {
        Update(vertices.data(),
               (long long)vertices.size() * (long long)sizeof(float),
               offset);
    }

private:
    GLenum GetGlTarget() const
    {
        return target_ == GpuBufferTarget::Array ? GL_ARRAY_BUFFER : GL_ELEMENT_ARRAY_BUFFER;
    }

    void Reset()
    {
        if (id_ != 0) {
            glDeleteBuffers(1, &id_);
            id_ = 0;
        }
    }

    unsigned int id_ = 0;
    GpuBufferTarget target_ = GpuBufferTarget::Array;
};
} // namespace Soldank
