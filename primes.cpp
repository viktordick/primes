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
typedef std::chrono::duration<double> duration;

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
            << std::hex << this->myidx;
        return s.str();
    }

    public:

    Batch(int _myidx, std::vector<T> *_data)
        :data(_data)
         ,myidx(_myidx)
    {
    };

    bool is_loaded() const {
        return this->data != nullptr;
    };

    size_t size() const {
        return this->data->size();
    };

    T& operator[](size_t idx) {
        return (*(this->data))[idx];
    }

    void push_back(T value) {
        this->data->push_back(value);
    }

    void load(std::vector<T> *data) {
        std::cout << "load " << this->myidx << std::endl;
        if (this->data != nullptr) {
            throw;
        };
        if (data == nullptr)
            throw;
        this->data = data;
        std::ifstream f(this->path());
        data->resize(BATCHSIZE);
        f.read((char*)&((*data)[0]), BATCHSIZE * sizeof(T));
        f.close();
    };

    std::vector<T>* unload() {
        std::cout << "unload " << this->myidx << std::endl;
        if (this->data == nullptr) {
            throw;
        }
        std::vector<T>* result = this->data;
        this->data = nullptr;
        return result;
    };

    void store() {
        std::ofstream f(this->path(), std::ios::binary);
        if (this->data == nullptr)
            throw;
        f.write((const char*)&((*(this->data))[0]), this->data->size() * sizeof(T));
        f.close();
    };
};

class PrimeStorage {
    std::vector<Batch> primes;
    std::vector< std::vector<T> > pool;
    size_t count;

    public:

    PrimeStorage()
        :pool(16), count(0)
    {
    }

    size_t size() const {
        return this->count;
    };
    T& operator[](size_t idx) {
        const auto s = BATCHSIZE;
        auto &P = this->primes;
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
        auto &P = this->primes;
        if (this->count % BATCHSIZE == 0) {
            if (!P.empty())
                P[P.size()-1].store();
            std::vector<T> *storage = nullptr;
            if (P.size() < this->pool.size()) {
                storage = &(this->pool[P.size()]);
                storage->reserve(BATCHSIZE);
            } else {
                storage = P[P.size()-1].unload();
            }
            P.push_back(Batch(P.size(), storage));
        }
        P[P.size()-1].push_back(value);
        this->count++;
    }

    size_t batches() {
        return this->primes.size();
    }
};

class PrimeCalculator {
    PrimeStorage primes;
    std::vector<bool> candidates;
    // Means we have computed all primes until primes[cur]^2
    size_t cur;

    std::vector<duration> times{2};

    public:

    PrimeCalculator() {
        auto &P = this->primes;
        P.append(3);
        P.append(5);
        P.append(7);
        this->cur = 0;
    };

    void extend() {
        // candidates are the odd numbers from p_{c}^2 to p_{c+1}^2, excluding.
        // this->candidates contains booleans for all these candidates, which
        // are set to true at first and then switched to false if they are a
        // multiple of a known prime after all.
        auto &P = this->primes;
        auto &B = this->candidates;

        const T start = sqr(P[this->cur]) + 2;
        const T new_cur = std::min(this->cur + 100, P.size()-1);
        const T end = sqr(P[new_cur]);
        B.resize((end-start)/2);
        for (size_t i = 0; i<B.size(); i++)
            B[i] = true;

        auto t = _clock::now();
        for (size_t i=0; i<P.size(); i++) {
            const auto p = P[i];
            T first = p*((start-1)/p+1);  // first multiple of p in range
            // we only check odd numbers
            if (first % 2 == 0)
                first += p;
            for (size_t idx = (first-start)/2; idx < B.size(); idx += p) {
                B[idx] = false;
            }
        };
        this->times[0] += (_clock::now() - t);

        t = _clock::now();
        for (T i=0; i<B.size(); i++)
            if (B[i]) {
                P.append(start+2*i);
            }
        this->times[1] += _clock::now() - t;
        this->cur = new_cur;
    }

    void print() {
        for (int i=0; i<2; i++)
            std::cout << this->times[i].count() << std::endl;
        /*
        for (size_t i=0; i<this->primes.size(); i++)
            std::cout << this->primes[i] << std::endl;
            */
    }
    void extend_until() {
        while (true) {
            this->extend();
            std::cout
                << this->primes.size() << " primes below "
                << sqr(this->primes[this->cur])
                << ", " << this->primes.batches() << " batches"
                << std::endl;
        }
    }

};

int main(int argc, char** argv) {
    std::cout.imbue(std::locale(""));
    PrimeCalculator p;
    p.extend_until();
    p.print();
    return 0;
}
