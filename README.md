# TuringsNightmare (TN)
An experimental PoW algorithm developed by IPBC
Intended to be ASIC resistant and to favor CPU over GPU

Idea based on [Core War](https://en.wikipedia.org/wiki/Core_War).

## Concept
Explanation can be found [here](https://github.com/ipbc-dev/TuringsNightmare/blob/master/TN_Explanation.md).

## Results
Here're some preliminary results running 10 instances in parellel on Intel Core i7-7700K and NVIDIA Geforce GTX 1080 Ti.

With no difference in the data between instances (non-divergent, all instances take the same branches), CPU is ~30 times faster.  
With only one byte difference in the data (divergent) the thread divergence just kills GPU performance while the CPU suffers no performance degradation.
```
Sanity checking CUDA... Sane
Sanity checking OpenCL... Sane

Running speed tests (no divergence)

CPU running 1 instances took 58ms
CUDA running 1 instances took 3240ms
OpenCL running 1 instances took 3208ms

CPU running 5 instances took 75ms
CUDA running 5 instances took 3334ms
OpenCL running 5 instances took 3344ms

CPU running 10 instances took 142ms
CUDA running 10 instances took 3435ms
OpenCL running 10 instances took 3418ms

CPU running 20 instances took 218ms
CUDA running 20 instances took 3757ms
OpenCL running 20 instances took 3680ms

Running speed tests (divergent)

CPU running 1 instances took 59ms
CUDA running 1 instances took 3262ms
OpenCL running 1 instances took 3227ms

CPU running 5 instances took 76ms
CUDA running 5 instances took 9669ms
OpenCL running 5 instances took 9619ms

CPU running 10 instances took 143ms
CUDA running 10 instances took 14671ms
OpenCL running 10 instances took 14839ms

CPU running 20 instances took 217ms
CUDA running 20 instances took 19212ms
OpenCL running 20 instances took 19290ms
```