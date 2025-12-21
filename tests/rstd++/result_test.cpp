/**
 * @file test_result.cpp
 * @brief Unit tests for Result<T, E> type using Google Test
 */

#include "rstd++/result.hpp"
#include <gtest/gtest.h>
#include <sstream>
#include <string>

using namespace rstd;

// ======================================================================
// Basic Construction Tests
// ======================================================================

TEST(ResultTest, OkConstructionWithValue)
{
    Result<int, std::string> r = Result<int, std::string>::Ok(42);
    EXPECT_TRUE(r.is_ok());
    EXPECT_FALSE(r.is_err());
    EXPECT_EQ(r.unwrap(), 42);
}

TEST(ResultTest, ErrConstructionWithValue)
{
    Result<int, std::string> r = Result<int, std::string>::Err("error");
    EXPECT_FALSE(r.is_ok());
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(r.unwrap_err(), "error");
}

TEST(ResultTest, OkConstructionWithVoid)
{
    Result<Void, std::string> r = Result<Void, std::string>::Ok(Void{});
    EXPECT_TRUE(r.is_ok());
    EXPECT_FALSE(r.is_err());
}

TEST(ResultTest, ErrConstructionWithVoid)
{
    Result<int, Void> r = Result<int, Void>::Err(Void{});
    EXPECT_FALSE(r.is_ok());
    EXPECT_TRUE(r.is_err());
}

TEST(ResultTest, VoidVoidResult)
{
    Result<Void, Void> r1 = Result<Void, Void>::Ok(Void{});
    EXPECT_TRUE(r1.is_ok());

    Result<Void, Void> r2 = Result<Void, Void>::Err(Void{});
    EXPECT_TRUE(r2.is_err());
}

// ======================================================================
// Factory Function Tests
// ======================================================================

TEST(ResultFactoryTest, OkWithValue)
{
    auto r = Ok<int, std::string>(42);
    EXPECT_TRUE(r.is_ok());
    EXPECT_EQ(r.unwrap(), 42);
}

TEST(ResultFactoryTest, ErrWithValue)
{
    auto r = Err<int, std::string>(std::string("error"));
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(r.unwrap_err(), "error");
}

TEST(ResultFactoryTest, OkRvalue)
{
    std::string s = "hello";
    auto r = Ok<std::string, int>(std::move(s));
    EXPECT_TRUE(r.is_ok());
    EXPECT_EQ(r.unwrap(), "hello");
}

TEST(ResultFactoryTest, ErrRvalue)
{
    std::string s = "error";
    auto r = Err<int, std::string>(std::move(s));
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(r.unwrap_err(), "error");
}

// ======================================================================
// Querying Tests
// ======================================================================

TEST(ResultQueryTest, IsOkAndWithPredicate)
{
    auto r1 = Result<int, std::string>::Ok(42);
    EXPECT_TRUE(r1.is_ok_and([](int x) { return x > 40; }));
    EXPECT_FALSE(r1.is_ok_and([](int x) { return x < 40; }));

    auto r2 = Result<int, std::string>::Err("error");
    EXPECT_FALSE(r2.is_ok_and([](int x) { return x > 40; }));
}

TEST(ResultQueryTest, IsErrAndWithPredicate)
{
    auto r1 = Result<int, std::string>::Err("error");
    EXPECT_TRUE(r1.is_err_and([](const std::string &s) { return s == "error"; }));
    EXPECT_FALSE(r1.is_err_and([](const std::string &s) { return s == "other"; }));

    auto r2 = Result<int, std::string>::Ok(42);
    EXPECT_FALSE(r2.is_err_and([](const std::string &s) { return s == "error"; }));
}

TEST(ResultQueryTest, IsOkAndWithVoid)
{
    auto r1 = Result<Void, std::string>::Ok(Void{});
    EXPECT_TRUE(r1.is_ok_and([](const Void &) { return true; }));

    auto r2 = Result<Void, std::string>::Err("error");
    EXPECT_FALSE(r2.is_ok_and([](const Void &) { return true; }));
}

TEST(ResultQueryTest, IsErrAndWithVoid)
{
    auto r1 = Result<int, Void>::Err(Void{});
    EXPECT_TRUE(r1.is_err_and([](const Void &) { return true; }));

    auto r2 = Result<int, Void>::Ok(42);
    EXPECT_FALSE(r2.is_err_and([](const Void &) { return true; }));
}

// ======================================================================
// Adapter Tests
// ======================================================================

TEST(ResultAdapterTest, OkToOptional)
{
    auto r1 = Result<int, std::string>::Ok(42);
    auto opt1 = r1.ok();
    EXPECT_TRUE(opt1.has_value());
    EXPECT_EQ(opt1.value(), 42);

    auto r2 = Result<int, std::string>::Err("error");
    auto opt2 = r2.ok();
    EXPECT_FALSE(opt2.has_value());
}

TEST(ResultAdapterTest, ErrToOptional)
{
    auto r1 = Result<int, std::string>::Err("error");
    auto opt1 = r1.err();
    EXPECT_TRUE(opt1.has_value());
    EXPECT_EQ(opt1.value(), "error");

    auto r2 = Result<int, std::string>::Ok(42);
    auto opt2 = r2.err();
    EXPECT_FALSE(opt2.has_value());
}

TEST(ResultAdapterTest, OkToOptionalWithVoid)
{
    auto r1 = Result<Void, std::string>::Ok(Void{});
    auto opt1 = r1.ok();
    EXPECT_TRUE(opt1.has_value());

    auto r2 = Result<Void, std::string>::Err("error");
    auto opt2 = r2.ok();
    EXPECT_FALSE(opt2.has_value());
}

TEST(ResultAdapterTest, ErrToOptionalWithVoid)
{
    auto r1 = Result<int, Void>::Err(Void{});
    auto opt1 = r1.err();
    EXPECT_TRUE(opt1.has_value());

    auto r2 = Result<int, Void>::Ok(42);
    auto opt2 = r2.err();
    EXPECT_FALSE(opt2.has_value());
}

// ======================================================================
// Transformation Tests
// ======================================================================

TEST(ResultTransformTest, MapOkValue)
{
    auto r = Result<int, std::string>::Ok(42);
    auto r2 = r.map([](int x) { return x * 2; });
    EXPECT_TRUE(r2.is_ok());
    EXPECT_EQ(r2.unwrap(), 84);
}

TEST(ResultTransformTest, MapErrValue)
{
    auto r = Result<int, std::string>::Err("error");
    auto r2 = r.map([](int x) { return x * 2; });
    EXPECT_TRUE(r2.is_err());
    EXPECT_EQ(r2.unwrap_err(), "error");
}

TEST(ResultTransformTest, MapChangesType)
{
    auto r = Result<int, std::string>::Ok(42);
    auto r2 = r.map([](int x) { return std::to_string(x); });
    EXPECT_TRUE(r2.is_ok());
    EXPECT_EQ(r2.unwrap(), "42");
}

TEST(ResultTransformTest, MapVoidToValue)
{
    auto r = Result<Void, std::string>::Ok(Void{});
    auto r2 = r.map([](const Void &) { return 100; });
    EXPECT_TRUE(r2.is_ok());
    EXPECT_EQ(r2.unwrap(), 100);
}

TEST(ResultTransformTest, MapValueToVoid)
{
    auto r = Result<int, std::string>::Ok(42);
    auto r2 = r.map([](int) { return Void{}; });
    EXPECT_TRUE(r2.is_ok());
}

TEST(ResultTransformTest, MapOrWithOk)
{
    auto r = Result<int, std::string>::Ok(42);
    int result = r.map_or(0, [](int x) { return x * 2; });
    EXPECT_EQ(result, 84);
}

TEST(ResultTransformTest, MapOrWithErr)
{
    auto r = Result<int, std::string>::Err("error");
    int result = r.map_or(0, [](int x) { return x * 2; });
    EXPECT_EQ(result, 0);
}

TEST(ResultTransformTest, MapOrWithVoid)
{
    auto r1 = Result<Void, std::string>::Ok(Void{});
    int result1 = r1.map_or(0, [](const Void &) { return 42; });
    EXPECT_EQ(result1, 42);

    auto r2 = Result<Void, std::string>::Err("error");
    int result2 = r2.map_or(0, [](const Void &) { return 42; });
    EXPECT_EQ(result2, 0);
}

// ======================================================================
// Extraction Tests
// ======================================================================

TEST(ResultExtractionTest, ExpectOk)
{
    auto r = Result<int, std::string>::Ok(42);
    EXPECT_EQ(r.expect("should be ok"), 42);
}

TEST(ResultExtractionTest, ExpectErrThrows)
{
    auto r = Result<int, std::string>::Err("error");
    EXPECT_THROW(
        {
            try {
                r.expect("custom message");
            } catch (const std::runtime_error &e) {
                std::string msg = e.what();
                EXPECT_NE(msg.find("custom message"), std::string::npos);
                EXPECT_NE(msg.find("error"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

TEST(ResultExtractionTest, ExpectWithVoid)
{
    auto r = Result<Void, std::string>::Ok(Void{});
    EXPECT_NO_THROW(r.expect("should be ok"));
}

TEST(ResultExtractionTest, UnwrapOk)
{
    auto r = Result<int, std::string>::Ok(42);
    EXPECT_EQ(r.unwrap(), 42);
}

TEST(ResultExtractionTest, UnwrapErrThrows)
{
    auto r = Result<int, std::string>::Err("error");
    EXPECT_THROW(
        {
            try {
                r.unwrap();
            } catch (const std::runtime_error &e) {
                std::string msg = e.what();
                EXPECT_NE(msg.find("unwrap()"), std::string::npos);
                EXPECT_NE(msg.find("error"), std::string::npos);
                throw;
            }
        },
        std::runtime_error);
}

TEST(ResultExtractionTest, UnwrapWithVoid)
{
    auto r = Result<Void, std::string>::Ok(Void{});
    EXPECT_NO_THROW(r.unwrap());
}

TEST(ResultExtractionTest, UnwrapOrDefault)
{
    auto r1 = Result<int, std::string>::Ok(42);
    EXPECT_EQ(r1.unwrap_or_default(), 42);

    auto r2 = Result<int, std::string>::Err("error");
    EXPECT_EQ(r2.unwrap_or_default(), 0);
}

TEST(ResultExtractionTest, ExpectErrOk)
{
    auto r = Result<int, std::string>::Err("error");
    EXPECT_EQ(r.expect_err("should be err"), "error");
}

TEST(ResultExtractionTest, ExpectErrWithVoid)
{
    auto r = Result<int, Void>::Err(Void{});
    EXPECT_NO_THROW(r.expect_err("should be err"));
}

TEST(ResultExtractionTest, UnwrapErrOk)
{
    auto r = Result<int, std::string>::Err("error");
    EXPECT_EQ(r.unwrap_err(), "error");
}

TEST(ResultExtractionTest, UnwrapErrWithVoid)
{
    auto r = Result<int, Void>::Err(Void{});
    EXPECT_NO_THROW(r.unwrap_err());
}

// ======================================================================
// Move Semantics Tests
// ======================================================================

TEST(ResultMoveTest, MoveOkValue)
{
    auto r = Result<std::string, int>::Ok(std::string("hello"));
    std::string s = std::move(r).unwrap();
    EXPECT_EQ(s, "hello");
}

TEST(ResultMoveTest, MoveErrValue)
{
    auto r = Result<int, std::string>::Err(std::string("error"));
    std::string s = std::move(r).unwrap_err();
    EXPECT_EQ(s, "error");
}

TEST(ResultMoveTest, MapWithRvalue)
{
    auto r = Result<std::string, int>::Ok(std::string("hello"));
    auto r2 = std::move(r).map([](std::string s) { return s + " world"; });
    EXPECT_EQ(r2.unwrap(), "hello world");
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

struct NonCopyable
{
    int value;
    NonCopyable(int v) : value(v) {}
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;
    NonCopyable(NonCopyable &&) = default;
    NonCopyable &operator=(NonCopyable &&) = default;
};

TEST(ResultComplexTest, NonCopyableType)
{
    auto r = Result<NonCopyable, std::string>::Ok(NonCopyable(42));
    EXPECT_TRUE(r.is_ok());
    EXPECT_EQ(std::move(r).unwrap().value, 42);
}

struct CustomError
{
    int code;
    std::string message;

    bool operator==(const CustomError &other) const
    {
        return code == other.code && message == other.message;
    }
};

std::ostream &operator<<(std::ostream &os, const CustomError &e)
{
    return os << "Error(" << e.code << ", " << e.message << ")";
}

TEST(ResultComplexTest, CustomErrorType)
{
    auto r = Result<int, CustomError>::Err(CustomError{404, "Not found"});
    EXPECT_TRUE(r.is_err());
    CustomError e = r.unwrap_err();
    EXPECT_EQ(e.code, 404);
    EXPECT_EQ(e.message, "Not found");
}

TEST(ResultComplexTest, CustomErrorInException)
{
    auto r = Result<int, CustomError>::Err(CustomError{500, "Server error"});
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
