//
//  language benchmark.cpp
//  STELA
//
//  Created by Indi Kernick on 20/1/19.
//  Copyright © 2019 Indi Kernick. All rights reserved.
//

#include <STELA/llvm.hpp>
#include <STELA/binding.hpp>
#include <benchmark/benchmark.h>
#include <STELA/code generation.hpp>
#include <STELA/syntax analysis.hpp>
#include <STELA/semantic analysis.hpp>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

using namespace stela;

namespace {

llvm::ExecutionEngine *generate(const stela::Symbols &syms, LogSink &log) {
  return stela::generateCode(stela::generateIR(syms, log), log);
}

llvm::ExecutionEngine *generate(const std::string_view source, LogSink &log) {
  stela::AST ast = stela::createAST(source, log);
  stela::Symbols syms = stela::initModules(log);
  stela::compileModule(syms, ast, log);
  return generate(syms, log);
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
BENCHMARK(squares)->Range(512, 1<<16)->Unit(benchmark::kMicrosecond);

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
BENCHMARK(squaresNoReserve)->Range(512, 1<<16)->Unit(benchmark::kMicrosecond);

}
