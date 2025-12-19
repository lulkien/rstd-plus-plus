#include "rstd++/result.hpp"
#include <gtest/gtest.h>

using namespace rstd;

/* -------------------------------------------------------------
      Construction helpers
         ------------------------------------------------------------- */
TEST(ResultFactory, OkCreatesOk)
{
    auto r = Ok<int, std::string>(10);
    EXPECT_TRUE(r.is_ok());
    EXPECT_FALSE(r.is_err());
    EXPECT_EQ(r.ok().value(), 10);
}

TEST(ResultFactory, ErrCreatesErr)
{
    auto r = Err<int, std::string>("bad");
    EXPECT_TRUE(r.is_err());
    EXPECT_FALSE(r.is_ok());
    EXPECT_EQ(r.err().value(), "bad");
}

/* -------------------------------------------------------------
      map – transforms Ok, leaves Err unchanged
         ------------------------------------------------------------- */
TEST(ResultMap, TransformsOk)
{
    auto r = Ok<int, std::string>(5);
    auto doubled = r.map([](int v) { return v * 2; });

    EXPECT_TRUE(doubled.is_ok());
    EXPECT_EQ(doubled.ok().value(), 10);
}

TEST(ResultMap, PropagatesErr)
{
    auto r = Err<int, std::string>("boom");
    auto out = r.map([](int v) { return v + 1; });

    EXPECT_TRUE(out.is_err());
    EXPECT_EQ(out.err().value(), "boom");
}

/* -------------------------------------------------------------
      map – rvalue overload (move semantics)
         ------------------------------------------------------------- */
// TEST(ResultMap, RvalueMove)
// {
//     std::string err = "oops";
//     auto r = Err<std::unique_ptr<int>, std::string>(std::move(err));
//
//     // map should not invoke the lambda; error is moved out unchanged
//     auto out = std::move(r).map([](std::unique_ptr<int>) { return 0; });
//
//     EXPECT_TRUE(out.is_err());
//     EXPECT_EQ(out.err().value(), "oops");
// }

/* -------------------------------------------------------------
      map – non‑copyable Ok value
         ------------------------------------------------------------- */
// TEST(ResultMap, OkNonCopyable)
// {
//     auto ptr = std::make_unique<int>(42);
//     auto r = Ok<std::unique_ptr<int>, std::string>(std::move(ptr));
//
//     auto s = r.map([](std::unique_ptr<int> p) { return std::to_string(*p);
//     });
//
//     EXPECT_TRUE(s.is_ok());
//     EXPECT_EQ(s.ok().value(), "42");
// }

/* -------------------------------------------------------------
      map_or – default on Err, apply fn on Ok
         ------------------------------------------------------------- */
TEST(ResultMapOr, ReturnsDefaultOnErr)
{
    auto r = Err<int, std::string>("err");
    int out = r.map_or(99, [](int v) { return v * 2; });
    EXPECT_EQ(out, 99);
}

TEST(ResultMapOr, AppliesFnOnOk)
{
    auto r = Ok<int, std::string>(7);
    int out = r.map_or(0, [](int v) { return v + 3; });
    EXPECT_EQ(out, 10);
}

/* -------------------------------------------------------------
      map_or – rvalue overload (move semantics)
         ------------------------------------------------------------- */
TEST(ResultMapOr, RvalueMove)
{
    std::string err = "bad";
    auto r = Err<std::unique_ptr<int>, std::string>(std::move(err));

    std::string out =
        std::move(r).map_or(std::string("default"), [](std::unique_ptr<int>) {
            return std::string("won't");
        });

    EXPECT_EQ(out, "default");
}

/* -------------------------------------------------------------
      map_or – non‑copyable Ok value
         ------------------------------------------------------------- */
// TEST(ResultMapOr, OkNonCopyable)
// {
//     auto ptr = std::make_unique<int>(5);
//     auto r = Ok<std::unique_ptr<int>, std::string>(std::move(ptr));
//
//     std::string out = r.map_or(std::string("none"), [](std::unique_ptr<int>
//     p) {
//         return std::to_string(*p);
//     });
//
//     EXPECT_EQ(out, "5");
// }
