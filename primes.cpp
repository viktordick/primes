#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>

typedef unsigned long int T;
typedef std::chrono::steady_clock _clock;
typedef std::chrono::duration<double> duration;

T sqr(T x) {
    return x*x;
};


const T target = 200000000;

class PrimeCalculator {
    std::vector<T> primes;
    std::vector<bool> candidates;
    // Means we have computed all primes until primes[cur]^2
    T cur;

    std::vector<duration> times{2};

    public:

    PrimeCalculator() {
        this->primes.reserve(2*target);
        this->primes.resize(3);
        this->primes[0] = 3;
        this->primes[1] = 5;
        this->primes[2] = 7;
        this->cur = 0;
    };

    void extend() {
        // candidates are the odd numbers from p_{c}^2 to p_{c+1}^2, excluding.
        // this->candidates contains booleans for all these candidates, which
        // are set to true at first and then switched to false if they are a
        // multiple of a known prime after all.
        const T start = sqr(this->primes[this->cur]) + 2;
        const T new_cur = std::min(this->cur + 100, T(this->primes.size())-1);
        const T end = sqr(this->primes[new_cur]);
        this->candidates.resize((end-start)/2);
        for (T i = 0; i<this->candidates.size(); i++)
            this->candidates[i] = true;

        auto t = _clock::now();
        for (auto p: this->primes) {
            T excluded = p*((start-1)/p+1);  // first multiple of p in range
            // we only check odd numbers
            if (excluded % 2 == 0)
                excluded += p;
            T index = (excluded - start)/2;
            for (; index < this->candidates.size(); index += p) {
                this->candidates[index] = false;
            }
        };
        this->times[0] += (_clock::now() - t);

        t = _clock::now();
        for (T i=0; i<this->candidates.size(); i++)
            if (this->candidates[i]) {
                this->primes.push_back(start+2*i);
            }
        this->times[1] += _clock::now() - t;
        this->cur = new_cur;
    }

    void print() {
        for (int i=0; i<2; i++)
            std::cout << this->times[i].count() << std::endl;
    }
    void extend_until() {
        while (this->primes.size() < target) {
            this->extend();
            std::cout << this->primes.size() << " primes below "
                << sqr(this->primes[this->cur]) << std::endl;
        }
    }

};

int main(int argc, char** argv) {
    PrimeCalculator p;
    p.extend_until();
    p.print();
    return 0;
}
