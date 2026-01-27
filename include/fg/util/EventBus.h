/*
 * SPDX-FileCopyrightText: 2025 Mao-Pao-Tong Workshop
 * SPDX-License-Identifier: MPL-2.0
 */
#pragma once
#include "Common.h"
#include <fg/util/Component.h>

namespace fog
{

    class EventBus
    {
    public:
        INJECT(EventBus())
        {
        }
        template <typename... Args, typename F>
        void subscribe(F &&func)
        {
            //
            getCallbacks<Args...>().emplace_back(std::forward<F>(func));
        }

        template <typename... Args>
        void emit(Args... args)
        {
            for (auto &cb : getCallbacks<Args...>())
            {
                cb(args...);
            }
        }

    private:
        struct CallbackBase
        {
            virtual ~CallbackBase() = default;
        };

        template <typename... Args>
        struct CallbackList : CallbackBase
        {
            std::vector<std::function<void(Args...)>> callbacks;
        };

        std::unordered_map<std::type_index, std::unique_ptr<CallbackBase>> registry;

        template <typename... Args>
        std::vector<std::function<void(Args...)>> &getCallbacks()
        {
            std::type_index ti = std::type_index(typeid(std::tuple<Args...>));
            auto it = registry.find(ti);
            if (it == registry.end())
            {
                auto list = std::make_unique<CallbackList<Args...>>();
                auto &ref = list->callbacks;
                registry[ti] = std::move(list);
                return ref;
            }
            return static_cast<CallbackList<Args...> *>(it->second.get())->callbacks;
        }
    };
};
