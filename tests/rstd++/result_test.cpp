#include "rstd++/result.hpp"
#include <gtest/gtest.h>

using namespace rstd;

TEST(ResultFactory, OkCreatesOk)
{
    auto r = Ok<int, std::string>(10);
    EXPECT_TRUE(r.is_ok());
    EXPECT_FALSE(r.is_err());
    EXPECT_EQ(r.unwrap(), 10);
}

TEST(ResultFactory, ErrCreatesErr)
{
    auto r = Err<int, std::string>("bad");
    EXPECT_TRUE(r.is_err());
    EXPECT_FALSE(r.is_ok());
    EXPECT_EQ(r.err().value(), "bad");
}

TEST(ResultMap, TransformsOk)
{
    auto r = Ok<int, std::string>(5);
    auto doubled = r.map([](int v) { return v * 2; });

    EXPECT_TRUE(doubled.is_ok());
    EXPECT_EQ(doubled.unwrap(), 10);
}

TEST(ResultMap, PropagatesErr)
{
    auto r = Err<int, std::string>("boom");
    auto out = r.map([](int v) { return v + 1; });

    EXPECT_TRUE(out.is_err());
    EXPECT_EQ(out.err().value(), "boom");
}

TEST(ResultMap, RvalueMove)
{
    std::string err = "oops";
    auto r = Err<std::unique_ptr<int>, std::string>(std::move(err));

    auto out = std::move(r).map([](std::unique_ptr<int> val) { return *val; });

    EXPECT_TRUE(out.is_err());
    EXPECT_EQ(out.err().value(), "oops");
}

TEST(ResultMap, OkNonCopyable)
{
    auto ptr = std::make_unique<int>(42);
    auto r = Ok<std::unique_ptr<int>, std::string>(std::move(ptr));

    auto s = std::move(r).map([](std::unique_ptr<int> p) { return *p; });

    EXPECT_TRUE(s.is_ok());
    EXPECT_EQ(s.unwrap(), 42);
}

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

// TEST(ResultMapOr, OkNonCopyable)
// {
//     auto ptr = std::make_unique<int>(5);
//     auto r = Ok<std::unique_ptr<int>, std::string>(std::move(ptr));
//
//     std::string out =
//         std::move(r).map_or(-1, [](std::unique_ptr<int> p) { return *p; });
//
//     EXPECT_EQ(out, 5);
// }
