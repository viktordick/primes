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

    // Count primes between P_{cur}^2 and tgt^2. If store is set, the computed
    // primes are appended to P. 
    template<bool store>
    size_t count(size_t startidx, T tgt) {
        // Candidates are the odd numbers from P_{c}^2 to P_{c+1}^2, excluding.
        // C contains booleans for all these candidates, which are set to true at
        // first and then switched to false if they are a multiple of a known prime
        // after all.
        std::vector<bool> C;
        size_t result = 0;
        const size_t tgtsqr = sqr(tgt);
        for (size_t cur=startidx; P[cur] < tgt; cur++) {
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
                if (p > tgtsqr) {
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

    void print(size_t startidx, size_t endidx, size_t result) const {
        std::cout
            << std::fixed << std::chrono::duration<double>(_clock::now() - starttime).count() << "s: "
            << "π(" << std::setw(12) << P[endidx] << "²) - "
            << "π(" << std::setw(12) << P[startidx] << "²) = "
            << result
            << std::endl;
    }

};

int main(int argc, char** argv) {
    std::cout.imbue(std::locale(""));
    PrimeCalculator p;
    // 65521 is the largest prime below 2^16
    size_t count = p.count<true>(0, 65521);
    p.print(0, p.stored_cur, count);
    size_t cur = p.stored_cur;
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
            size_t count = p.count<false>(local_cur, p.P[tgt]);
#pragma omp critical
            p.print(local_cur, tgt, count);
        }
    }
    return 0;
}
