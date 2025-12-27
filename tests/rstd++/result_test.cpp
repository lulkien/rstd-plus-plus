/**
 * @file test_result.cpp
 * @brief Unit tests for Result<T, E> type using Google Test
 */

#include "rstd++/core.hpp"
#include "rstd++/result.hpp"
#include "gtest/gtest.h"
#include <cmath>
#include <cstdlib>
#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <string>

using namespace rstd;
using std::string;

#define take(a) std::move(a)

inline auto f_equal(const float a, const float b, const float e = 1e-6) -> bool
{
    return std::fabs(a - b) < e;
}

TEST(ResultCreationTest, CreateOkAndBasicQuery)
{
    auto r1 = Result<int, const char *>::Ok(420);
    auto r2 = Result<std::unique_ptr<int>, const char *>::Ok(
        std::make_unique<int>(69));
    auto r3 = Ok<Void, const char *>({});
    auto r4 = Ok<std::unique_ptr<double>, const char *>(
        std::make_unique<double>(-123.456f));

    EXPECT_TRUE(r1.is_ok());
    EXPECT_EQ(r1.unwrap(), 420);

    EXPECT_TRUE(r2.is_ok());
    EXPECT_EQ(*take(r2).unwrap(), 69);

    EXPECT_TRUE(r3.is_ok());
    EXPECT_EQ(r3.unwrap(), Void{});

    EXPECT_TRUE(r4.is_ok());
    EXPECT_TRUE(f_equal(*take(r4).unwrap(), -123.456f));
}

TEST(ResultCreationTest, CreateErrAndBasicQuery)
{
    auto r1 = Result<int, const char *>::Err("ABCXYZ");
    EXPECT_TRUE(r1.is_err());
    EXPECT_EQ(r1.unwrap_err(), "ABCXYZ");

    auto r2 = Result<const char *, std::unique_ptr<int>>::Err(
        std::make_unique<int>(99));
    EXPECT_TRUE(r2.is_err());
    EXPECT_EQ(*take(r2).unwrap_err(), 99);

    auto r3 = Err<float, string>("this is error");
    EXPECT_TRUE(r3.is_err());
    EXPECT_EQ(r3.unwrap_err(), "this is error");

    auto r4 =
        Err<Void, std::unique_ptr<double>>(std::make_unique<double>(-123.456f));
    EXPECT_TRUE(r4.is_err());
    EXPECT_TRUE(f_equal(*take(r4).unwrap_err(), -123.456f));
}

TEST(ResultPredicateTest, OkIsOkAnd)
{
    // Test l-value case
    auto r1 = Result<int, string>::Ok(9999);
    auto b1 = r1.is_ok_and([](const int &value) -> auto { return value > 0; });
    EXPECT_TRUE(b1);

    // Test r-value case
    auto b2 = Result<string, int>::Ok("123").is_ok_and(
        [](const string &str) -> auto { return str.length() > 5; });
    EXPECT_FALSE(b2);
}

TEST(ResultPredicateTest, OkIsErrAnd)
{
    /// Test l-value case
    auto r1 = Result<int, string>::Ok(9999);
    auto b1 = r1.is_err_and(
        [](const string &msg) -> bool { return msg == "Invalid"; });
    EXPECT_FALSE(b1);

    // Test r-value case
    auto b2 = Result<int, Void>::Ok(99).is_err_and(
        [](const Void & /*ignored*/) -> bool { return true; });
    EXPECT_FALSE(b2);
}

TEST(ResultPredicateTest, ErrIsErrAnd)
{
    // Test l-value case
    auto r1 = Result<Void, int>::Err(1);
    auto b1 =
        r1.is_err_and([](const int &value) -> auto { return value != 0; });
    EXPECT_TRUE(b1);

    // Test r-value case
    auto b2 = Result<Void, string>::Err("Invalid").is_err_and(
        [](const string &msg) -> bool { return msg == "Invalid"; });
    EXPECT_TRUE(b2);
}

TEST(ResultPredicateTest, ErrIsOkAnd)
{
    // Test l-value case
    auto r1 = Result<Void, int>::Err(1);
    auto b1 =
        r1.is_ok_and([](const Void & /*ignored*/) -> auto { return true; });
    EXPECT_FALSE(b1);

    // Test r-value case
    auto b2 = Result<Void, int>::Err(999).is_ok_and(
        [](const Void & /*ignored*/) -> auto { return true; });
    EXPECT_FALSE(b2);
}

TEST(ResultOptionalAdaptor, OkToSome)
{
    // Test l-value case
    auto r1 = Result<int, Void>::Ok(100);
    auto o1 = r1.ok();
    EXPECT_TRUE(o1.has_value());
    EXPECT_EQ(o1.value(), 100);

    // Test r-value case
    auto o2 = Result<string, Void>::Ok("abcxyz").ok();
    EXPECT_TRUE(o2.has_value());
    EXPECT_EQ(o2.value(), "abcxyz");
}

TEST(ResultOptionalAdaptor, OkToNone)
{
    // Test l-value case
    auto r1 = Result<int, Void>::Ok(100);
    auto o1 = r1.err();
    EXPECT_FALSE(o1.has_value());

    // Test r-value case
    auto o2 = Result<int, float>::Ok(99).err();
    EXPECT_FALSE(o2.has_value());
}

TEST(ResultOptionalAdaptor, ErrToSome)
{
    // Test l-value case
    auto r1 = Result<Void, int>::Err(50);
    auto o1 = r1.err();
    EXPECT_TRUE(o1.has_value());
    EXPECT_EQ(o1.value(), 50);

    // Test r-value case
    auto o2 = Result<Void, float>::Err(9.9f).err();
    EXPECT_TRUE(o2.has_value());
    EXPECT_TRUE(f_equal(o2.value(), 9.9f));
}

TEST(ResultOptionalAdaptor, ErrToNone)
{
    // Test l-value case
    auto r1 = Result<Void, int>::Err(50);
    auto o1 = r1.ok();
    EXPECT_FALSE(o1.has_value());

    // Test r-value case
    auto o2 = Result<Void, float>::Err(9.9f).ok();
    EXPECT_FALSE(o2.has_value());
}

TEST(ResultTransformTest, OkMap)
{
    // Test l-value case
    auto r1 = Result<int, Void>::Ok(100);
    auto r2 = r1.map([](const int &value) -> double { return value / 7.0f; });

    EXPECT_TRUE(r2.is_ok());
    EXPECT_TRUE(f_equal(r2.unwrap(), 100 / 7.0f));

    // Test r-value case
    auto r3 = Result<int, Void>::Ok(-90).map([](const int &value) -> string {
        if (value > 0) {
            return {"positive"};
        } else if (value < 0) {
            return {"negative"};
        } else {
            return {"zero"};
        }
    });
    EXPECT_TRUE(r3.is_ok());
    EXPECT_EQ(r3.unwrap(), "negative");
}

TEST(ResultTransformTest, ErrMap)
{
    // Test l-value case
    auto r1 = Result<int, Void>::Err(Void{});
    auto r2 = r1.map([](const int &value) -> double { return value / 7.0f; });

    EXPECT_TRUE(r2.is_err());
    EXPECT_EQ(r2.unwrap_err(), Void{});

    // Test r-value case
    auto r3 = Result<int, const char *>::Err("Error").map(
        [](const int &value) -> string {
            if (value > 0) {
                return {"positive"};
            } else if (value < 0) {
                return {"negative"};
            } else {
                return {"zero"};
            }
        });
    EXPECT_TRUE(r3.is_err());
    EXPECT_EQ(r3.unwrap_err(), "Error");
}

TEST(ResultTransformTest, OkMapOr)
{
    // Test l-value case
    auto r1 = Result<int, Void>::Ok(100);
    auto o1 =
        r1.map_or(9999, [](const int &value) -> int { return value / 2; });
    EXPECT_EQ(o1, 50);

    // Test r-value case
    auto o2 = Result<int, Void>::Ok(99).map_or(string("unknown"),
                                               [](const int &val) -> string {
                                                   if (val > 100) {
                                                       return {"above 100"};
                                                   } else {
                                                       return {"below 100"};
                                                   }
                                               });
    EXPECT_EQ(o2, "below 100");
}

TEST(ResultTransformTest, ErrMapOr)
{

    // Test l-value case
    auto r1 = Result<int, Void>::Err({});
    auto o1 =
        r1.map_or(-9999, [](const int &value) -> int { return value / 2; });
    EXPECT_EQ(o1, -9999);

    // Test r-value case
    auto o2 = Result<int, Void>::Err({}).map_or(string("unknown"),
                                                [](const int &val) -> string {
                                                    if (val > 100) {
                                                        return {"above 100"};
                                                    } else {
                                                        return {"below 100"};
                                                    }
                                                });
    EXPECT_EQ(o2, "unknown");
}

TEST(ResultTransformTest, OkMapOrElse)
{
    // Test l-value case
    auto r1 = Result<int, Void>::Ok(1);
    auto o1 = r1.map_or_else([](const auto & /*unused*/)
                                 -> string { return {"Got error"}; },
                             [](const auto &value) -> string {
                                 std::stringstream ss;
                                 ss << "Got value: " << value;
                                 return ss.str();
                             });
    EXPECT_EQ(o1, "Got value: 1");

    // Test r-value case
    auto o2 = Result<int, Void>::Ok(-99)
                  .map_or_else([](const auto & /*unused*/)
                                   -> string { return {"Got error"}; },
                               [](const auto &value) -> string {
                                   std::stringstream os;
                                   os << "Got value: " << value;
                                   return os.str();
                               });
    EXPECT_EQ(o2, "Got value: -99");
}

TEST(ResultTransformTest, ErrMapOrElse)
{
    auto r1 = Result<Void, const char *>::Err("invalid");
    auto o1 = r1.map_or_else(
        [](const auto &err) -> string {
            std::stringstream ss;
            ss << "Got error: " << err;
            return ss.str();
        },
        [](const auto & /*unused*/) -> string { return {"Got value"}; });
    EXPECT_EQ(o1, "Got error: invalid");

    auto o2 = Result<Void, const char *>::Err("Unknown").map_or_else(
        [](const auto &err) -> string {
            std::stringstream ss;
            ss << "Got error: " << err;
            return ss.str();
        },
        [](const auto & /*unsused*/) -> string { return {"Got value"}; });
    EXPECT_EQ(o2, "Got error: Unknown");
}

TEST(ResultTransformTest, OkMapErr)
{
    auto r1 = Result<int, const char *>::Ok(99);
    auto r2 = r1.map_err([](const auto &err) -> auto {
        return string("New err: ") + string(err);
    });
    EXPECT_TRUE(r2.is_ok());
    EXPECT_EQ(r2.unwrap(), 99);

    auto r3 =
        Result<int, const char *>::Ok(99).map_err([](const auto &err) -> auto {
            return string("Newer err: ") + string(err);
        });
    EXPECT_TRUE(r3.is_ok());
    EXPECT_EQ(r3.unwrap(), 99);
}

TEST(ResultTransformTest, ErrMapErr)
{
    auto r1 = Result<int, const char *>::Err("error");
    auto r2 = r1.map_err([](const auto &err) -> auto {
        return string("New err: ") + string(err);
    });
    EXPECT_TRUE(r2.is_err());
    EXPECT_EQ(r2.unwrap_err(), "New err: error");

    auto r3 = Result<int, const char *>::Err("other error")
                  .map_err([](const auto &err) -> auto {
                      return string("Newer err: ") + string(err);
                  });
    EXPECT_TRUE(r3.is_err());
    EXPECT_EQ(r3.unwrap_err(), "Newer err: other error");
}

TEST(ResultTransformTest, OkInspect)
{
    auto r1 = Result<int, Void>::Ok(99);
    EXPECT_DEATH(
        {
            r1.inspect([](const auto &value) -> void {
                EXPECT_EQ(value, 99);
                abort();
            });
        },
        "");

    auto test_lambda = []() -> void {
        Result<int, Void>::Ok(-99).inspect([](const auto &value) -> void {
            EXPECT_EQ(value, -99);
            abort();
        });
    };
    EXPECT_DEATH({ test_lambda(); }, "");
}

TEST(ResultTransformTest, ErrInspect)
{
    auto r1 = Result<int, Void>::Err({});
    EXPECT_NO_FATAL_FAILURE(
        { r1.inspect([](const auto & /*unused*/) -> void { abort(); }); });

    auto test_lambda = []() -> void {
        Result<int, Void>::Err({}).inspect(
            [](const auto & /*unused*/) -> void { abort(); });
    };
    EXPECT_NO_FATAL_FAILURE({ test_lambda(); });
}

TEST(ResultTransformTest, OkInspectErr)
{
    auto r1 = Result<int, Void>::Ok(99);
    EXPECT_NO_FATAL_FAILURE(
        { r1.inspect_err([](const auto & /*unused*/) -> void { abort(); }); });

    auto test_lambda = []() -> void {
        Result<int, Void>::Ok(99).inspect_err(
            [](const auto & /*unused*/) -> void { abort(); });
    };
    EXPECT_NO_FATAL_FAILURE({ test_lambda(); });
}

TEST(ResultTransformTest, ErrInspectErr)
{
    auto r1 = Result<int, const char *>::Err("this is error");
    EXPECT_DEATH(
        {
            r1.inspect_err([](const auto &err) -> void {
                EXPECT_EQ(err, "this is error");
                abort();
            });
        },
        "");

    auto test_lambda = []() -> void {
        Result<int, const char *>::Err("this is another error")
            .inspect_err([](const auto &err) -> void {
                EXPECT_EQ(err, "this is another error");
                abort();
            });
    };
    EXPECT_DEATH({ test_lambda(); }, "");
}

TEST(ResultTransformTest, ChainOperation)
{
    // Chain map test, with r-value
    constexpr int year_of_birth = 1998;
    constexpr int current_year = 2025;
    auto check =
        Result<int, const char *>::Ok(year_of_birth)
            .inspect([year_of_birth](const auto &value) -> void {
                EXPECT_EQ(value, year_of_birth);
            })
            .map([](const int &year) -> int { return current_year - year; })
            .and_then([](const int &age) -> auto {
                if (age < 0 or age > 200) {
                    return Result<int, const char *>::Err("Invalid age");
                }
                return Result<int, const char *>::Ok(age);
            })
            .map([](const int &age) -> string {
                if (age >= 18) {
                    return {"over 18"};
                }
                return {"under 18"};
            });

    EXPECT_TRUE(check.is_ok());
    EXPECT_EQ(check.unwrap(), "over 18");
}
