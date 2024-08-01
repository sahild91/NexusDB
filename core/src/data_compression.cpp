#include "nexusdb/data_compression.h"

namespace nexusdb {

std::vector<uint8_t> Compression::compress_rle(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> compressed;
    for (size_t i = 0; i < data.size(); ) {
        uint8_t count = 1;
        while (count < 255 && i + count < data.size() && data[i] == data[i + count]) {
            count++;
        }
        compressed.push_back(count);
        compressed.push_back(data[i]);
        i += count;
    }
    return compressed;
}

std::vector<uint8_t> Compression::decompress_rle(const std::vector<uint8_t>& compressed_data) {
    std::vector<uint8_t> decompressed;
    for (size_t i = 0; i < compressed_data.size(); i += 2) {
        uint8_t count = compressed_data[i];
        uint8_t value = compressed_data[i + 1];
        decompressed.insert(decompressed.end(), count, value);
    }
    return decompressed;
}

} // namespace nexusdb