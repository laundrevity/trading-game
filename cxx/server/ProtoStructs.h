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