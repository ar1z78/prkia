#ifndef SIGNALS_H
#define SIGNALS_H

#include <functional>
#include <vector>
#include <memory>
#include <algorithm>

namespace aoia {

    
    class SignalConnection {
    public:
        SignalConnection() : m_connected(std::make_shared<bool>(false)) {}
        SignalConnection(std::shared_ptr<bool> flag) : m_connected(flag) {}

        void disconnect() { if (m_connected) *m_connected = false; }
        bool is_connected() const { return m_connected && *m_connected; }

    private:
        std::shared_ptr<bool> m_connected;
    };

    template<typename Signature>
    class Signal;

    template<typename RET, typename... ARGS>
    class Signal<RET(ARGS...)> {
    public:
        using SlotType = std::function<RET(ARGS...)>;
        // Matches the 'slot_function_type' expected by legacy code
        using slot_function_type = SlotType;

        SignalConnection connect(SlotType slot) {
            auto flag = std::make_shared<bool>(true);
            m_slots.push_back({slot, flag});
            return SignalConnection(flag);
        }

        void operator()(ARGS... args) {
            // Cleanup dead slots
            m_slots.erase(std::remove_if(m_slots.begin(), m_slots.end(),
                [](const SlotEntry& s) { return !*s.flag; }), m_slots.end());

            for (auto& s : m_slots) {
                if (*s.flag) s.func(args...);
            }
        }

        void disconnect_all() {
            for (auto& s : m_slots) { *s.flag = false; }
            m_slots.clear();
        }

    private:
        struct SlotEntry {
            SlotType func;
            std::shared_ptr<bool> flag;
        };
        std::vector<SlotEntry> m_slots;
    };
}

#endif
