module;

#include <filesystem>
#include <fstream>
#include <ios>
#include <sstream>

export module Shared.Core.Data.FileWriter;

import Shared.Core.Data.IFileWriter;

export namespace Soldank
{
class FileWriter : public IFileWriter
{
public:
    FileWriterError AppendData(const char* data, std::streamsize data_size) override
    {
        buffer_.write(data, data_size);
        if (buffer_.bad()) {
            return FileWriterError::BufferError;
        }

        return FileWriterError::NoError;
    }

    FileWriterError Write(const std::filesystem::path& file_path,
                          std::ios_base::openmode mode = std::ios_base::out) const override
    {
        std::ofstream file(file_path, mode);
        if (!file.is_open()) {
            return FileWriterError::FileNotOpen;
        }

        file.clear();
        if (file.bad()) {
            return FileWriterError::BufferError;
        }

        file << buffer_.rdbuf();
        if (file.bad()) {
            return FileWriterError::BufferError;
        }

        file.close();

        return FileWriterError::NoError;
    }

private:
    std::stringstream buffer_;
};
} // namespace Soldank
