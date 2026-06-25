module;

#include <expected>
#include <string>
#include <ios>

#include "core/utility/Expected.hpp"

export module Shared.Core.Data.IFileReader;

export namespace Soldank
{
enum class FileReaderError
{
    FileNotFound = 0,
    BufferError
};

class IFileReader
{
public:
    virtual ~IFileReader() = default;
    virtual std::expected<std::string, FileReaderError> Read(
      const std::string& file_path,
      std::ios_base::openmode mode = std::ios_base::in) const = 0;
};
} // namespace Soldank
