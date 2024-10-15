#ifndef __IFILE_WRITER_HPP__
#define __IFILE_WRITER_HPP__

#include <expected>
#include <ios>
#include <filesystem>

namespace Soldank
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

#endif
