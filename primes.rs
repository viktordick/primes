use std::time::Instant;
use crossbeam::{channel::bounded,thread::scope};
use bitvec::prelude::*;

fn sqr(x: u32) -> u64 {
    return (x as u64)*(x as u64);
}

struct Calc {
    p: Vec<u32>,
    start_time: Instant,
}

impl Calc {
    fn new() -> Calc {
        let mut vec = Vec::<u32>::with_capacity(1<<28);
        vec.push(3);
        vec.push(5);
        vec.push(7);
        Calc{p: vec, start_time: Instant::now()}
    }
    /* Compute primes between p[cur]^2 and p[cur+1]^2, excluding. Resizes c so it holds a bool for
     * each odd number in that range. Performes the sieve on this range and returns the offset
     * such that c[i] contains whether offset+2*i is prime or not.
     */
    fn sieve(&self, cur: usize, c: &mut BitVec) -> u64 {
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
                let mut bit = c.get_mut(idx).unwrap();
                *bit = false;
                idx += p as usize;
            }
        }
        start
    }

    /* Compute generating primes between 9 and self.p[endidx]^2 and fill self.p with them.*/
    fn compute(&mut self, endidx: usize) {
        let mut c: BitVec = BitVec::new();
        for cur in 0 ..= endidx {
            let start = self.sieve(cur, &mut c) as usize;
            for i in 0 .. c.len() {
                if c[i] {
                    self.p.push((start+2*i) as u32);
                }
            }
        }
    }

    /* Count primes between self.p[startidx]^2 and self.p[endidx]^2.*/
    fn count(&self, startidx: usize, endidx: usize) -> usize {
        let mut c: BitVec = BitVec::new();
        let mut result: usize = 0;

        for cur in startidx .. endidx {
            self.sieve(cur, &mut c);
            result += c.count_ones();
        }
        result
    }

    fn print(&self, startidx: usize, endidx: usize, result: usize) {
        let elapsed = self.start_time.elapsed();
        println!(
            "{:>5}.{:03}s: π({:>12}²) - π({:>12}²) = {}",
            elapsed.as_secs(), elapsed.subsec_millis(),
            self.p[endidx],
            self.p[startidx],
            result,
        );
    }
}

fn main() {
    let mut calc = Calc::new();
    let mut cur: usize = 6540;
    calc.compute(cur);
    calc.print(0, cur, calc.p.len());

    let (sender, receiver) = bounded(0);

    scope(|s| {
        for _ in 0..8 {
            s.spawn(|_| {
                let receiver = receiver.clone();
                for (cur,nxt) in receiver {
                    let count = calc.count(cur, nxt);
                    calc.print(cur, nxt, count);
                }
            });
        }

        loop {
            let tgt = cur+1024;
            sender.send((cur,tgt)).unwrap();
            cur = tgt;
        }
    }).unwrap();
}
