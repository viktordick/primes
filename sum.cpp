#include <iostream>
#include <fstream>
#include <vector>
#include <locale>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <bitset>

class Counter {
    std::vector<size_t> cached_counts;
    std::vector<uint64_t> buffer;
    int currently_cached = -1;

    void readfile(int idx) {
        if (idx == currently_cached)
            return;
        buffer.resize(uint64_t(1) << 26);
        std::stringstream s;
        s << "archive/" << std::setfill('0') << std::setw(8)
            << std::hex << idx;
        std::ifstream f(s.str(), std::ios::binary);
        f.read((char*)&buffer[0], buffer.size()*8);
        if (f.fail()) {
            std::cerr << "File not found or too small." << std::endl;
            exit(1);
        }
        f.close();
        currently_cached = idx;
        std::cerr << s.str() << std::endl;
    }

    public:

    // Read out how many primes are below ceil
    size_t sum(uint64_t ceil) {
        // Calculate the position of the first bit representing a number larger
        // than ceil - index of the file, position within it and number of the
        // bit.

        // We only store odd numbers, so drop last bit after adding 1
        // 0th bit is 1, 1st is 3, 2nd is 5 ...
        auto fullpos = (ceil + 1) >> 1;

        // The highest 4 bytes are the prefix, for each prefix we start a
        // separate file. Each file is a bitfield of the set values.
        const auto fileidx = (fullpos >> 32);
        // position in buffer (least sign. 4 bytes, divided by 64 to convert
        // from bit to uint64_t)
        const uint32_t pos = (fullpos & 0xffffffff) >> 6;
        // position inside the last 8-byte-block
        const uint8_t bit = (fullpos & 63);

        size_t result = (ceil >= 2)?1:0;
        // count up full files as needed
        for (uint32_t idx=0; idx<fileidx; idx++) {
            if (idx == cached_counts.size()) {
                readfile(idx);
                size_t cnt = 0;
                for (auto x: buffer)
                    cnt += std::bitset<64>(x).count();
                cached_counts.push_back(cnt);
            }
            result += cached_counts[idx];
        }

        // early exit if we do not need to partially evaluate.
        if (pos == 0 && bit == 0)
            return result;

        readfile(fileidx);
        for (uint32_t idx=0; idx<pos; idx++)
            result += std::bitset<64>(buffer[idx]).count();

        // sum the remaining bits, which are in the middle of an 8-byte-block
        if (bit == 0)
            return result;
        uint8_t *remainder = (uint8_t*)&buffer[pos];
        for (uint8_t idx=0; idx<bit; idx++)
            result += (remainder[idx/8] >> (7-idx%8)) & 1;
        return result;
    }
};

int main(int argc, char** argv) {
    std::cout.imbue(std::locale(""));
    Counter c;
    uint64_t target = 2;
    while (true) {
        auto result = c.sum(target);
        std::cout
            << std::setw(20) << target
            << std::setw(20) << result
            << std::endl;
        target *= 2;
    }
}
