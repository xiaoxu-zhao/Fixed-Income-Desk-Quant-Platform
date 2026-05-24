#pragma once

#include "execution/MarketEvent.hpp"

#include <functional>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

namespace fi::execution {

struct TopOfBook {
    long long timestampNs{0};
    std::optional<double> bestBid;
    std::optional<double> bestAsk;
    double midPrice{0.0};
    double spread{0.0};
    int bidDepth{0};
    int askDepth{0};
};

struct TradeRecord {
    long long timestampNs{0};
    Side restingSide{Side::Ask};
    double price{0.0};
    int size{0};
};

class LimitOrderBook {
public:
    void onEvent(const MarketEvent& event);
    std::optional<double> bestBid() const;
    std::optional<double> bestAsk() const;
    double midPrice() const;
    double spread() const;
    int bidDepth() const;
    int askDepth() const;
    TopOfBook snapshot(long long timestampNs) const;
    const std::vector<TradeRecord>& trades() const { return trades_; }

private:
    struct OrderInfo {
        Side side{Side::Bid};
        double price{0.0};
        int size{0};
    };

    void addDepth(Side side, double price, int size);
    void reduceDepth(Side side, double price, int size);
    int levelDepth(Side side, double price) const;

    std::map<double, int, std::greater<double>> bids_;
    std::map<double, int> asks_;
    std::unordered_map<long long, OrderInfo> orders_;
    std::vector<TradeRecord> trades_;
};

} // namespace fi::execution

