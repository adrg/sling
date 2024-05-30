/*
The MIT License (MIT)
Copyright (c) 2019 Adrian-George Bostan <adrg@epistack.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef SLING_SIGSLOT_H
#define SLING_SIGSLOT_H

#ifndef SLING_SKIP_DEFS
#define signals
#define slots
#endif

#include <functional>
#include <map>

namespace sl
{
    using SlotKey = std::size_t;

    template<typename ...Args>
    class Signal;

    template<typename ...Args>
    class Slot
    {
    private:
        struct Connection
        {
            Signal<Args...>* signal;
            Slot<Args...>* slot;
            SlotKey key;
            bool managed;

            Connection(Signal<Args...>* signal, Slot<Args...>* slot,
                SlotKey key, bool managed) :
                signal(signal), slot(slot), key(key), managed(managed) {
            }

            void releaseSlot() {
                this->slot->connection = nullptr;
                if (this->managed) {
                    delete this->slot;
                }
            }
        };

        std::function<void(Args...)> callback;
        Connection* connection = nullptr;

        void move(Slot* slot) {
            this->callback = std::move(slot->callback);
            this->connection = nullptr;

            if (slot->connection != nullptr) {
                this->connection = slot->connection;
                slot->connection->releaseSlot();
                this->connection->slot = this;
                this->connection->managed = false;
            }
        }
    public:
        Slot() : callback() {
        }

        Slot(const std::function<void(Args...)>& f) : callback(f) {
        }

        Slot(std::function<void(Args...)>&& f) : callback(f) {
        }

        template <class T>
        Slot(T* target, void (T::*method)(Args...)) {
            setCallback(target, method);
        }

        template <class T>
        Slot(T* target, void (T::*method)(Args...) const) {
            setCallback(target, method);
        }

        Slot(const Slot& slot) : callback(slot.callback) {
        }

        Slot(Slot&& slot) {
            this->move(&slot);
        }

        virtual ~Slot() {
            this->disconnect();
        }

        Slot& operator = (const Slot& slot) {
            this->callback = slot.callback;
            return *this;
        }

        Slot& operator = (Slot&& slot) {
            this->disconnect();
            this->move(&slot);
            return *this;
        }

        void disconnect() {
            if (this->connection != nullptr) {
                this->connection->signal->disconnect(this);
            }
        }

        void setCallback(const std::function<void(Args...)>& f) {
            this->callback = f;
        }

        void setCallback(std::function<void(Args...)>&& f) {
            this->callback = f;
        }

        template <class T>
        void setCallback(T* target, void (T::*method)(Args...)) {
            this->callback = [target, method](Args... args) {
                (target->*method)(args...);
            };
        }

        template <class T>
        void setCallback(T* target, void (T::*method)(Args...) const) {
            this->callback = [target, method](Args... args) {
                (target->*method)(args...);
            };
        }

        friend class Signal<Args...>;
    };

    template<typename ...Args>
    class Signal
    {
    private:
        using Connection = typename Slot<Args...>::Connection;

        std::map<std::size_t, Connection> connections;
        SlotKey sequence;

        SlotKey connect(Slot<Args...>* slot, bool managed) {
            if (slot == nullptr) {
                return 0;
            }
            if (slot->connection != nullptr) {
                if (slot->connection->signal == this) {
                    return slot->connection->key;
                }
                slot->disconnect();
            }

            ++this->sequence;
            auto res = this->connections.emplace(
                std::make_pair(
                    this->sequence,
                    Connection(this, slot, this->sequence, managed)
                )
            );

            slot->connection = &res.first->second;
            return this->sequence;
        }

        void move(Signal* signal) {
            this->clear();
            this->connections = std::move(signal->connections);
            this->sequence = signal->sequence;

            for (auto& connection : this->connections) {
                connection.second.signal = this;
            }

            signal->sequence = 0;
            signal->connections.clear();
        }
    public:
        Signal() : connections(), sequence(0) {
        }

        Signal(const Signal& signal) = delete;

        Signal(Signal&& signal) {
            this->move(&signal);
        }

        virtual ~Signal() {
            this->clear();
        }

        Signal& operator = (const Signal& signal) = delete;

        Signal& operator = (Signal&& signal) {
            this->move(&signal);
            return *this;
        }

        void operator() (Args... args) const {
            this->emit(args...);
        }

        SlotKey connect(Slot<Args...>* slot) {
            return this->connect(slot, false);
        }

        SlotKey connect(Slot<Args...>& slot) {
            return this->connect(&slot, false);
        }

        SlotKey connect(Slot<Args...>&& slot) {
            return this->connect(new Slot<Args...>(std::move(slot)), true);
        }

        void disconnect(SlotKey key) {
            auto it = this->connections.find(key);
            if (it != this->connections.end()) {
                it->second.releaseSlot();
                this->connections.erase(it);
            }
        }

        void disconnect(Slot<Args...>* slot) {
            if (slot != nullptr && slot->connection != nullptr &&
                slot->connection->signal == this) {
                this->disconnect(slot->connection->key);
            }
        }

        void disconnect(Slot<Args...>& slot) {
            this->disconnect(&slot);
        }

        void clear() {
            for (auto& conn : this->connections) {
                conn.second.releaseSlot();
            }
            this->connections.clear();
        }

        void emit(Args... args) const {
            for (const auto& conn : this->connections) {
                if (conn.second.slot->callback) {
                    conn.second.slot->callback(std::forward<Args>(args)...);
                }
            }
        }
    };
}

#endif
