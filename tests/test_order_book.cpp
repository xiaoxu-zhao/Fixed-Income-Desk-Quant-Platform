#include "TestFramework.hpp"

#include "execution/LimitOrderBook.hpp"

TEST_CASE("Order book best bid and ask update correctly") {
    fi::execution::LimitOrderBook book;
    book.onEvent({0, "ZN", fi::execution::EventType::Add, fi::execution::Side::Bid, 110.00, 20, 1});
    book.onEvent({1, "ZN", fi::execution::EventType::Add, fi::execution::Side::Ask, 110.02, 30, 2});
    book.onEvent({2, "ZN", fi::execution::EventType::Add, fi::execution::Side::Bid, 110.01, 10, 3});
    REQUIRE(book.bestBid().has_value());
    REQUIRE(book.bestAsk().has_value());
    REQUIRE_APPROX(*book.bestBid(), 110.01, 1.0e-12);
    REQUIRE_APPROX(*book.bestAsk(), 110.02, 1.0e-12);
}

TEST_CASE("Cancel and trade events reduce top-of-book depth") {
    fi::execution::LimitOrderBook book;
    book.onEvent({0, "ZN", fi::execution::EventType::Add, fi::execution::Side::Bid, 110.00, 20, 1});
    book.onEvent({1, "ZN", fi::execution::EventType::Add, fi::execution::Side::Ask, 110.02, 30, 2});
    book.onEvent({2, "ZN", fi::execution::EventType::Cancel, fi::execution::Side::Bid, 110.00, 5, 1});
    REQUIRE(book.bidDepth() == 15);
    book.onEvent({3, "ZN", fi::execution::EventType::Trade, fi::execution::Side::Ask, 110.02, 10, 2});
    REQUIRE(book.askDepth() == 20);
    REQUIRE(book.trades().size() == 1);
}

