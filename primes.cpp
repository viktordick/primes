#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <locale>
#include <iomanip>

#define ARCHIVE 0

typedef unsigned long int T;
typedef std::chrono::steady_clock _clock;

auto starttime = _clock::now();

T sqr(T x) {
    return x*x;
};

class PrimeCalculator {
    std::vector<uint32_t> P = {3,5,7};
    uint64_t target = 8;
    int ln2target = 3;
    // candidates are the odd numbers from p_{c}^2 to p_{c+1}^2, excluding.
    // C contains booleans for all these candidates, which are set to true at
    // first and then switched to false if they are a multiple of a known prime
    // after all.
    std::vector<bool> C;
    // Means we have computed all primes until primes[cur]^2
    size_t cur = 0;

    size_t count = 4;
    std::vector<uint8_t> A;
    T archive_prefix = 0;

    public:

    PrimeCalculator() {
        P.reserve(16*1024*1024);
#if ARCHIVE
        A.resize(T(1) << 29, 0);
        for (auto p: P)
            archive(p);
#endif
    };

    void archive(T value) {
        // drop last bit, all our primes are odd
        value >>= 1;
        // The highest 4 bytes are the prefix, for each prefix we start a
        // separate file. Each file is a bitfield of the set values.
        const T prefix = (value >> 32);

        if (prefix != archive_prefix) {
            std::stringstream s;
            s << "archive/" << std::setfill('0') << std::setw(8)
                << std::hex << archive_prefix;
            std::ofstream f(s.str(), std::ios::binary);
            f.write((const char*)&A[0], A.size());
            if (f.fail())
                throw;
            f.close();

            for (auto &a: A)
                a = 0;
            archive_prefix = prefix;
        }
        A[(value & 0xffffffff) >> 3] |= (1 << (7-(value & 7)));
    }

    // Compute more primes. Assuming we have computed all primes until p_{n}^2,
    // where p_{n} is the nth prime, this computes all primes until p_{n+1}^2.
    // The prime is archived and, if it is small enouth that it might be
    // relevant for further computations, stored in memory.
    void extend() {
        const size_t new_cur = cur + 1;
        if (new_cur == P.size()) {
            std::cerr << "Capacity reached." << std::endl;
            throw;
        };
        const T start = sqr(T(P[cur])) + 2;
        const T end = sqr(T(P[new_cur]));
        C.resize((end-start)/2);
        for (size_t i = 0; i<C.size(); i++)
            C[i] = true;

        for (size_t i=0; i<new_cur; i++) {
            const auto p = T(P[i]);
            T first = p*((start-1)/p+1);  // first multiple of p in range
            // we only check odd numbers
            if (first % 2 == 0)
                first += p;
            for (size_t idx = (first-start)/2; idx < C.size(); idx += p) {
                C[idx] = false;
            }
        };

        for (T i=0; i<C.size(); i++)
            if (C[i]) {
                const auto p = start + 2*i;
                if (P.size() < P.capacity() && uint32_t(p) == p)
                    P.push_back(p);
                if (p > target) {
                    std::cout
                        << std::fixed << std::chrono::duration<double>(_clock::now() - starttime).count() << "s: "
                        << "π(2^" << std::setw(11) << ln2target << ") = "
                        << "π(" << std::setw(12) << target << ") = "
                        << count << std::endl;
                    ln2target++;
                    target *= 2;
                }
#if ARCHIVE
                archive(p);
#endif
                count++;
            }
        cur = new_cur;
        if (cur % 1000 == 0) {
            std::cout
                << std::fixed << std::chrono::duration<double>(_clock::now() - starttime).count() << "s: "
                << "π(" << std::setw(12) << P[cur] << "²) = "
                << "π(" << std::setw(12) << sqr(P[cur]) << ") = "
                << count
                << std::endl;
        }
    }

};

int main(int argc, char** argv) {
    std::cout.imbue(std::locale(""));
    PrimeCalculator p;
    while (true)
        p.extend();
    return 0;
}
