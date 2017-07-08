/*
    Copyright (c) Aleksey Fedotov
    MIT license
*/

#pragma once

#include <exception>
#include <memory>

#if defined(WIN32) || defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS)
#   define KL_WINDOWS
#elif defined(__linux__)
#   define KL_LINUX
#endif

#define KL_MACRO_BLOCK(code) do { code; } while (false);
#define KL_EMPTY_MACRO_BLOCK() do {} while (false);

#ifdef KL_DEBUG
#   define KL_PANIC_BLOCK(code) KL_MACRO_BLOCK(code)
#   define KL_PANIC(...) KL_MACRO_BLOCK(throw std::exception(__VA_ARGS__))
#   define KL_PANIC_IF(condition, ...) KL_MACRO_BLOCK(if (condition) throw std::exception(__VA_ARGS__))
#else
#   define KL_PANIC_BLOCK(code) KL_EMPTY_MACRO_BLOCK()
#   define KL_PANIC(...) KL_EMPTY_MACRO_BLOCK()
#   define KL_PANIC_IF(condition, ...) KL_EMPTY_MACRO_BLOCK()
#endif

template <typename T> using sptr = std::shared_ptr<T>;
template <typename T> using uptr = std::unique_ptr<T>;