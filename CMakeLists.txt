cmake_minimum_required(VERSION 3.16)

project(FixedIncomeDeskQuantPlatform VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FIQ_BUILD_TESTS "Build C++ tests" ON)
option(FIQ_USE_CATCH2 "Use Catch2 via CMake FetchContent for tests" OFF)

add_library(fiq_engine
    src/core/Date.cpp
    src/core/CsvReader.cpp
    src/core/Utils.cpp
    src/rates/YieldCurve.cpp
    src/rates/FixedRateBond.cpp
    src/rates/VanillaSwap.cpp
    src/rates/Portfolio.cpp
    src/rates/RiskEngine.cpp
    src/rates/PnLExplain.cpp
    src/execution/MarketEvent.cpp
    src/execution/LimitOrderBook.cpp
    src/execution/MarketDataSimulator.cpp
    src/execution/ExecutionAnalytics.cpp
    src/execution/ExecutionSimulator.cpp
)

target_include_directories(fiq_engine PUBLIC include)

if (MSVC)
    target_compile_options(fiq_engine PRIVATE /W4)
else()
    target_compile_options(fiq_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(run_daily_desk_report apps/run_daily_desk_report.cpp)
target_link_libraries(run_daily_desk_report PRIVATE fiq_engine)

add_executable(run_intraday_simulation apps/run_intraday_simulation.cpp)
target_link_libraries(run_intraday_simulation PRIVATE fiq_engine)

add_executable(run_full_desk_day apps/run_full_desk_day.cpp)
target_link_libraries(run_full_desk_day PRIVATE fiq_engine)

if (FIQ_BUILD_TESTS)
    enable_testing()

    set(FIQ_TEST_SOURCES
        tests/test_yield_curve.cpp
        tests/test_bond_pricing.cpp
        tests/test_swap_pricing.cpp
        tests/test_risk_engine.cpp
        tests/test_pnl_explain.cpp
        tests/test_order_book.cpp
        tests/test_execution_analytics.cpp
    )

    if (FIQ_USE_CATCH2)
        include(FetchContent)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.7.1
        )
        FetchContent_MakeAvailable(Catch2)
        add_executable(fiq_tests ${FIQ_TEST_SOURCES})
        target_compile_definitions(fiq_tests PRIVATE FIQ_USE_CATCH2)
        target_link_libraries(fiq_tests PRIVATE fiq_engine Catch2::Catch2WithMain)
    else()
        add_executable(fiq_tests tests/TestMain.cpp ${FIQ_TEST_SOURCES})
        target_link_libraries(fiq_tests PRIVATE fiq_engine)
    endif()

    target_include_directories(fiq_tests PRIVATE tests)
    add_test(NAME fiq_tests COMMAND fiq_tests)
endif()
