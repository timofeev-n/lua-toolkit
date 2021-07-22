//◦ Playrix ◦

#pragma once
#include <SDKDDKVer.h>


#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#endif

#include <atomic>
#include <chrono>
#include <cmath>
#include <forward_list>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <random>
#include <regex>
#include <string>
#include <string_view>
#include <type_traits>
#include <thread>
#include <variant>
#include <vector>


#include <boost/optional.hpp>

#include <uv.h>

#ifdef Yield
#undef Yield
#endif

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gtest/gtest-param-test.h>

#include <EngineAssert.h>
#include <Core/Log.h>
#include <PlayrixEngine.h>

