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

#include <boost/dynamic_bitset.hpp>

typedef uint64_t T;
typedef std::chrono::steady_clock _clock;

auto starttime = _clock::now();

T sqr(T x) {
    return x*x;
};

class PrimeCalculator {

    public:
    std::vector<uint32_t> P = {3,5,7};

    PrimeCalculator() {
        P.reserve(1 << 28);
    };

    /* Compute primes between p[cur]^2 and p[cur+1]^2, excluding. Resizes c so
     * it holds a bool for each odd number in that range. Performs the sieve
     * on this range and returns the offset such that c[i] contains whether
     * offset+2*i is prime or not.
     */
    size_t sieve(size_t cur, boost::dynamic_bitset<> &c) const {
        const T start = sqr(T(P[cur])) + 2;
        const T end = sqr(T(P[cur+1]));
        c.clear();
        c.resize((end-start)/2, true);

        for (size_t i=0; i<=cur; i++) {
            const auto p = T(P[i]);
            T first = p*((start-1)/p+1);  // first multiple of p in range
            // we only check odd numbers
            if (first % 2 == 0)
                first += p;
            for (size_t idx = (first-start)/2; idx < c.size(); idx += p) {
                c[idx] = false;
            }
        };
        return start;
    }

    /* Compute generating primes between 9 and P[endidx]^2 and fill P with
     * them.*/
    void compute(size_t endidx) {
        boost::dynamic_bitset<> c;
        for (size_t cur = 0; cur<endidx; cur++) {
            auto start = sieve(cur, c);
            for (size_t i = 0; i<c.size(); i++) {
                if (c[i])
                    P.push_back(start+2*i);
            }
        }
    }

    /* Count primes between P[startidx]^2 and P[endidx]^2*/
    size_t count(size_t startidx, size_t endidx) const {
        size_t result = 0;
        boost::dynamic_bitset<> c;
        for (size_t cur = startidx; cur<endidx; cur++) {
            sieve(cur, c);
            result += c.count();
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
};

int main(int argc, char** argv) {
    std::cout.imbue(std::locale(""));
    PrimeCalculator p;
    auto &P = p.P;
    // 65521=P[6540] is the largest prime below 2^16
    size_t cur = 6540;
    p.compute(cur);
    p.print(0, cur, P.size()-3);

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
            size_t count = p.count(local_cur, tgt);
            #pragma omp critical
            p.print(local_cur, tgt, count);
        }
    }
    return 0;
}
