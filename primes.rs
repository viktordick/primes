use std::time::Instant;

fn sqr(x: u32) -> u64 {
    return (x as u64)*(x as u64);
}

struct Calc {
    p: Vec<u32>,
}

impl Calc {
    fn new() -> Calc {
        let mut vec = Vec::<u32>::with_capacity(1<<28);
        vec.push(3);
        vec.push(5);
        vec.push(7);
        Calc{p: vec}
    }
    fn sieve(&self,
             cur: usize,
             c: &mut Vec<bool>,
             ) -> u64 {
        /* Compute primes between p[cur]^2 and p[cur+1]^2, excluding.
         * Resizes c so it holds a bool for each odd number in that range.
         * Performes the sieve on this range and returns the offset such
         * that c[i] contains whether offset+2*i is prime or not.
         */
        let start = sqr(self.p[cur]) + 2;
        let end = sqr(self.p[cur+1]);

        c.clear();
        c.resize(((end-start)/2) as usize, true);
        for i in 0 ..= cur {
            let p = self.p[i] as u64;
            // First odd multiple of p in range
            let mut first = p*((start-1)/p+1);
            if first % 2 == 0 {
                first += p;
            }
            let mut idx = ((first-start)/2) as usize;
            while idx < c.len() {
                c[idx] = false;
                idx += p as usize;
            }
        }
        start
    }

    fn compute(&mut self, until: u64) {
        /* Compute primes from self.p[0]^2=9 up to `until`*/
        let mut c: Vec<bool> = vec![];
        for cur in 0 ..= until as usize{
            let start = self.sieve(cur, &mut c);
            for i in 0 .. c.len() {
                let p = start + 2*i as u64;
                if p > until {
                    return;
                }
                if c[i] {
                    self.p.push(p as u32);
                }
            }
        }
    }

    fn count(&self, startidx: usize, endidx: usize) -> usize {
        /* Count primes between self.p[startidx]^2 and self.p[endidx]^2.*/
        let target = sqr(self.p[endidx]);
        let mut c: Vec<bool> = vec![];
        let mut result: usize = 0;

        for cur in startidx .. endidx {
            let start = self.sieve(cur, &mut c);

            for i in 0 .. c.len() {
                let p = start + 2*i as u64;
                if p > target {
                    return result;
                }
                if c[i] {
                    result += 1;
                }
            }
        }
        result
    }
}

fn print(start: Instant, value: usize) {
    let elapsed = start.elapsed();
    println!("{}.{} {}", elapsed.as_secs(), elapsed.subsec_millis(), value);
}

fn main() {
    let start = Instant::now();

    let mut calc = Calc::new();
    // 65521 is the largest prime below 2^16, so this computes all primes
    // until 2^32.
    calc.compute(sqr(65521));
    print(start, calc.p.len());
    // The index of 65521
    let mut cur: usize = 6539;
    for _i in 0..5 {
        let tgt = cur + 1024;
        print(start, calc.count(cur, tgt));
        cur = tgt;
    }
}
