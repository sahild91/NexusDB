#ifndef NEXUSDB_COMPRESSION_H
#define NEXUSDB_COMPRESSION_H

#include <vector>
#include <cstdint>

namespace nexusdb {

class Compression {
public:
    static std::vector<uint8_t> compress_rle(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> decompress_rle(const std::vector<uint8_t>& compressed_data);
};

} // namespace nexusdb

#endif // NEXUSDB_COMPRESSION_H