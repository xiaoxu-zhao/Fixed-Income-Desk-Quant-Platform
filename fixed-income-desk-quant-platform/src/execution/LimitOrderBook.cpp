#include "execution/LimitOrderBook.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace fi::execution {

void LimitOrderBook::addDepth(Side side, double price, int size) {
    if (size <= 0) {
        return;
    }
    if (side == Side::Bid) {
        bids_[price] += size;
    } else {
        asks_[price] += size;
    }
}

void LimitOrderBook::reduceDepth(Side side, double price, int size) {
    if (size <= 0) {
        return;
    }
    auto reduce = [price, size](auto& levels) {
        auto it = levels.find(price);
        if (it == levels.end()) {
            return;
        }
        if (it->second <= size) {
            levels.erase(it);
        } else {
            it->second -= size;
        }
    };
    if (side == Side::Bid) {
        reduce(bids_);
    } else {
        reduce(asks_);
    }
}

int LimitOrderBook::levelDepth(Side side, double price) const {
    if (side == Side::Bid) {
        const auto it = bids_.find(price);
        return it == bids_.end() ? 0 : it->second;
    }
    const auto it = asks_.find(price);
    return it == asks_.end() ? 0 : it->second;
}

void LimitOrderBook::onEvent(const MarketEvent& event) {
    if (event.size <= 0) {
        throw std::invalid_argument("MarketEvent size must be positive");
    }

    if (event.eventType == EventType::Add) {
        addDepth(event.side, event.price, event.size);
        orders_[event.orderId] = {event.side, event.price, event.size};
        return;
    }

    Side side = event.side;
    double price = event.price;
    int reduceSize = event.size;
    const auto orderIt = orders_.find(event.orderId);
    if (orderIt != orders_.end()) {
        side = orderIt->second.side;
        price = orderIt->second.price;
        reduceSize = std::min(event.size, orderIt->second.size);
        orderIt->second.size -= reduceSize;
        if (orderIt->second.size == 0) {
            orders_.erase(orderIt);
        }
    }

    reduceDepth(side, price, reduceSize);
    if (event.eventType == EventType::Trade) {
        trades_.push_back({event.timestampNs, side, price, reduceSize});
    }
}

std::optional<double> LimitOrderBook::bestBid() const {
    if (bids_.empty()) {
        return std::nullopt;
    }
    return bids_.begin()->first;
}

std::optional<double> LimitOrderBook::bestAsk() const {
    if (asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->first;
}

double LimitOrderBook::midPrice() const {
    const auto bid = bestBid();
    const auto ask = bestAsk();
    if (!bid || !ask) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return 0.5 * (*bid + *ask);
}

double LimitOrderBook::spread() const {
    const auto bid = bestBid();
    const auto ask = bestAsk();
    if (!bid || !ask) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return *ask - *bid;
}

int LimitOrderBook::bidDepth() const {
    const auto bid = bestBid();
    return bid ? levelDepth(Side::Bid, *bid) : 0;
}

int LimitOrderBook::askDepth() const {
    const auto ask = bestAsk();
    return ask ? levelDepth(Side::Ask, *ask) : 0;
}

TopOfBook LimitOrderBook::snapshot(long long timestampNs) const {
    return {timestampNs, bestBid(), bestAsk(), midPrice(), spread(), bidDepth(), askDepth()};
}

} // namespace fi::execution

