# myBudget

## Overview

*myBudget* is a software designed to keep track of your budgets.
It is intended for people who want to better manage their expenses and incomes.

## Features

- Intuitive cli
- Create custom named wallet/category
- Transfer between wallets
- Export to CSV

## Supported Platforms

- GNU/Linux
- MacOS
- Windows (experimental)

## Dependencies

- C compiler (gcc/clang)
- make
- cmake

## Build

### Debug

```
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Debug ..
$ make
```

### Release

```
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
```
