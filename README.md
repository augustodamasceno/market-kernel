<div align="center">

# **MarketKernel**

### *A C++ base library for building trading indicators, strategies, and bots powered by neural networks, fuzzy logic, and genetic algorithms.*

<p>
  <a href="#"><img alt="License" src="https://img.shields.io/badge/License-BSD_2--Clause-orange.svg"></a>
</p>

</div>

---

## **Overview**

MarketKernel provides a robust infrastructure for handling market data with modern C++ features.  
It supports market data and trading protocol integrations for **B3 SBE** (**UMDF** and **FIX**).

It is intended to be used as a base library for:

- **Indicators**: Quantitative indicators and signal generation
- **Strategies**: Trading strategy development and backtesting
- **Bots**: Automated trading systems and algorithmic trading
- **AI**: Neural networks, fuzzy logic systems, and genetic algorithms applied to trading

---

## **Documentation:** 

### **Online:** [https://augustodamasceno.org/market-kernel/](https://augustodamasceno.org/market-kernel/)
### **Offline:** [docs/index.md](docs/index.md)  
### **SBE Codecs Generation:** [include/schemas/README.md](include/schemas/README.md)
### **Network IP Configuration and Market Data Feed Endpoints:** [include/mk_ips.hpp](include/mk_ips.hpp)

---

## **Project Structure**

The library is organized into **core modules** and **optional engines**:

```
include/           — Core Headers
  └── schemas/     ── Protocol codecs auto-generated from exchange schemas
      └── b3/      ─── B3 SBE/UMDF C++ codecs generated with SBE Tool from Aeron
                   ─── B3 UMDF XML schema
src/               — Core Implementations
tests/             — Core Tests
engines/           — Third-party integrations and specialized computational engines
  ├── neural/      ── Neural networks via LibTorch
  ├── genetics/    ── Genetic algorithms via PyGAD
  └── fuzzy/       ── Fuzzy logic inference
examples/          — Example code and usage demonstrations
  ├── core/     
  └── engines/
```

**Design principle:** The core library is dependency-free and fully functional standalone. AI/ML frameworks, external computational engines, and specialized third-party integrations are isolated in `engines/` and can be optionally enabled at build time.

---

## AI Components

AI/ML frameworks are integrated as optional **engines** that can be selectively enabled at build time.

### Neural Networks (`engines/neural/`)
[LibTorch](https://pytorch.org/cppdocs/) (PyTorch C++ API) is used for neural network training and inference directly in C++.

### Genetic Algorithms (`engines/genetics/`)
Genetic algorithm optimisation is handled via a C++/Python interface with [PyGAD](https://pygad.readthedocs.io/). Strategy parameters and fitness evaluation run on the C++ side; PyGAD drives the evolutionary loop over the Python bridge.

### Fuzzy Logic (`engines/fuzzy/`)
Fuzzy logic support is planned for a future version.

---


## **Contact**

* **Email:** [augustodamasceno@protonmail.com](mailto:augustodamasceno@protonmail.com)

---

## **License**

This project is licensed under BSD 2-Clause License.

<small>Copyright &copy; 2026, Augusto Damasceno. All rights reserved.</small>

---
