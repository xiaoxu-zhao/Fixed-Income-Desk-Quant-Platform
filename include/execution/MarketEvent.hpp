#pragma once

#include <string>
#include <vector>

namespace fi::execution {

enum class EventType {
    Add,
    Cancel,
    Trade
};

enum class Side {
    Bid,
    Ask
};

struct MarketEvent {
    long long timestampNs{0};
    std::string symbol;
    EventType eventType{EventType::Add};
    Side side{Side::Bid};
    double price{0.0};
    int size{0};
    long long orderId{0};
};

EventType parseEventType(const std::string& value);
Side parseSide(const std::string& value);
std::string toString(EventType eventType);
std::string toString(Side side);
std::vector<MarketEvent> readMarketEventsCsv(const std::string& path);
void writeMarketEventsCsv(const std::string& path, const std::vector<MarketEvent>& events);

} // namespace fi::execution

