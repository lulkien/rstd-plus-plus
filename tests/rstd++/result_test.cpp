/**
 * @file test_result.cpp
 * @brief Unit tests for Result<T, E> type using Google Test
 */

#include "rstd++/core.hpp"
#include "rstd++/result.hpp"
#include <cmath>
#include <gtest/gtest.h>
#include <memory>
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

TEST(ResultTransformTest, OkMapOk)
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

    // Chain map test
    auto year_of_birth = Result<int, Void>::Ok(1998);
    auto over_18 =
        year_of_birth.map([](const int &yob) -> int { return 2025 - yob; })
            .map([](const int &age) -> string {
                if (age >= 18) {
                    return {"over 18"};
                }
                return {"under 18"};
            });

    EXPECT_TRUE(over_18.is_ok());
    EXPECT_EQ(over_18.unwrap(), "over 18");
}

//
// TEST(ResultTransformTest, MapOk)
// {
//     auto r = Ok<int, string>(42);
//     auto r2 = r.map([](int x) -> int { return x * 2; });
//     EXPECT_TRUE(r2.is_ok());
//     EXPECT_EQ(r2.unwrap(), 84);
// }
//
// TEST(ResultTransformTest, MapOkNonCopy)
// {
//     auto r1 = Ok<NonCopyableValue, string>(50);
//     auto r2 = take(r1).map([](NonCopyableValue &&v) -> NonCopyableValue {
//     return {v.value * 2}; }); EXPECT_TRUE(r2.is_ok());
//     EXPECT_EQ(take(r2).unwrap(), 100);
// }
//
// TEST(ResultTransformTest, MapErr)
// {
//     auto r = Err<int, string>("MapErr");
//     auto r2 = r.map([](int x) -> int { return x * 2; });
//     EXPECT_TRUE(r2.is_err());
//     EXPECT_EQ(r2.unwrap_err(), "MapErr");
// }
//
// TEST(ResultTransformTest, MapErrNonCopy)
// {
//     auto r1 = Err<int, NonCopyableError>("MapErrNonCopy");
//     auto r2 = take(r1).map([](int x) -> int { return x * 2; });
//     EXPECT_TRUE(r2.is_err());
//     EXPECT_EQ(take(r2).unwrap_err(), "MapErrNonCopy");
// }
//
// TEST(ResultTransformTest, MapToNewType)
// {
//     auto r = Ok<int, const char *>(42);
//     auto r2 = r.map([](int x) -> float { return 5.0f; });
//     EXPECT_TRUE(r2.is_ok());
//     EXPECT_TRUE(f_equal(r2.unwrap(), 5.0f));
// }
//
// TEST(ResultTransformTest, MapVoidToValue)
// {
//     auto r = Ok<Void, const char *>(Void{});
//     auto r2 = r.map([](const Void &) -> int { return 100; });
//     EXPECT_TRUE(r2.is_ok());
//     EXPECT_EQ(r2.unwrap(), 100);
// }
//
// TEST(ResultTransformTest, MapOrWithOk)
// {
//     auto r = Ok<int, const char *>(42);
//     auto result = r.map_or(0, [](int x) -> int { return x * 2; });
//     EXPECT_EQ(result, 84);
// }
//
// TEST(ResultTransformTest, MapOrWithErr)
// {
//     auto r = Err<int, const char *>("MapOrWithErr");
//     auto result = r.map_or(0, [](int x) -> int { return x * 2; });
//     EXPECT_EQ(result, 0);
// }
//
// TEST(ResultTransformTest, MapOrWithVoid)
// {
//     auto r1 = Ok<Void, const char *>(Void{});
//     auto result1 = r1.map_or(0, [](const Void &) -> int { return 42; });
//     EXPECT_EQ(result1, 42);
//
//     auto r2 = Err<Void, const char *>("MapOrWithVoid");
//     auto result2 = r2.map_or(0, [](const Void &) -> int { return 42; });
//     EXPECT_EQ(result2, 0);
// }
//
// // ======================================================================
// // Extraction Tests
// // ======================================================================
//
// TEST(ResultExtractionTest, ExpectOk)
// {
//     auto r = Ok<int, const char *>(42);
//     EXPECT_EQ(r.expect("should be ok"), 42);
// }
//
// TEST(ResultExtractionTest, ExpectFailed)
// {
//     auto r = Err<int, string>("BBBB");
//     EXPECT_THROW(
//         {
//             try {
//                 r.expect("AAAA");
//             } catch (const std::runtime_error &e) {
//                 auto check = string{e.what()}.compare("AAAA: BBBB") ==
//                 0; EXPECT_TRUE(check); throw;
//             }
//         },
//         std::runtime_error);
// }
//
// TEST(ResultExtractionTest, ExpectWithVoid)
// {
//     auto r = Ok<Void, const char *>(Void{});
//     EXPECT_NO_THROW(r.expect("should be ok"));
// }
//
// TEST(ResultExtractionTest, UnwrapOk)
// {
//     auto r = Ok<int, const char *>(42);
//     EXPECT_EQ(r.unwrap(), 42);
// }
//
// TEST(ResultExtractionTest, UnwrapFailed)
// {
//     auto r = Err<int, const char *>("AAAA");
//     EXPECT_THROW(
//         {
//             try {
//                 r.unwrap();
//             } catch (const std::runtime_error &e) {
//                 auto check = string{e.what()}.compare(
//                                  "called `Result::unwrap()` on an `Err`
//                                  value: AAAA") == 0;
//                 EXPECT_TRUE(check);
//                 throw;
//             }
//         },
//         std::runtime_error);
// }
//
// TEST(ResultExtractionTest, UnwrapWithVoid)
// {
//     auto r = Ok<Void, string>(Void{});
//     EXPECT_NO_THROW(r.unwrap());
// }
//
// TEST(ResultExtractionTest, UnwrapOrDefault)
// {
//     auto r1 = Ok<int, string>(42);
//     EXPECT_EQ(r1.unwrap_or_default(), 42);
//
//     auto r2 = Err<int, string>("AAAA");
//     EXPECT_EQ(r2.unwrap_or_default(), 0);
// }
//
// TEST(ResultExtractionTest, ExpectErrOk)
// {
//     auto r = Err<int, string>("AAAA");
//     EXPECT_EQ(r.expect_err("should be err"), "AAAA");
// }
//
// TEST(ResultExtractionTest, ExpectErrFailed)
// {
//     auto r = Ok<int, const char *>(100);
//     EXPECT_THROW(
//         {
//             try {
//                 r.expect_err("AAAA");
//             } catch (const std::runtime_error &e) {
//                 auto check = string{e.what()}.compare("AAAA: 100") == 0;
//                 EXPECT_TRUE(check);
//                 throw;
//             }
//         },
//         std::runtime_error);
// }
//
// TEST(ResultExtractionTest, ExpectErrWithVoid)
// {
//     auto r = Err<int, Void>(Void{});
//     EXPECT_NO_THROW(r.expect_err("should be err"));
// }
//
// TEST(ResultExtractionTest, UnwrapErrOk)
// {
//     auto r = Err<int, string>("AAAA");
//     EXPECT_EQ(r.unwrap_err(), "AAAA");
// }
//
// TEST(ResultExtractionTest, UnwrapErrFailed)
// {
//     auto r = Ok<int, const char *>(100);
//     EXPECT_THROW(
//         {
//             try {
//                 r.unwrap_err();
//             } catch (const std::runtime_error &e) {
//                 auto check = string{e.what()}.compare(
//                     "called `Result::unwrap_err()` on an `Ok` value: 100") ==
//                     0;
//                 EXPECT_TRUE(check);
//                 throw;
//             }
//         },
//         std::runtime_error);
// }
//
// TEST(ResultExtractionTest, UnwrapErrWithVoid)
// {
//     auto r = Err<int, Void>(Void{});
//     EXPECT_NO_THROW(r.unwrap_err());
// }
//
// // ======================================================================
// // Void Type Tests
// // ======================================================================
//
// TEST(VoidTest, Equality)
// {
//     Void v1;
//     Void v2;
//     EXPECT_EQ(v1, v2);
//     EXPECT_FALSE(v1 != v2);
// }
//
// TEST(VoidTest, Printable)
// {
//     std::ostringstream oss;
//     oss << Void{};
//     EXPECT_EQ(oss.str(), "()");
// }
//
// // ======================================================================
// // Complex Type Tests
// // ======================================================================
//
// TEST(ResultComplexTest, NonCopyableType)
// {
//     auto r = Ok<NonCopyableValue, string>(NonCopyableValue(42));
//     EXPECT_TRUE(r.is_ok());
//     EXPECT_EQ(std::move(r).unwrap().value, 42);
// }
//
// struct CustomError
// {
//     int code;
//     string message;
//
//     auto operator==(const CustomError &other) const -> bool
//     {
//         return code == other.code && message == other.message;
//     }
// };
//
// auto operator<<(std::ostream &os, const CustomError &e) -> std::ostream &
// {
//     return os << "Error(" << e.code << ", " << e.message << ")";
// }
//
// TEST(ResultComplexTest, CustomErrorType)
// {
//     auto r = Err<int, CustomError>(CustomError{.code = 404, .message = "Not
//     found"}); EXPECT_TRUE(r.is_err()); CustomError e = r.unwrap_err();
//     EXPECT_EQ(e.code, 404);
//     EXPECT_EQ(e.message, "Not found");
// }
//
// TEST(ResultComplexTest, CustomErrorInException)
// {
//     auto r = Err<int, CustomError>(CustomError{.code = 500, .message =
//     "Server error"}); EXPECT_THROW(
//         {
//             try {
//                 r.unwrap();
//             } catch (const std::runtime_error &e) {
//                 string msg = e.what();
//                 EXPECT_NE(msg.find("Error(500, Server error)"),
//                 string::npos); throw;
//             }
//         },
//         std::runtime_error);
// }
