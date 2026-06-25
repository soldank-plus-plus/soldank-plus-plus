module;

#include <ios>
#include <filesystem>

#include "core/utility/Expected.hpp"

export module Shared.Core.Data.IFileWriter;

export namespace Soldank
{
enum class FileWriterError
{
    NoError = 0,
    FileNotOpen,
    BufferError
};

class IFileWriter
{
public:
    virtual ~IFileWriter() = default;
    virtual FileWriterError AppendData(const char* data, std::streamsize data_size) = 0;
    virtual FileWriterError Write(const std::filesystem::path& file_path,
                                  std::ios_base::openmode mode = std::ios_base::out) const = 0;
};
} // namespace Soldank
