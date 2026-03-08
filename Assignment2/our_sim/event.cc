#include "common.hh"

Event::Event(float timestamp, EventType type, Request* request, Thread* thread) {
    this->timestamp = timestamp;
    this->type = type;
    this->request = request;
    this->thread = thread;
}