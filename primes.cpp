#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <locale>
#include <string>
#include <sstream>
#include <iomanip>

typedef unsigned long int T;
typedef std::chrono::steady_clock _clock;

auto starttime = _clock::now();

const size_t BATCHSIZE = 16*1024*1024;

T sqr(T x) {
    return x*x;
};

class Batch {
    std::vector<T> *data;
    const int myidx;

    std::string path() const {
        std::stringstream s;
        s << "result/" << std::setfill('0') << std::setw(8)
            << std::hex << myidx;
        return s.str();
    }

    public:

    Batch(int _myidx, std::vector<T> *_data)
        :data(_data)
         ,myidx(_myidx)
    {
    };

    bool is_loaded() const {
        return data != nullptr;
    };

    size_t size() const {
        return data->size();
    };

    T& operator[](size_t idx) {
        return (*data)[idx];
    }

    void push_back(T value) {
        data->push_back(value);
    }

    void load(std::vector<T> *data) {
        if (this->data != nullptr) {
            throw;
        };
        if (data == nullptr)
            throw;
        this->data = data;
        std::ifstream f(path());
        data->resize(BATCHSIZE);
        f.read((char*)&((*data)[0]), BATCHSIZE * sizeof(T));
        if (f.fail())
            throw;
        f.close();
    };

    std::vector<T>* unload() {
        if (data == nullptr) {
            throw;
        }
        std::vector<T>* result = data;
        data = nullptr;
        return result;
    };

    void store() {
        std::ofstream f(path(), std::ios::binary | std::ios::trunc);
        if (data == nullptr)
            throw;
        f.write((const char*)&((*data)[0]), data->size() * sizeof(T));
        f.close();
    };
};

class PrimeStorage {
    std::vector<Batch> P;
    std::vector< std::vector<T> > pool;
    size_t count;

    public:

    PrimeStorage()
        :pool(2), count(0)
    {
    }

    size_t size() const {
        return count;
    };

    T& operator[](size_t idx) {
        const auto s = BATCHSIZE;
        const int i = idx/s;
        const int j = idx%s;
        if (!P[i].is_loaded()) {
            for (int k=i-1; k!=i; k--) {
                if (k == -1)
                    k = P.size()-2;
                if (k == i)
                    throw;
                if (P[k].is_loaded()) {
                    P[i].load(P[k].unload());
                    break;
                }
            }
        }
        return P[i][j];
    };
    void append(T value) {
        if (count % BATCHSIZE == 0) {
            if (!P.empty())
                P[P.size()-1].store();
            std::vector<T> *storage = nullptr;
            if (P.size() < pool.size()) {
                storage = &(pool[P.size()]);
                storage->reserve(BATCHSIZE);
            } else {
                storage = P[P.size()-1].unload();
                storage->resize(0);
            }
            P.push_back(Batch(P.size(), storage));
        }
        P[P.size()-1].push_back(value);
        count++;
    }

    size_t batches() {
        return P.size();
    }
};

class PrimeCalculator {
    PrimeStorage P;
    std::vector<bool> candidates;
    // Means we have computed all primes until primes[cur]^2
    size_t cur;

    public:

    PrimeCalculator() {
        P.append(3);
        P.append(5);
        P.append(7);
        cur = 0;
    };

    void extend() {
        // candidates are the odd numbers from p_{c}^2 to p_{c+1}^2, excluding.
        // candidates contains booleans for all these candidates, which
        // are set to true at first and then switched to false if they are a
        // multiple of a known prime after all.
        auto &B = candidates;

        const size_t new_cur = cur + 1; // std::min(cur + 128, P.size()-1);
        if (P[new_cur] > (1 << 31)) {
            std::cerr << "Overflow" << std::endl;
            throw;
        }
        const T start = sqr(P[cur]) + 2;
        const T end = sqr(P[new_cur]);
        const size_t workmem = (end-start) >> 24;
        B.resize((end-start)/2);
        for (size_t i = 0; i<B.size(); i++)
            B[i] = true;

        for (size_t i=0; i<new_cur; i++) {
            const auto p = P[i];
            T first = p*((start-1)/p+1);  // first multiple of p in range
            // we only check odd numbers
            if (first % 2 == 0)
                first += p;
            for (size_t idx = (first-start)/2; idx < B.size(); idx += p) {
                B[idx] = false;
            }
        };

        for (T i=0; i<B.size(); i++)
            if (B[i]) {
                P.append(start+2*i);
            }
        cur = new_cur;
        if (cur % 100 == 0)
            std::cout
                << std::fixed << std::chrono::duration<double>(_clock::now() - starttime).count() << "s "
                << "π(" << P[cur] << "²) = " << (1+P.size())
                << ", " << workmem << "MB workmem"
                << std::endl;
    }

};

int main(int argc, char** argv) {
    std::cout.imbue(std::locale(""));
    PrimeCalculator p;
    while (true)
        p.extend();
    return 0;
}
