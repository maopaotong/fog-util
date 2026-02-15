/*
 * SPDX-FileCopyrightText: 2025 Mao-Pao-Tong Workshop
 * SPDX-License-Identifier: MPL-2.0
 */
#pragma once

#include "Common.h"
#include "Options.h"
#include "ConfigMembers.h"

#define INJECT(Sig)     \
    using Inject = Sig; \
    Sig

#define SELF(T) \
    using Self = T;

#define GROUP(G) \
    static inline std::string Group{G};

#define SELFG(T, G) SELF(T) GROUP(G)

#define IDENTITY(...) __VA_ARGS__

#define REMOVE_PARENS(...) IDENTITY __VA_ARGS__

#define MEMBERX(mname, args)                                                                                                        \
    struct AutoRegisteredMember_##mname                                                                                             \
    {                                                                                                                               \
        AutoRegisteredMember_##mname()                                                                                              \
        {                                                                                                                           \
            AutoRegisteredObjects::getInstance().addMember<Self, decltype(Self::mname)>(#mname, &Self::mname, REMOVE_PARENS(args)); \
        };                                                                                                                          \
    };                                                                                                                              \
    static inline AutoRegisteredMember_##mname autoRegisteredMember_##mname{};

#define MEMBERK(mname, key) MEMBERX(mname, (key))

#define MEMBERD(mname, dftV) MEMBERX(mname, (#mname, dftV))

#define MEMBERKD(mname, key, dftV) MEMBERX(mname, (key, dftV))

#define INIT(mname)                                                           \
    struct AutoRegisteredInit_##mname                                         \
    {                                                                         \
        AutoRegisteredInit_##mname()                                          \
        {                                                                     \
            AutoRegisteredObjects::getInstance().addInit<Self>(&Self::mname); \
        };                                                                    \
    };                                                                        \
    static inline AutoRegisteredInit_##mname autoRegisteredInit_##mname{};    \
    void mname

namespace fog
{

    template <typename T>
    struct always_false : std::false_type
    {
    };

    template <typename T>
    struct ConstructorTraits;

    template <typename R, typename... Args>
    struct ConstructorTraits<R(*)(Args...)>
    {
        using Type = R;
        using ArgsTuple = std::tuple<Args...>;
        static constexpr size_t arity = sizeof...(Args);
    };

    template <typename T, typename = void>
    struct hasInject : std::false_type
    {
    };

    template <typename T>
    struct hasInject<T, std::void_t<typename T::Inject>> : std::true_type
    {
    };

    template <typename T, typename Tuple>
    struct tuplePushFront;

    template <typename T, typename... Ts>
    struct tuplePushFront<T, std::tuple<Ts...>>
    {
        using type = std::tuple<T, Ts...>;
    };

    struct AutoRegisteredObjects
    {
        struct MemberInfo
        {
            bool asPtr;
            std::type_index pType;
            std::type_index vType;
            std::function<void(std::any, std::any)> setter;
            std::string key;
            std::function<bool(std::any &)> defaultVal;
            MemberInfo(bool asPtr, std::type_index pType, std::type_index vType, //
                       std::function<void(std::any, std::any)> setter,
                       std::string key,
                       std::function<bool(std::any &)> defaultVal) : //
                                                                     asPtr(asPtr), pType(pType), vType(vType), setter(setter), key(key), defaultVal(defaultVal)
            {
            }
        };
        struct MethodInfo
        {
            std::function<void(std::any)> method;
            MethodInfo(std::function<void(std::any)> method) : method(method)
            {
            }
        };
        struct ObjectInfo
        {
            std::unordered_map<std::string, MemberInfo> members;
            std::vector<MethodInfo> inits;
        };

        std::unordered_map<std::type_index, ObjectInfo> objects;

        template <typename T, typename F>
        void addMember(const std::string &fieldName, F T::*member, const std::string &key)
        {
            auto func = [](std::any &dftV)
            { return false; };
            doAddMember<T, F>(fieldName, member, key, func);
        }
        template <typename T, typename F>
        void addMember(const std::string &fieldName, F T::*member, const std::string &key, F dftVal)
        {
            std::function<bool(std::any &)> dftFunc;
            makeDefaultValFunction<F>(dftVal, dftFunc);
            doAddMember<T, F>(fieldName, member, key, dftFunc);
        }

        template <typename T, typename F>
        void doAddMember(const std::string &fieldName, F T::*member, const std::string &key, std::function<bool(std::any &)> dftFunc)
        {
            auto &objInfo = objects[std::type_index(typeid(T))];
            bool isPtr = std::is_pointer_v<F>;
            std::type_index vType = isPtr ? typeid(std::remove_pointer_t<F>) : typeid(F);
            std::type_index pType = isPtr ? typeid(F) : typeid(F *);
            objInfo.members.emplace(fieldName, MemberInfo(isPtr, pType, vType, [member](std::any obj, std::any value)
                                                          {
                                                              // we trust the value provided here is type match
                                                              T *tObj = std::any_cast<T *>(obj);
                                                              F val = std::any_cast<F>(value);
                                                              tObj->*member = val; //
                                                          },
                                                          key, dftFunc));
        }

        template <typename T>
        typename std::enable_if_t<!std::is_pointer_v<T>, void> makeDefaultValFunction(T dftV, std::function<bool(std::any &)> &func)
        {

            func = [dftV](std::any &retVal)
            {
                retVal = std::make_any<T>(dftV);
                return true;
            };
        }

        template <typename T>
        typename std::enable_if_t<std::is_pointer_v<T>, void> makeDefaultValFunction(T dftV, std::function<bool(std::any &)> &func)
        {

            func = [dftV](std::any &retVal)
            {
                if (dftV)
                {
                    retVal = std::make_any<T>(dftV);
                    return true;
                }
                return false;
            };
        }

        template <typename T, typename F>
        void addInit(F T::*init)
        {
            auto &objInfo = objects[std::type_index(typeid(T))];

            MethodInfo methodInfo([init](std::any obj)
                                  {
                                      T *tObj = std::any_cast<T *>(obj);
                                      (tObj->*init)(); //
                                  });
            if (objInfo.inits.size() > 0)
            {
                throw std::runtime_error("only support single init method, there are already one registered.");
            }
            objInfo.inits.emplace_back(methodInfo);
        }

        static AutoRegisteredObjects &getInstance()
        {
            static AutoRegisteredObjects instance;
            return instance;
        }
    }; // end of class

    template <typename T, typename C>
    struct ArgOfConstructor
    {
        using UsageFunc = std::function<std::any()>;
        UsageFunc ptrFunc; // T * .
        ArgOfConstructor(UsageFunc ptrFunc) : ptrFunc(ptrFunc)
        {
        }
    };
    /**
     * Component manage a set of functions and interfaces.there is no data and only contains meta info.
     */
    struct Injector;
    struct Component
    {
        using UsageFunc = std::function<std::any()>;
        using FieldsFunc = ConfigMembers<void>::Function;

        static Component make(std::type_index tid, UsageFunc createAsStatic, UsageFunc createAsDynamic,
                              FieldsFunc fields)
        {
            Object objS;
            objS.emplace(tid, createAsStatic);
            Object objD;
            objD.emplace(tid, createAsDynamic);
            return Component(tid, objS, objD, fields);
        }

        template <typename T, typename Imp, typename Tuple, std::size_t... Is>
        static Component make(UsageFunc createAsS, UsageFunc createAsD, std::index_sequence<Is...>, //
                              FieldsFunc fields)
        {
            Object objS; // map from type id => function to get/crate the required type of value.
            if (createAsS)
            {
                ((registerInterface<T, Imp, Tuple, Is>(objS, createAsS)), ...);
            }
            Object objD; // map from type id => function to get/crate the required type of value.
            if (createAsD)
            {
                ((registerInterface<T, Imp, Tuple, Is>(objD, createAsD)), ...);
            }

            return Component(typeid(T), objS, objD, fields);
        }
        
        template <typename T, typename Imp, typename AdtsTuple, typename IJ>
        static typename std::enable_if_t<!hasGroup<Imp>::value, Component> make(IJ &&ij)
        {
            return Impl::doMakeByImpl<T, Imp, AdtsTuple>(ij, ConfigMembers<void>::Function{});
        }

        template <typename T, typename Imp, typename AdtsTuple, typename IJ>
        static typename std::enable_if_t<hasGroup<Imp>::value, Component> make(IJ &&ij)
        {
            return Impl::doMakeByImpl<T, Imp, AdtsTuple>(ij, ConfigMembers<Imp>([&ij]()
                                                                                {
                                                                                       const Component *comp = (ij)(typeid(Options::Groups));
                                                                                       if (comp)
                                                                                       {
                                                                                           return comp->get<Options::Groups>(Component::AsStatic);
                                                                                       }
                                                                                       throw std::runtime_error("cannot resolve group, no component of Options::Groups registered."); }));
        }

    private:
        using Usage = unsigned int;

        using AnyFunc = std::function<std::any()>;
        using AddonFunc = std::function<std::any(Injector &)>;
        using InitFunc = std::function<bool(std::any)>;

        static constexpr Usage AsStatic = 1U << 0;
        static constexpr Usage AsDynamic = 1U << 1;

        using Interface = std::unordered_set<std::type_index>;
        using Object = std::unordered_map<std::type_index, UsageFunc>;
        using Value = UsageFunc;
        using Members = FieldsFunc;

        // struct Context
        // {

        //     UsageFunc pFunc;
        //     Context(UsageFunc pF) : pFunc(pF)
        //     {
        //     }
        // };

        std::type_index typeId; // register the main type of the component.
        Object objS;            // cast funcs
        Object objD;            // cast funcs

        Members members;
        friend struct Injector;
        Component(std::type_index typeId, Object objS, Object objD, Members mbs)
            : typeId(typeId), objS(objS), objD(objD), members(mbs) {};

        template <typename T>
        T *get(Usage usgR) const
        {
            return std::any_cast<T *>(get(usgR, typeid(T)));
        }

        std::any get(Usage usgR, std::type_index tid) const
        {
            return (getPtrFunc(usgR, tid))();
        }

        template <typename T>
        T &getRef(Usage usgR) const
        {
            return *get<T>(usgR);
        }

        UsageFunc getPtrFunc(Usage usgR, std::type_index tid) const
        {
            std::function<void()> f;

            UsageFunc func;
            if (usgR & AsStatic)
            {
                func = getPtrFuncStatic(tid);
            }
            else if (usgR & AsDynamic)
            {
                func = getPtrFuncDynamic(tid);
            }

            if (!func)
            {
                throw std::runtime_error("no ptr func found for the type and usage required.");
            }
            return func;
        }

        UsageFunc getPtrFuncStatic(std::type_index tid) const
        {
            if (auto it = objS.find(tid); it != objS.end())
            {
                return it->second;
            }
            return {};
        }

        UsageFunc getPtrFuncDynamic(std::type_index tid) const
        {
            if (auto it = objD.find(tid); it != objD.end())
            {
                return it->second;
            }
            return {};
        }

        // template <typename F>
        // void addType(std::type_index ifType, F &&func) // add a interface type for the component.
        // {
        //     auto it = obj.find(typeid(T));
        //     if (it != obj.end())
        //     {
        //         throw std::runtime_error("type already exists, cannot add same type for a component.");
        //     }
        //     obj.emplace(ifType, func);
        // }

        // template <typename F>
        // void forEachInterfaceType(F &&visit)
        // {
        //     for (auto it = obj.begin(); it != obj.end(); it++)
        //     {
        //         visit(it->first);
        //     }
        // // }

        // template <typename T>
        // bool hasType()
        // {
        //     std::type_index tid = typeid(T);
        //     return this->obj.find(tid) != this->obj.end();
        // }

        template <typename T, typename Imp, typename Tuple, std::size_t I>
        static void registerInterface(Object &obj, UsageFunc createAsPtr)
        {
            using InterfaceType = std::tuple_element_t<I, Tuple>;
            std::type_index tid = typeid(InterfaceType);
            auto func = [createAsPtr]() -> InterfaceType *
            {
                std::any impAny = createAsPtr();
                T *main = std::any_cast<T *>(impAny);
                return static_cast<InterfaceType *>(main); // cast to type.
            };

            obj.emplace(tid, func);
        }

        /**
         *
         * The typical impl of component, this impl relay on the macro defined above.
         *
         *  1. INJECT(MyClass(YouClass * uc)){} : declaring the constructor args to be injected.
         *  2. SELF(MyClass)        : This is as the helper macro for below ones.
         *  3. GROUP("config")      : Declaring the group name and used to resolve value from
         *      external by a function registered.
         *  4. MEMBERK(myMemberVar, "myConfigKey") : If the type(for insance float,int...) of
         *      the member is missing from registering, the key declared here is used as the argument
         *      to get value from a external function registered. The group above is another
         *      argument as well.
         *  5. MEMBERKD(myMemberVar2, "myConfigKey2", 1.0f) : declaring the default value to be set
         *      when initializing the target member. It's used when the registered function return
         *      false. If you use MEMBERK to declare the member variable, a exception will be
         *      throwed if the external function return false when initalising the target object.
         *
         *
         * And analysis the template type to find any constructor, member variable to be injected.
         * Register interface type, constructor function, member inject function and init function.
         * The injector does not manage pointer for static or dynamic instance returned by the get
         * method of the injector, there is no way for now to clean the static instance after
         * created.
         *
         *
         */

        struct Impl
        {

            template <typename T, typename Imp, typename AdtsTuple, typename IJ>
            static Component doMakeByImpl(IJ &&ij, ConfigMembers<void>::Function members)
            {

                using TAdtsTuple = tuplePushFront<T, AdtsTuple>::type;
                Component::UsageFunc funcAsStatic;  // empty func default.
                Component::UsageFunc funcAsDynamic; // empty func default.

                makeFunctionForUsage<T, Imp>(ij, funcAsStatic, funcAsDynamic); //

                return Component::make<T, Imp, TAdtsTuple>(funcAsStatic, funcAsDynamic,                                             //
                                                           std::make_index_sequence<std::tuple_size_v<std::decay_t<TAdtsTuple>>>{}, //
                                                           members);
            }

            template <typename T, typename Imp, typename IJ>
            static void makeFunctionForUsage(IJ &&ij, Component::UsageFunc &funcAsStatic, Component::UsageFunc &funcAsDynamic)
            {

                funcAsStatic = [&ij]() -> std::any
                {
                    return std::make_any<T *>(getPtrStatic<Imp>(ij));
                };

                funcAsDynamic = [&ij]() -> std::any
                {
                    return std::make_any<T *>(getPtrDynamic<Imp>(ij));
                };
            }

            template <typename T, typename IJ>
            static T *getPtrStatic(IJ &&ij)
            {
                static T *instance = createInstance<T>(ij);
                return (instance);
            }

            template <typename T, typename IJ>
            static T *getPtrDynamic(IJ &&ij)
            {

                T *ptr = createInstance<T>(ij);
                return ptr;
            }

            // abstract as Pointer.
            template <typename T, typename IJ>
            static typename std::enable_if_t<std::is_abstract_v<T>, T *> createInstance(IJ &&ij)
            {
                static_assert(always_false<T>::value, "abstract type & no injected impl class registered.");
            }
            // abstract as Value
            template <typename T, typename IJ>
            static typename std::enable_if_t<std::is_abstract_v<T>, T> createInstance()
            {
                static_assert(always_false<T>::value, "abstract type & no injected impl class registered.");
            }

            // concrete && !hasInject && as Pointer
            template <typename T, typename IJ>
            static typename std::enable_if_t<!std::is_abstract_v<T> && !hasInject<T>::value, T *> createInstance(IJ &&ij)
            {
                T *ret = new T{};
                init(ret, ij);
                return ret;
            }

            // concrete && hasInject && as Pointer
            template <typename T, typename IJ>
            static typename std::enable_if_t<!std::is_abstract_v<T> && hasInject<T>::value, T *> createInstance(IJ &&ij)
            {

                using ArgsTuple = typename ConstructorTraits<std::add_pointer_t<typename T::Inject>>::ArgsTuple;
                constexpr int N = ConstructorTraits<std::add_pointer_t<typename T::Inject>>::arity;
                return createInstanceByConstructor<T, ArgsTuple>(ij, std::make_index_sequence<N>{});

                // static_assert(N < 2, "todo more than 1 element in args list.");
            }

            // C<1>:As Pointer
            template <typename T, typename ArgsTuple, typename IJ, std::size_t... Is>
            static T *createInstanceByConstructor(IJ &&ij, std::index_sequence<Is...>)
            {
                // usgR is the runtime arg provided by the top most getPtr(usgR), this argument control only the outer most object creation.
                // do dynamic usge, do not propagate to deep layer, may be useful for other usage after unset the AsDynamic mask.
                //
                T *ret = new T(getAsConstructorArg<T, Is, std::tuple_element_t<Is, ArgsTuple>>(ij)...);
                init<T>(ret, ij);
                return ret;
            }

            template <typename T, typename IJ>
            static void init(T *ptr, IJ &&ij)
            {

                AutoRegisteredObjects &autoRegObjs = AutoRegisteredObjects::getInstance();
                if (auto itObj = autoRegObjs.objects.find(std::type_index(typeid(T))); itObj != autoRegObjs.objects.end()) // make init func to setting fields.
                {
                    std::any objPA = std::make_any<T *>(ptr);
                    const AutoRegisteredObjects::ObjectInfo &objInfo = itObj->second;
                    setRegistedMembers<T>(objPA, objInfo, ij);
                    callRegistedInit<T>(objPA, objInfo);
                }
            }

            template <typename T>
            static void callRegistedInit(std::any &objPA, const AutoRegisteredObjects::ObjectInfo &objInfo)
            {
                for (const auto &init : objInfo.inits)
                {
                    init.method(objPA);
                }
            }
            template <typename T, typename IJ>
            static void setRegistedMembers(std::any &objPA, const AutoRegisteredObjects::ObjectInfo &objInfo, IJ &&ij)
            {
                //

                for (const auto &fieldPair : objInfo.members)
                {
                    const std::string &mebName = fieldPair.first;
                    const AutoRegisteredObjects::MemberInfo &mebInfo = fieldPair.second;
                    //

                    std::any val;
                    const Component *cPtr = ij(mebInfo.vType);
                    if (cPtr)
                    {
                        if (mebInfo.asPtr)
                        {
                            val = cPtr->get(Component::AsStatic, mebInfo.vType);
                        }
                        else
                        {
                            val = cPtr->get(Component::AsStatic, mebInfo.vType); // TODO
                        }
                        //
                    }
                    else
                    { // no component bind to the type, try to get registed fields.
                        const Component *pCPtr = ij(typeid(T));
                        if (pCPtr->members)
                        {
                            if (!(pCPtr->members)(mebInfo.vType, mebName, mebInfo.key, val, false))
                            {
                                if (!(mebInfo.defaultVal)(val))
                                {
                                    throw std::runtime_error("connot resolve the value for member:" + mebName + ",key:" + mebInfo.key + "(no default value)");
                                }
                            }
                        }
                        else
                        {
                            throw std::runtime_error("connot resolve the value for member:" + mebName + ",key:" + mebInfo.key + "(no function registered)");
                        }
                    }

                    (mebInfo.setter)(objPA, val);

                } // fields
            }

            // Arg as Pointer
            template <typename C, std::size_t I, typename Arg, typename IJ>
            static typename std::enable_if_t<std::is_pointer_v<Arg>, Arg> getAsConstructorArg(IJ &&ij) // return Arg or Arg *
            {
                using T = std::remove_pointer_t<Arg>;
                T *ret = doGetAsConstructorArg<C, I, T>(ij);
                return ret;
            }

            template <typename C, std::size_t I, typename Arg, typename IJ>
            static typename std::enable_if_t<std::is_reference_v<Arg>, Arg> getAsConstructorArg(IJ &&ij) // return Arg or Arg *
            {
                using T = std::remove_reference_t<Arg>;
                T *ret = doGetAsConstructorArg<C, I, T>(ij);
                return *ret;
            }

            template <typename C, std::size_t I, typename Arg, typename IJ>
            static typename std::enable_if_t<!std::is_pointer_v<Arg> && !std::is_reference_v<Arg>, Arg &> getAsConstructorArg(IJ &&ij) // return Arg or Arg *
            {
                Arg *ret = doGetAsConstructorArg<C, I, Arg>(ij);
                return *ret;
            }

            template <typename C, std::size_t I, typename T, typename IJ>
            static T *doGetAsConstructorArg(IJ &&ij)
            {
                const Component *cPtr = (ij)(typeid(T));
                if (cPtr)
                {
                    return cPtr->get<T>(Component::AsStatic);
                }

                cPtr = ij(typeid(ArgOfConstructor<T, C>));
                if (cPtr)
                {
                    ArgOfConstructor<T, C> *cArg = cPtr->get<ArgOfConstructor<T, C>>(Component::AsStatic);
                    std::any aPtr = (cArg->ptrFunc)();

                    return std::any_cast<T *>(aPtr);
                }
                throw std::runtime_error("cannot resolve component for a arg of constructor.");
            }
            friend struct Injector;
        }; //
    };
};
