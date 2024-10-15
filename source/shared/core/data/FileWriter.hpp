#ifndef __FILE_WRITER_HPP__
#define __FILE_WRITER_HPP__

#include "core/data/IFileWriter.hpp"

#include <sstream>

namespace Soldank
{
class FileWriter : public IFileWriter
{
public:
    FileWriterError AppendData(const char* data, std::streamsize data_size) override;
    FileWriterError Write(const std::filesystem::path& file_path,
                          std::ios_base::openmode mode = std::ios_base::out) const override;

private:
    std::stringstream buffer_;
};
} // namespace Soldank

#endif
