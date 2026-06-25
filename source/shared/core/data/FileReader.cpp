module;

#include <fstream>
#include <ios>
#include <sstream>
#include <string>

#include "core/utility/Expected.hpp"

export module Shared.Core.Data.FileReader;

import Shared.Core.Data.IFileReader;

export namespace Soldank
{
class FileReader : public IFileReader
{
public:
    std::expected<std::string, FileReaderError> Read(
      const std::string& file_path,
      std::ios_base::openmode mode = std::ios_base::in) const override
    {
        std::ifstream file_to_read(file_path, mode);
        if (!file_to_read.is_open()) {
            return std::unexpected(FileReaderError::FileNotFound);
        }

        std::stringstream buffer;
        buffer << file_to_read.rdbuf();

        if (buffer.bad()) {
            return std::unexpected(FileReaderError::BufferError);
        }

        return buffer.str();
    }
};
} // namespace Soldank
