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

#define PRINT_CODE 0

llvm::ExecutionEngine *generate(const stela::Symbols &syms, LogSink &log, [[maybe_unused]] const char *id) {
  #if PRINT_CODE
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
  #else
  return stela::generateCode(stela::generateIR(syms, log), log);
  #endif
}

#undef PRINT_CODE

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

#define GENERATE(SOURCE) [[maybe_unused]] auto *engine = generate(SOURCE, log())
#define GET_FUNC(NAME, ...) getFunc<__VA_ARGS__>(engine, NAME)

void ensureLLVM() {
  if (!stela::hasLLVM()) {
    stela::initLLVM();
  }
}

void squares(::benchmark::State &state) {
  ensureLLVM();

  GENERATE(R"(
    extern func squares(count: uint) -> [uint] {
      var array: [uint];
      reserve(array, count);
      for (i := 0u; i < count; i++) {
        push_back(array, i * i);
      }
      return array;
    }
  )");
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

void euler1_stela(::benchmark::State &state) {
  ensureLLVM();
  
  GENERATE(R"(
    extern func calc(n: uint) {
      var sum = 0u;
      for (i := 0u; i != n; i++) {
        if (i % 3u == 0u || i % 5u == 0u) {
          sum += i;
        }
      }
      return sum;
    }
  )");
  auto calc = getFunc<Uint(Uint)>(engine, "calc");
  
  for (auto _ : state) {
    ::benchmark::DoNotOptimize(calc(state.range()));
  }
}
BENCHMARK(euler1_stela)->Arg(1000)->Arg(10000)->Unit(benchmark::kMicrosecond);

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

void mandelbrot_stela(::benchmark::State &state) {
  ensureLLVM();
  
  GENERATE(R"(
    type complex struct {
      r: real;
      i: real;
    };
    
    func plus(a: complex, b: complex) {
      return make complex {a.r + b.r, a.i + b.i};
    }
    
    func square(c: complex) {
      return make complex {c.r*c.r - c.i*c.i, 2.0 * c.r * c.i};
    }
    
    func next(z: complex, c: complex) {
      return plus(square(z), c);
    }
    
    func magnitude2(c: complex) {
      return c.r*c.r + c.i*c.i;
    }
    
    func mandelbrot(c: complex, iterations: sint) {
      var z = make complex {0.0, 0.0};
      var n = 0;
      for (; n < iterations && magnitude2(z) <= 4.0; n++) {
        z = next(z, c);
      }
      return n != iterations;
    }
    
    extern func render_mandelbrot(resolution: sint, iterations: sint) {
      let real_beg = -1.5;
      let real_end = 0.5;
      let imag_beg = -1.0;
      let imag_end = 1.0;
      let real_stp = (real_end - real_beg) / make real resolution;
      let imag_stp = (imag_end - imag_beg) / make real resolution;
      
      var pixels: [bool];
      reserve(pixels, make uint (resolution * resolution));
      for (i := 0; i != resolution; i++) {
        for (r := 0; r != resolution; r++) {
          let real_pos = real_beg + real_stp * make real r;
          let imag_pos = imag_beg + imag_stp * make real i;
          push_back(pixels, mandelbrot(make complex {real_pos, imag_pos}, iterations));
        }
      }
      return pixels;
    }
  )");
  
  auto mandelbrot = GET_FUNC("render_mandelbrot", Array<Bool>(Sint, Sint));
  
  for (auto _ : state) {
    ::benchmark::DoNotOptimize(mandelbrot(state.range(0), state.range(1)));
  }
}

BENCHMARK(mandelbrot_stela)->Unit(benchmark::kMillisecond)
  ->Args({128, 100})->Args({256, 100})->Args({512, 100});

template <typename Real>
struct Complex {
  Real r, i;
};

template <typename Real>
Complex(Real, Real) -> Complex<Real>;

template <typename Real>
Complex<Real> plus(Complex<Real> a, Complex<Real> b) {
  return {a.r + b.r, a.i + b.i};
}

template <typename Real>
Complex<Real> square(Complex<Real> c) {
  return {c.r*c.r - c.i*c.i, Real{2} * c.r * c.i};
}

template <typename Real>
Complex<Real> next(Complex<Real> z, Complex<Real> c) {
  return plus(square(z), c);
}

template <typename Real>
Real magnitude2(Complex<Real> c) {
  return c.r*c.r + c.i*c.i;
}

template <typename Real, typename Integer>
bool mandelbrot(Complex<Real> c, Integer iterations) {
  Complex z = {Real{0}, Real{0}};
  Integer n = 0;
  for (; n < iterations && magnitude2(z) <= Real{2*2}; ++n) {
    z = next(z, c);
  }
  return n != iterations;
}

template <typename Real, typename Integer>
std::vector<unsigned char> mandelbrot(Integer resolution, Integer iterations) {
  constexpr Real real_beg = -1.5;
  constexpr Real real_end = 0.5;
  constexpr Real imag_beg = -1.0;
  constexpr Real imag_end = 1.0;
  const Real real_stp = (real_end - real_beg) / resolution;
  const Real imag_stp = (imag_end - imag_beg) / resolution;

  std::vector<unsigned char> pixels;
  pixels.reserve(static_cast<size_t>(resolution * resolution));
  for (Integer i = 0; i != resolution; ++i) {
    for (Integer r = 0; r != resolution; ++r) {
      const Real real_pos = real_beg + real_stp * r;
      const Real imag_pos = imag_beg + imag_stp * i;
      pixels.push_back(mandelbrot(Complex{real_pos, imag_pos}, iterations));
    }
  }
  return pixels;
}

template <typename Real, typename Integer>
void mandelbrot_cpp(::benchmark::State &state) {
  for (auto _ : state) {
    ::benchmark::DoNotOptimize(mandelbrot<Real, Integer>(
      static_cast<Integer>(state.range(0)),
      static_cast<Integer>(state.range(1))
    ));
  }
}

BENCHMARK_TEMPLATE(mandelbrot_cpp, double, int32_t)->Unit(benchmark::kMillisecond)
  ->Args({128, 100})->Args({256, 100})->Args({512, 100});
BENCHMARK_TEMPLATE(mandelbrot_cpp, float, int32_t)->Unit(benchmark::kMillisecond)
  ->Args({128, 100})->Args({256, 100})->Args({512, 100});

/*
real is double
sint is i32

mandelbrot_stela/128/100                      3.59 ms         3.57 ms          191
mandelbrot_stela/256/100                      14.2 ms         14.2 ms           48
mandelbrot_stela/512/100                      56.5 ms         56.3 ms           13
mandelbrot_cpp<double, int32_t>/128/100       3.61 ms         3.59 ms          192
mandelbrot_cpp<double, int32_t>/256/100       14.3 ms         14.2 ms           49
mandelbrot_cpp<double, int32_t>/512/100       56.8 ms         56.6 ms           12
mandelbrot_cpp<float, int32_t>/128/100        3.97 ms         3.96 ms          173
mandelbrot_cpp<float, int32_t>/256/100        16.0 ms         16.0 ms           44
mandelbrot_cpp<float, int32_t>/512/100        64.0 ms         63.6 ms           11

real is float
sint is i32

mandelbrot_stela/128/100                      3.57 ms         3.56 ms          189
mandelbrot_stela/256/100                      14.3 ms         14.2 ms           48
mandelbrot_stela/512/100                      57.9 ms         57.7 ms           13
mandelbrot_cpp<double, int32_t>/128/100       3.58 ms         3.56 ms          195
mandelbrot_cpp<double, int32_t>/256/100       14.3 ms         14.3 ms           50
mandelbrot_cpp<double, int32_t>/512/100       56.6 ms         56.4 ms           12
mandelbrot_cpp<float, int32_t>/128/100        4.01 ms         4.01 ms          171
mandelbrot_cpp<float, int32_t>/256/100        16.0 ms         15.9 ms           45
mandelbrot_cpp<float, int32_t>/512/100        63.7 ms         63.5 ms           10
*/

}
