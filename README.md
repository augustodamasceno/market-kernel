# MarketKernel

A C++ base library for building trading indicators, strategies, and bots powered by neural networks, fuzzy logic, and genetic algorithms.

## Overview

MarketKernel provides a robust infrastructure for handling market data with modern C++ features. It is intended to be used as a base library for:

- **Indicators**: Quantitative indicators and signal generation
- **Strategies**: Trading strategy development and backtesting
- **Bots**: Automated trading systems and algorithmic trading
- **AI**: Neural networks, fuzzy logic systems, and genetic algorithms applied to trading

## AI Components

### Neural Networks
[LibTorch](https://pytorch.org/cppdocs/) (PyTorch C++ API) is used for neural network training and inference directly in C++.

### Genetic Algorithms
Genetic algorithm optimisation is handled via a C++/Python interface with [PyGAD](https://pygad.readthedocs.io/). Strategy parameters and fitness evaluation run on the C++ side; PyGAD drives the evolutionary loop over the Python bridge.

### Fuzzy Logic
Fuzzy logic support is planned for a future version.
