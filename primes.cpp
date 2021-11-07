#include <stdlib.h>
#include <math.h>

#include <cmath>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <locale>
#include <iomanip>

typedef uint64_t T;
typedef std::chrono::steady_clock _clock;

auto starttime = _clock::now();

T sqr(T x) {
    return x*x;
};

class PrimeCalculator {

    public:
    std::vector<uint32_t> P = {3,5,7};
    // If count is called with store=true, the index of the largest prime lower
    // than tgt is additionally stored here.
    size_t stored_cur;

    PrimeCalculator() {
        P.reserve(1 << 28);
    };

    // Count primes between P_{startidx}^2 and target. If store is set, the
    // computed primes are appended to P.
    template<bool store>
    size_t count(size_t startidx, T target) {
        // Candidates are the odd numbers from P_{c}^2 to P_{c+1}^2, excluding.
        // C contains booleans for all these candidates, which are set to true at
        // first and then switched to false if they are a multiple of a known prime
        // after all.
        std::vector<bool> C;
        size_t result = 0;
        const size_t tgtsqrt = std::ceil(sqrt(target));
        for (size_t cur=startidx; P[cur] < tgtsqrt; cur++) {
            if (store)
                stored_cur = cur;
            if (cur+1 == P.size()) {
                std::cerr << "Capacity reached." << std::endl;
                throw;
            };
            const T start = sqr(T(P[cur])) + 2;
            const T end = sqr(T(P[cur+1]));
            C.resize((end-start)/2);
            for (size_t i = 0; i<C.size(); i++)
                C[i] = true;

            for (size_t i=0; i<cur+1; i++) {
                const auto p = T(P[i]);
                T first = p*((start-1)/p+1);  // first multiple of p in range
                // we only check odd numbers
                if (first % 2 == 0)
                    first += p;
                for (size_t idx = (first-start)/2; idx < C.size(); idx += p) {
                    C[idx] = false;
                }
            };

            for (T i=0; i<C.size(); i++) {
                const auto p = start + 2*i;
                if (p > target) {
                    return result;
                }
                if (C[i]) {
                    if (store && uint32_t(p) == p)
                        P.push_back(p);
                    result++;
                }
            }
        }
        return result;
    }

    void print(size_t startidx, size_t endidx, T result, T end=0) const {
        if (endidx)
            end = P[endidx];

        std::cout
            << std::fixed << std::chrono::duration<double>(_clock::now() - starttime).count() << "s: "
            << "π(" << std::setw(12);
        if (endidx)
            std::cout << P[endidx] << "²";
        else
            std::cout << end;
        std::cout << ") - "
            << "π(" << std::setw(12) << P[startidx] << "²) = "
            << result
            << std::endl;
    }

    void archive() const {
        std::ofstream f("generators.dat", std::ios::binary);
        f.write((const char*)&P[0], 4*P.size());
    }
    void load() {
        std::ifstream f("generators.dat", std::ios::binary);
        P.resize(203191691);
        f.read((char*)&P[0], 4*P.size());
    }
};

int main(int argc, char** argv) {
    std::cout.imbue(std::locale(""));
    PrimeCalculator p;
    auto &P = p.P;
    size_t cur = 6539;
    if (argc == 1) {
        // 65521 is the largest prime below 2^16
        size_t count = p.count<true>(0, sqr(65521));
        p.print(0, p.stored_cur, count);
        p.archive();
    } else {
        p.load();
        const size_t start = atoll(argv[1]);

        // Move index forward to a point that would also have been part of the
        // default run, but right before the forced starting point.
        while (sqr(P[cur+1024]) < start)
            cur += 1024;
        if (sqr(P[cur]) > start)
            cur = 0;
        auto count = p.count<false>(cur, start);
        p.print(cur, 0, count, start);
        if (sqr(P[cur]) > start) {
            cur = 6539;
            p.print(0, cur, 203191688);
        }
    }

    #pragma omp parallel
    {
        while (true) {
            size_t local_cur;
            size_t tgt;
            #pragma omp critical
            {
                local_cur = cur;
                tgt = cur + 1024;
                cur = tgt;
            }
            size_t count = p.count<false>(local_cur, sqr(p.P[tgt]));
            #pragma omp critical
            p.print(local_cur, tgt, count);
        }
    }
    return 0;
}
