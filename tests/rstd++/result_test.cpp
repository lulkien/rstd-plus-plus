/**
 * @file test_result.cpp
 * @brief Unit tests for Result<T, E> type using Google Test
 */

#include "rstd++/result.hpp"
#include <cmath>
#include <gtest/gtest.h>
#include <sstream>
#include <string>

using namespace rstd;

#define take(a) std::move(a)

struct NonCopyableValue
{
    int value;
    NonCopyableValue() : value(0) {}
    NonCopyableValue(int v) : value(v) {}
    NonCopyableValue(const NonCopyableValue &) = delete;
    auto operator=(const NonCopyableValue &) -> NonCopyableValue & = delete;
    NonCopyableValue(NonCopyableValue &&) = default;
    auto operator=(NonCopyableValue &&) -> NonCopyableValue & = default;
    auto operator==(const int &ov) const -> bool { return value == ov; }
    auto operator==(const NonCopyableValue &o) const -> bool { return value == o.value; }
};

struct NonCopyableError
{
    std::string error;
    NonCopyableError() : error{""} {}
    NonCopyableError(const char *str) : error{str} {}
    NonCopyableError(const NonCopyableError &) = delete;
    auto operator=(const NonCopyableError &) -> NonCopyableError & = delete;
    NonCopyableError(NonCopyableError &&) = default;
    auto operator=(NonCopyableError &&) -> NonCopyableError & = default;
    auto operator==(const char *str) const -> bool { return error.compare(str) == 0; }
    auto operator==(const NonCopyableError &o) const -> bool { return error.compare(o.error) == 0; }
};

inline auto f_equal(const float a, const float b, const float e = 1e-6) -> bool
{
    return std::fabs(a - b) < e;
}

// ======================================================================
// Basic Construction Tests
// ======================================================================

TEST(ResultCreateTest, OkCreate)
{
    auto r = Result<int, std::string>::Ok(42);
    EXPECT_TRUE(r.is_ok());
    EXPECT_EQ(r.unwrap(), 42);
}

TEST(ResultCreateTest, OkCreateNonCopy)
{
    auto r = Result<NonCopyableValue, std::string>::Ok(42);
    EXPECT_TRUE(r.is_ok());
    EXPECT_EQ(take(r).unwrap(), 42);
}

TEST(ResultCreateTest, ErrCreate)
{
    auto r = Result<int, std::string>::Err("ErrCreate");
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(r.unwrap_err(), "ErrCreate");
}

TEST(ResultCreateTest, ErrCreateNonCopy)
{
    auto r = Result<Void, NonCopyableError>::Err("ErrCreateNonCopy");
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(take(r).unwrap_err(), "ErrCreateNonCopy");
}

TEST(ResultCreateTest, OkVoid)
{
    auto r = Result<Void, std::string>::Ok(Void{});
    EXPECT_TRUE(r.is_ok());
    EXPECT_FALSE(r.is_err());
}

TEST(ResultCreateTest, ErrVoid)
{
    auto r = Result<int, Void>::Err(Void{});
    EXPECT_FALSE(r.is_ok());
    EXPECT_TRUE(r.is_err());
}

// ======================================================================
// Factory Function Tests
// ======================================================================

TEST(ResultFactoryCreateTest, OkCreate)
{
    auto r = Ok<int, std::string>(42);
    EXPECT_TRUE(r.is_ok());
    EXPECT_EQ(r.unwrap(), 42);
}

TEST(ResultFactoryCreateTest, OkCreateNonCopy)
{
    auto r = Ok<NonCopyableValue, int>(100);
    EXPECT_TRUE(r.is_ok());
    EXPECT_EQ(take(r).unwrap(), 100);
}

TEST(ResultFactoryCreateTest, ErrCreate)
{
    auto r = Err<int, std::string>(std::string("ErrCreate"));
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(r.unwrap_err(), "ErrCreate");
}

TEST(ResultFactoryCreateTest, ErrCreateNonCopy)
{
    auto r = Err<int, NonCopyableError>("ErrCreateNonCopy");
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(take(r).unwrap_err(), "ErrCreateNonCopy");
}

TEST(ResultFactoryCreateTest, OkVoid)
{
    auto r = Ok<Void, std::string>(Void{});
    EXPECT_TRUE(r.is_ok());
    EXPECT_FALSE(r.is_err());
}

TEST(ResultFactoryCreateTest, ErrVoid)
{
    auto r = Err<int, Void>(Void{});
    EXPECT_FALSE(r.is_ok());
    EXPECT_TRUE(r.is_err());
}

// ======================================================================
// Querying Tests
// ======================================================================

TEST(ResultQueryTest, IsOkAnd)
{
    auto r1 = Ok<int, std::string>(42);
    EXPECT_TRUE(r1.is_ok_and([](int x) -> bool { return x > 40; }));
    EXPECT_FALSE(r1.is_ok_and([](int x) -> bool { return x < 40; }));

    auto r2 = Err<int, std::string>("error");
    EXPECT_FALSE(r2.is_ok_and([](int x) -> bool { return x > 40; }));
}

TEST(ResultQueryTest, IsOkAndNonCopy)
{
    auto r1 = Ok<NonCopyableValue, std::string>(10);
    EXPECT_TRUE(take(r1).is_ok_and([](NonCopyableValue &&v) -> bool { return v.value > 0; }));
    EXPECT_FALSE(take(r1).is_ok_and([](NonCopyableValue &&v) -> bool { return v.value < 0; }));

    auto r2 = Err<int, NonCopyableError>("IsOkAndNonCopy");
    EXPECT_FALSE(take(r2).is_ok_and([](NonCopyableValue &&v) -> bool { return v.value > 0; }));
}

TEST(ResultQueryTest, IsErrAnd)
{
    auto r1 = Err<int, std::string>("error");
    EXPECT_TRUE(r1.is_err_and([](const std::string &s) -> bool { return s == "error"; }));
    EXPECT_FALSE(r1.is_err_and([](const std::string &s) -> bool { return s == "other"; }));

    auto r2 = Ok<int, std::string>(42);
    EXPECT_FALSE(r2.is_err_and([](const std::string &s) -> bool { return s == "error"; }));
}

TEST(ResultQueryTest, IsErrAndNonCopy)
{
    auto r1 = Err<int, NonCopyableError>("IsErrAndNonCopy");
    EXPECT_TRUE(take(r1).is_err_and(
        [](NonCopyableError &&e) -> bool { return e.error == "IsErrAndNonCopy"; }));
    EXPECT_FALSE(
        take(r1).is_err_and([](NonCopyableError &&e) -> bool { return e.error == "other"; }));

    auto r2 = Ok<NonCopyableValue, std::string>(42);
    EXPECT_FALSE(r2.is_err_and([](const std::string &s) -> bool { return s == "error"; }));
}

TEST(ResultQueryTest, IsOkAndVoid)
{
    auto r1 = Ok<Void, std::string>(Void{});
    EXPECT_TRUE(r1.is_ok_and([](const Void &) -> bool { return true; }));

    auto r2 = Err<Void, std::string>("error");
    EXPECT_FALSE(r2.is_ok_and([](const Void &) -> bool { return true; }));
}

TEST(ResultQueryTest, IsErrAndVoid)
{
    auto r1 = Err<int, Void>(Void{});
    EXPECT_TRUE(r1.is_err_and([](const Void &) -> bool { return true; }));

    auto r2 = Ok<int, Void>(42);
    EXPECT_FALSE(r2.is_err_and([](const Void &) -> bool { return true; }));
}

// ======================================================================
// Adapter Tests
// ======================================================================

TEST(ResultAdapterTest, OkToOptional)
{
    auto r1 = Ok<int, std::string>(42);
    auto opt1 = r1.ok();
    EXPECT_TRUE(opt1.has_value());
    EXPECT_EQ(opt1.value(), 42);

    auto r2 = Err<int, std::string>("error");
    auto opt2 = r2.ok();
    EXPECT_FALSE(opt2.has_value());
}

TEST(ResultAdapterTest, OkToOptionalNonCopy)
{
    auto r1 = Ok<NonCopyableValue, std::string>(42);
    auto opt1 = take(r1).ok();
    EXPECT_TRUE(opt1.has_value());
    EXPECT_EQ(opt1.value(), 42);

    auto r2 = Err<NonCopyableValue, std::string>("error");
    auto opt2 = take(r2).ok();
    EXPECT_FALSE(opt2.has_value());
}

TEST(ResultAdapterTest, ErrToOptional)
{
    auto r1 = Err<int, std::string>("error");
    auto opt1 = r1.err();
    EXPECT_TRUE(opt1.has_value());
    EXPECT_EQ(opt1.value(), "error");

    auto r2 = Ok<int, std::string>(42);
    auto opt2 = r2.err();
    EXPECT_FALSE(opt2.has_value());
}

TEST(ResultAdapterTest, ErrToOptionalNonCopy)
{
    auto r1 = Err<int, NonCopyableError>("error");
    auto opt1 = take(r1).err();
    EXPECT_TRUE(opt1.has_value());
    EXPECT_EQ(opt1.value(), "error");

    auto r2 = Ok<int, NonCopyableError>(42);
    auto opt2 = take(r2).err();
    EXPECT_FALSE(opt2.has_value());
}

TEST(ResultAdapterTest, OkToOptionalVoid)
{
    auto r1 = Ok<Void, std::string>(Void{});
    auto opt1 = r1.ok();
    EXPECT_TRUE(opt1.has_value());

    auto r2 = Err<Void, std::string>("error");
    auto opt2 = r2.ok();
    EXPECT_FALSE(opt2.has_value());
}

TEST(ResultAdapterTest, ErrToOptionalVoid)
{
    auto r1 = Err<int, Void>(Void{});
    auto opt1 = r1.err();
    EXPECT_TRUE(opt1.has_value());

    auto r2 = Ok<int, Void>(42);
    auto opt2 = r2.err();
    EXPECT_FALSE(opt2.has_value());
}

// ======================================================================
// Transformation Tests
// ======================================================================

TEST(ResultTransformTest, MapOk)
{
    auto r = Ok<int, std::string>(42);
    auto r2 = r.map([](int x) -> int { return x * 2; });
    EXPECT_TRUE(r2.is_ok());
    EXPECT_EQ(r2.unwrap(), 84);
}

TEST(ResultTransformTest, MapOkNonCopy)
{
    auto r1 = Ok<NonCopyableValue, std::string>(50);
    auto r2 = take(r1).map([](NonCopyableValue &&v) -> NonCopyableValue { return {v.value * 2}; });
    EXPECT_TRUE(r2.is_ok());
    EXPECT_EQ(take(r2).unwrap(), 100);
}

TEST(ResultTransformTest, MapErr)
{
    auto r = Err<int, std::string>("MapErr");
    auto r2 = r.map([](int x) -> int { return x * 2; });
    EXPECT_TRUE(r2.is_err());
    EXPECT_EQ(r2.unwrap_err(), "MapErr");
}

TEST(ResultTransformTest, MapErrNonCopy)
{
    auto r1 = Err<int, NonCopyableError>("MapErrNonCopy");
    auto r2 = take(r1).map([](int x) -> int { return x * 2; });
    EXPECT_TRUE(r2.is_err());
    EXPECT_EQ(take(r2).unwrap_err(), "MapErrNonCopy");
}

TEST(ResultTransformTest, MapToNewType)
{
    auto r = Ok<int, const char *>(42);
    auto r2 = r.map([](int x) -> float { return 5.0f; });
    EXPECT_TRUE(r2.is_ok());
    EXPECT_TRUE(f_equal(r2.unwrap(), 5.0f));
}

TEST(ResultTransformTest, MapVoidToValue)
{
    auto r = Ok<Void, const char *>(Void{});
    auto r2 = r.map([](const Void &) -> int { return 100; });
    EXPECT_TRUE(r2.is_ok());
    EXPECT_EQ(r2.unwrap(), 100);
}

TEST(ResultTransformTest, MapOrWithOk)
{
    auto r = Ok<int, const char *>(42);
    auto result = r.map_or(0, [](int x) -> int { return x * 2; });
    EXPECT_EQ(result, 84);
}

TEST(ResultTransformTest, MapOrWithErr)
{
    auto r = Err<int, const char *>("MapOrWithErr");
    auto result = r.map_or(0, [](int x) -> int { return x * 2; });
    EXPECT_EQ(result, 0);
}

TEST(ResultTransformTest, MapOrWithVoid)
{
    auto r1 = Ok<Void, const char *>(Void{});
    auto result1 = r1.map_or(0, [](const Void &) -> int { return 42; });
    EXPECT_EQ(result1, 42);

    auto r2 = Err<Void, const char *>("MapOrWithVoid");
    auto result2 = r2.map_or(0, [](const Void &) -> int { return 42; });
    EXPECT_EQ(result2, 0);
}

// ======================================================================
// Extraction Tests
// ======================================================================

TEST(ResultExtractionTest, ExpectOk)
{
    auto r = Ok<int, const char *>(42);
    EXPECT_EQ(r.expect("should be ok"), 42);
}

TEST(ResultExtractionTest, ExpectFailed)
{
    auto r = Err<int, std::string>("BBBB");
    EXPECT_THROW(
        {
            try {
                r.expect("AAAA");
            } catch (const std::runtime_error &e) {
                auto check = std::string{e.what()}.compare("AAAA: BBBB") == 0;
                EXPECT_TRUE(check);
                throw;
            }
        },
        std::runtime_error);
}

TEST(ResultExtractionTest, ExpectWithVoid)
{
    auto r = Ok<Void, const char *>(Void{});
    EXPECT_NO_THROW(r.expect("should be ok"));
}

TEST(ResultExtractionTest, UnwrapOk)
{
    auto r = Ok<int, const char *>(42);
    EXPECT_EQ(r.unwrap(), 42);
}

TEST(ResultExtractionTest, UnwrapFailed)
{
    auto r = Err<int, const char *>("AAAA");
    EXPECT_THROW(
        {
            try {
                r.unwrap();
            } catch (const std::runtime_error &e) {
                auto check = std::string{e.what()}.compare(
                                 "called `Result::unwrap()` on an `Err` value: AAAA") == 0;
                EXPECT_TRUE(check);
                throw;
            }
        },
        std::runtime_error);
}

TEST(ResultExtractionTest, UnwrapWithVoid)
{
    auto r = Ok<Void, std::string>(Void{});
    EXPECT_NO_THROW(r.unwrap());
}

TEST(ResultExtractionTest, UnwrapOrDefault)
{
    auto r1 = Ok<int, std::string>(42);
    EXPECT_EQ(r1.unwrap_or_default(), 42);

    auto r2 = Err<int, std::string>("AAAA");
    EXPECT_EQ(r2.unwrap_or_default(), 0);
}

TEST(ResultExtractionTest, ExpectErrOk)
{
    auto r = Err<int, std::string>("AAAA");
    EXPECT_EQ(r.expect_err("should be err"), "AAAA");
}

TEST(ResultExtractionTest, ExpectErrFailed)
{
    auto r = Ok<int, const char *>(100);
    EXPECT_THROW(
        {
            try {
                r.expect_err("AAAA");
            } catch (const std::runtime_error &e) {
                auto check = std::string{e.what()}.compare("AAAA: 100") == 0;
                EXPECT_TRUE(check);
                throw;
            }
        },
        std::runtime_error);
}

TEST(ResultExtractionTest, ExpectErrWithVoid)
{
    auto r = Err<int, Void>(Void{});
    EXPECT_NO_THROW(r.expect_err("should be err"));
}

TEST(ResultExtractionTest, UnwrapErrOk)
{
    auto r = Err<int, std::string>("AAAA");
    EXPECT_EQ(r.unwrap_err(), "AAAA");
}

TEST(ResultExtractionTest, UnwrapErrFailed)
{
    auto r = Ok<int, const char *>(100);
    EXPECT_THROW(
        {
            try {
                r.unwrap_err();
            } catch (const std::runtime_error &e) {
                auto check = std::string{e.what()}.compare(
                    "called `Result::unwrap_err()` on an `Ok` value: 100") == 0;
                EXPECT_TRUE(check);
                throw;
            }
        },
        std::runtime_error);
}

TEST(ResultExtractionTest, UnwrapErrWithVoid)
{
    auto r = Err<int, Void>(Void{});
    EXPECT_NO_THROW(r.unwrap_err());
}

// ======================================================================
// Void Type Tests
// ======================================================================

TEST(VoidTest, Equality)
{
    Void v1;
    Void v2;
    EXPECT_EQ(v1, v2);
    EXPECT_FALSE(v1 != v2);
}

TEST(VoidTest, Printable)
{
    std::ostringstream oss;
    oss << Void{};
    EXPECT_EQ(oss.str(), "()");
}

// ======================================================================
// Complex Type Tests
// ======================================================================

TEST(ResultComplexTest, NonCopyableType)
{
    auto r = Ok<NonCopyableValue, std::string>(NonCopyableValue(42));
    EXPECT_TRUE(r.is_ok());
    EXPECT_EQ(std::move(r).unwrap().value, 42);
}

struct CustomError
{
    int code;
    std::string message;

    auto operator==(const CustomError &other) const -> bool
    {
        return code == other.code && message == other.message;
    }
};

auto operator<<(std::ostream &os, const CustomError &e) -> std::ostream &
{
    return os << "Error(" << e.code << ", " << e.message << ")";
}

TEST(ResultComplexTest, CustomErrorType)
{
    auto r = Err<int, CustomError>(CustomError{.code = 404, .message = "Not found"});
    EXPECT_TRUE(r.is_err());
    CustomError e = r.unwrap_err();
    EXPECT_EQ(e.code, 404);
    EXPECT_EQ(e.message, "Not found");
}

TEST(ResultComplexTest, CustomErrorInException)
{
    auto r = Err<int, CustomError>(CustomError{.code = 500, .message = "Server error"});
    EXPECT_THROW(
        {
            try {
                r.unwrap();
            } catch (const std::runtime_error &e) {
                std::string msg = e.what();
                EXPECT_NE(msg.find("Error(500, Server error)"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}
