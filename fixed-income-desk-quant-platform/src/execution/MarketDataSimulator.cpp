#include "execution/MarketDataSimulator.hpp"

#include <algorithm>
#include <cmath>
#include <random>

namespace fi::execution {
namespace {

struct ActiveOrder {
    long long orderId{0};
    Side side{Side::Bid};
    double price{0.0};
    int size{0};
};

} // namespace

MarketDataSimulator::MarketDataSimulator(unsigned int seed) : seed_(seed) {}

std::vector<MarketEvent> MarketDataSimulator::generate(const std::string& symbol,
                                                       int eventCount,
                                                       double startPrice,
                                                       double tickSize) const {
    std::mt19937 rng(seed_);
    std::uniform_int_distribution<int> stepDist(-1, 1);
    std::uniform_int_distribution<int> sizeDist(5, 40);
    std::uniform_int_distribution<int> actionDist(0, 99);

    std::vector<MarketEvent> events;
    events.reserve(static_cast<std::size_t>(std::max(eventCount, 0)));
    std::vector<ActiveOrder> active;
    active.reserve(static_cast<std::size_t>(eventCount));

    long long timestamp = 0;
    long long orderId = 10000;
    int midTicks = static_cast<int>(std::round(startPrice / tickSize));

    auto addOrder = [&](Side side, double price, int size) {
        const long long id = ++orderId;
        events.push_back({timestamp, symbol, EventType::Add, side, price, size, id});
        active.push_back({id, side, price, size});
    };

    addOrder(Side::Bid, (midTicks - 1) * tickSize, 25);
    addOrder(Side::Ask, (midTicks + 1) * tickSize, 30);

    while (static_cast<int>(events.size()) < eventCount) {
        timestamp += 1'000'000;
        if (actionDist(rng) < 35) {
            midTicks += stepDist(rng);
        }

        const double bid = (midTicks - 1) * tickSize;
        const double ask = (midTicks + 1) * tickSize;
        for (auto& order : active) {
            if (static_cast<int>(events.size()) >= eventCount) {
                break;
            }
            const bool wouldCross = (order.side == Side::Bid && order.price >= ask) ||
                                    (order.side == Side::Ask && order.price <= bid);
            if (order.size > 0 && wouldCross) {
                events.push_back({timestamp, symbol, EventType::Cancel,
                                  order.side, order.price, order.size, order.orderId});
                order.size = 0;
            }
        }
        active.erase(std::remove_if(active.begin(), active.end(), [](const ActiveOrder& order) {
            return order.size <= 0;
        }), active.end());
        if (static_cast<int>(events.size()) >= eventCount) {
            break;
        }
        addOrder(Side::Bid, bid, sizeDist(rng));
        if (static_cast<int>(events.size()) >= eventCount) {
            break;
        }
        addOrder(Side::Ask, ask, sizeDist(rng));
        if (static_cast<int>(events.size()) >= eventCount) {
            break;
        }

        active.erase(std::remove_if(active.begin(), active.end(), [](const ActiveOrder& order) {
            return order.size <= 0;
        }), active.end());

        if (!active.empty() && actionDist(rng) < 45 && static_cast<int>(events.size()) < eventCount) {
            std::uniform_int_distribution<std::size_t> pick(0, active.size() - 1);
            auto& order = active[pick(rng)];
            const int cancelSize = std::min(order.size, std::max(1, sizeDist(rng) / 2));
            events.push_back({timestamp + 250'000, symbol, EventType::Cancel,
                              order.side, order.price, cancelSize, order.orderId});
            order.size -= cancelSize;
        }

        active.erase(std::remove_if(active.begin(), active.end(), [](const ActiveOrder& order) {
            return order.size <= 0;
        }), active.end());

        if (!active.empty() && actionDist(rng) < 35 && static_cast<int>(events.size()) < eventCount) {
            std::uniform_int_distribution<std::size_t> pick(0, active.size() - 1);
            auto& order = active[pick(rng)];
            const int tradeSize = std::min(order.size, std::max(1, sizeDist(rng) / 3));
            events.push_back({timestamp + 500'000, symbol, EventType::Trade,
                              order.side, order.price, tradeSize, order.orderId});
            order.size -= tradeSize;
        }
    }
    return events;
}

void MarketDataSimulator::generateToCsv(const std::string& path,
                                        const std::string& symbol,
                                        int eventCount,
                                        double startPrice,
                                        double tickSize) const {
    writeMarketEventsCsv(path, generate(symbol, eventCount, startPrice, tickSize));
}

} // namespace fi::execution
