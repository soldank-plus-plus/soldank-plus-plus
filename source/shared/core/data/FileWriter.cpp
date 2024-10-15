#include "core/data/FileWriter.hpp"

#include <fstream>

namespace Soldank
{
FileWriterError FileWriter::AppendData(const char* data, std::streamsize data_size)
{
    buffer_.write(data, data_size);
    if (buffer_.bad()) {
        return FileWriterError::BufferError;
    }

    return FileWriterError::NoError;
}

FileWriterError FileWriter::Write(const std::filesystem::path& file_path,
                                  std::ios_base::openmode mode) const
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
} // namespace Soldank
