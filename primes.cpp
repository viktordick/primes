#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <locale>
#include <iomanip>

typedef unsigned long int T;
typedef std::chrono::steady_clock _clock;

auto starttime = _clock::now();

T sqr(T x) {
    return x*x;
};

class PrimeCalculator {
    std::vector<uint32_t> P;
    // candidates are the odd numbers from p_{c}^2 to p_{c+1}^2, excluding.
    // C contains booleans for all these candidates, which are set to true at
    // first and then switched to false if they are a multiple of a known prime
    // after all.
    std::vector<bool> C;
    // Means we have computed all primes until primes[cur]^2
    size_t cur;

    size_t count;

    public:

    PrimeCalculator() {
        P.reserve(16*1024*1024);
        P.push_back(3);
        P.push_back(5);
        P.push_back(7);
        cur = 0;
        count = 4;
    };

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
                count++;
                auto p = start + 2*i;
                if (P.size() < P.capacity() && uint32_t(p) == p)
                    P.push_back(start+2*i);
            }
        cur = new_cur;
        if (cur % 1000 == 0) {
            std::cout
                << std::fixed << std::chrono::duration<double>(_clock::now() - starttime).count() << "s: "
                << "π(" << P[cur] << "²) = " 
                << "π(" << sqr(P[cur]) << ") = " 
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
