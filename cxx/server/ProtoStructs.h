#pragma once
#include <cstdint>

struct Header {
    int32_t size;
    uint32_t aux;
};

struct FrameRef {
    FrameRef(Header* h, char* d) : header(h), data(d) {}
    Header* header;
    char* data;
};

enum class MessageKind {
    BUSINESS = 0x00,
    OTHER = 0x01
};

Header BuildHeader(MessageKind message_kind, const std::string& data) {
    const auto size = static_cast<int32_t>(data.length());
    const auto aux = static_cast<uint32_t>(message_kind);
    return Header{size, aux};
}