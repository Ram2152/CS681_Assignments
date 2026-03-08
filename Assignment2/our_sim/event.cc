#include "common.hh"

Event::Event(float timestamp, EventType type) {
    this->timestamp = timestamp;
    this->type = type;
}