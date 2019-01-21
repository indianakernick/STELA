//
//  language benchmark.cpp
//  STELA
//
//  Created by Indi Kernick on 20/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include <iostream>
#include <unordered_set>
#include <STELA/llvm.hpp>
#include <STELA/binding.hpp>
#include <benchmark/benchmark.h>
#include <STELA/code generation.hpp>
#include <STELA/syntax analysis.hpp>
#include <STELA/semantic analysis.hpp>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

using namespace stela;

namespace {

llvm::ExecutionEngine *generate(const stela::Symbols &syms, LogSink &log, const char *id) {
  static std::unordered_set<const char *> printed;
  const bool print = printed.insert(id).second;
  std::unique_ptr<llvm::Module> module = stela::generateIR(syms, log);
  llvm::Module *modulePtr = module.get();
  std::string str;
  llvm::raw_string_ostream strStream(str);
  if (print) {
    strStream << *modulePtr;
    strStream.flush();
    std::cerr << "Generated IR\n" << str;
    str.clear();
  }
  llvm::ExecutionEngine *engine = stela::generateCode(std::move(module), log);
  if (print) {
    strStream << *modulePtr;
    strStream.flush();
    std::cerr << "Optimized IR\n" << str;
  }
  return engine;
}

llvm::ExecutionEngine *generate(const std::string_view source, LogSink &log) {
  stela::AST ast = stela::createAST(source, log);
  stela::Symbols syms = stela::initModules(log);
  stela::compileModule(syms, ast, log);
  return generate(syms, log, source.data());
}

template <typename Fun, bool Member = false>
auto getFunc(llvm::ExecutionEngine *engine, const std::string &name) {
  return stela::Function<Fun, Member>{engine->getFunctionAddress(name)};
}

LogSink &log() {
  static StreamSink stream;
  static FilterSink filter{stream, LogPri::info};
  return filter;
}

void ensureLLVM() {
  if (!stela::hasLLVM()) {
    stela::initLLVM();
  }
}

void squares(::benchmark::State &state) {
  ensureLLVM();

  auto *engine = generate(R"(
    extern func squares(count: uint) -> [uint] {
      var array: [uint];
      reserve(array, count);
      for (i := 0u; i < count; i++) {
        push_back(array, i * i);
      }
      return array;
    }
  )", log());
  auto squares = getFunc<Array<Uint>(Uint)>(engine, "squares");
  
  for (auto _ : state) {
    Array<Uint> array = squares(state.range());
    ::benchmark::DoNotOptimize(array);
  }
}
BENCHMARK(squares)->Range(512, 65536)->Unit(benchmark::kMicrosecond);

void squaresNoReserve(::benchmark::State &state) {
  ensureLLVM();

  auto *engine = generate(R"(
    extern func squares(count: uint) -> [uint] {
      var array: [uint];
      for (i := 0u; i < count; i++) {
        push_back(array, i * i);
      }
      return array;
    }
  )", log());
  auto squares = getFunc<Array<Uint>(Uint)>(engine, "squares");
  
  for (auto _ : state) {
    Array<Uint> array = squares(state.range());
    ::benchmark::DoNotOptimize(array);
  }
}
BENCHMARK(squaresNoReserve)->Range(512, 65536)->Unit(benchmark::kMicrosecond);

void euler1(::benchmark::State &state) {
  ensureLLVM();
  
  auto *engine = generate(R"(
    extern func calc(n: uint) {
      var sum = 0u;
      for (i := 0u; i != n; i++) {
        if (i % 3u == 0u || i % 5u == 0u) {
          sum += i;
        }
      }
      return sum;
    }
  )", log());
  auto calc = getFunc<Uint(Uint)>(engine, "calc");
  
  for (auto _ : state) {
    ::benchmark::DoNotOptimize(calc(state.range()));
  }
}
BENCHMARK(euler1)->Arg(1000)->Arg(10000)->Unit(benchmark::kMicrosecond);

Uint calcEuler1(const Uint n) {
  Uint sum = 0;
  for (Uint i = 0; i != n; ++i) {
    if (i % 3 == 0 || i % 5 == 0) {
      sum += i;
    }
  }
  return sum;
}

void euler1_cpp(::benchmark::State &state) {
  for (auto _ : state) {
    ::benchmark::DoNotOptimize(calcEuler1(static_cast<Uint>(state.range())));
  }
}
BENCHMARK(euler1_cpp)->Arg(1000)->Arg(10000)->Unit(benchmark::kMicrosecond);

}
