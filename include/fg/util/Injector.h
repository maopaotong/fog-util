/*
 * SPDX-FileCopyrightText: 2025 Mao-Pao-Tong Workshop
 * SPDX-License-Identifier: MPL-2.0
 */
#pragma once
#include "Component.h"

namespace fog
{
    struct Injector
    {
        Injector()
        {
        }

        void bindComp(Component comp)
        {
            return ij.bindComp(comp);
        }

        template <typename T, typename F>
        void bindFunc(F &&ptrFunc)
        {
            bindComp(Component::make(typeid(T), ptrFunc, {}, {}));
        }

        template <typename T>
        void bindImpl()
        {
            bindComp(Component::make<T, T, std::tuple<>>(ij));
        }

        //
        template <typename T, typename Imp>
        void bindImpl()
        {
            bindComp(Component::make<T, Imp, std::tuple<>>(ij));
        }

        template <typename T, typename Imp, typename T1>
        void bindImpl()
        {
            bindComp(Component::make<T, Imp, std::tuple<T1>>(ij));
        }

        template <typename... T>
        void bindAllImpl()
        {
            ((bindImpl<T>()), ...);
        }

        template <typename T>
        void bindPtr(T *obj)
        {
            bindFunc<T>([obj]() -> T *
                        { return obj; });
        }

        template <typename T, typename Imp, typename T1, typename T2>
        void bindImpl()
        {
            bindComp(Component::make<T, Imp, T1, T2>(ij));
        }
        //
        template <typename T, typename OT>
        void bindMethod(T *(OT::*method)())
        {
            bindFunc<T>([this, method]()
                        {
                            OT *obj = this->get<OT>();
                            return (obj->*method)(); //
                        });
        }

        template <typename T, typename OT>
        void bindMember(T *OT::*member)
        {
            bindFunc<T>([this, member]() -> T *
                        {
                            OT *obj = this->get<OT>();
                            return obj->*member; //
                        });
        }
        //

        //
        template <typename T, Component::Usage usgR = Component::AsStatic>
        T *get()
        {
            return ij.getComponent(typeid(T)).get<T>(usgR);
        }

        template <typename T>
        bool hasBind()
        {
            auto it = factories.find(typeid(T));
            return it != factories.end();
        }

        template <typename T, typename C, typename F>
        void bindArgOfConstructor(F &&func)
        {
            bindFunc<ArgOfConstructor<T, C>>([func]() -> ArgOfConstructor<T, C> *
                                             {
                                                 static ArgOfConstructor<T, C> instance(func);
                                                 return &instance; //
                                             });
        }
        //
    private:
    private:
        struct IJ
        {
            std::unordered_map<std::type_index, Component> components;

            IJ()
            {
            }
            IJ(const IJ &) = delete;
            IJ &operator=(const IJ &) = delete;

            IJ(IJ &&) = delete;
            IJ &operator=(IJ &&) = delete;

            const Component *operator()(std::type_index tid) const
            {

                if (auto it = components.find(tid); it != components.end())
                {
                    return &it->second;
                }
                return nullptr;
            }

            void bindComp(Component comp)
            {
                if (components.find(comp.typeId) != components.end())
                {
                    throw std::runtime_error("type id already bond, cannot bind, you may need rebind method.");
                }
                components.emplace(comp.typeId, comp);
            }

            Component &getComponent(std::type_index tid)
            {
                if (auto it = components.find(tid); it != components.end())
                {
                    return it->second;
                }
                throw std::runtime_error("must bind before get the instance by type from Injector.");
            }
        };

        IJ ij;
    };
};