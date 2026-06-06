#include "event_bus.h"
#include <string.h>

static EventCallback s_subscribers[EVT_COUNT][EVT_MAX_SUBSCRIBERS];
static int s_subscriberCount[EVT_COUNT];

void EventBus_Init(void) {
    memset(s_subscribers, 0, sizeof(s_subscribers));
    memset(s_subscriberCount, 0, sizeof(s_subscriberCount));
}

void EventBus_Subscribe(EventType type, EventCallback callback) {
    if (type >= EVT_COUNT || !callback) return;
    int idx = s_subscriberCount[type];
    if (idx >= EVT_MAX_SUBSCRIBERS) return;
    s_subscribers[type][idx] = callback;
    s_subscriberCount[type] = idx + 1;
}

void EventBus_Publish(EventType type, void* data) {
    if (type >= EVT_COUNT) return;
    for (int i = 0; i < s_subscriberCount[type]; i++) {
        if (s_subscribers[type][i]) {
            s_subscribers[type][i](data);
        }
    }
}

void EventBus_Clear(void) {
    EventBus_Init();
}
