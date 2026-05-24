#include "execution/MarketEvent.hpp"

#include "core/CsvReader.hpp"
#include "core/Utils.hpp"

#include <stdexcept>

namespace fi::execution {

EventType parseEventType(const std::string& value) {
    const auto clean = fi::core::toLower(fi::core::trim(value));
    if (clean == "add") {
        return EventType::Add;
    }
    if (clean == "cancel") {
        return EventType::Cancel;
    }
    if (clean == "trade") {
        return EventType::Trade;
    }
    throw std::invalid_argument("Unknown market event type: " + value);
}

Side parseSide(const std::string& value) {
    const auto clean = fi::core::toLower(fi::core::trim(value));
    if (clean == "bid" || clean == "buy") {
        return Side::Bid;
    }
    if (clean == "ask" || clean == "sell" || clean == "offer") {
        return Side::Ask;
    }
    throw std::invalid_argument("Unknown market side: " + value);
}

std::string toString(EventType eventType) {
    switch (eventType) {
        case EventType::Add:
            return "ADD";
        case EventType::Cancel:
            return "CANCEL";
        case EventType::Trade:
            return "TRADE";
    }
    return "UNKNOWN";
}

std::string toString(Side side) {
    return side == Side::Bid ? "BID" : "ASK";
}

std::vector<MarketEvent> readMarketEventsCsv(const std::string& path) {
    const auto rows = fi::core::CsvReader::readRows(path);
    std::vector<MarketEvent> events;
    events.reserve(rows.size());
    for (const auto& row : rows) {
        events.push_back({
            fi::core::toLongLong(row.at("timestamp_ns"), "timestamp_ns"),
            row.at("symbol"),
            parseEventType(row.at("event_type")),
            parseSide(row.at("side")),
            fi::core::toDouble(row.at("price"), "price"),
            fi::core::toInt(row.at("size"), "size"),
            fi::core::toLongLong(row.at("order_id"), "order_id")
        });
    }
    return events;
}

void writeMarketEventsCsv(const std::string& path, const std::vector<MarketEvent>& events) {
    std::vector<std::vector<std::string>> rows;
    rows.reserve(events.size());
    for (const auto& event : events) {
        rows.push_back({
            std::to_string(event.timestampNs),
            event.symbol,
            toString(event.eventType),
            toString(event.side),
            fi::core::formatDouble(event.price, 6),
            std::to_string(event.size),
            std::to_string(event.orderId)
        });
    }
    fi::core::CsvReader::writeRows(
        path,
        {"timestamp_ns", "symbol", "event_type", "side", "price", "size", "order_id"},
        rows);
}

} // namespace fi::execution

