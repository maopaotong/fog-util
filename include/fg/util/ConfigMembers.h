#pragma once
#include "Options.h"
namespace fog
{

    template <typename T, typename = void>
    struct hasGroup : std::false_type
    {
    };

    template <typename T>
    struct hasGroup<T, std::void_t<decltype(T::Group)>> : std::true_type
    {
    };
    template <typename T>
    struct ConfigMembers
    {
        using Function = std::function<bool(const std::type_index &, const std::string &, const std::string &, std::any &, bool strict)>;

        ConfigMembers(std::function<Options::Groups *()> groups) : groups(groups)
        {
        }

        bool operator()(const std::type_index &mType, const std::string &mName, const std::string &key, std::any &fval, bool strict)
        {
            Options::Groups *gps = groups();
            if (!gps)
            {
                if (strict)
                {
                    throw std::runtime_error("no options groups found from injector.");
                }
                return false;
            }
            std::string rKey = key.empty() ? mName : key;
            std::string gname = resolveGroup<T>();
            if (auto it = gps->groups.find(gname); it != gps->groups.end())
            {
                Options &ops = it->second;

                Options::Option *opt = ops.getOption(rKey);
                if (opt)
                {
                    if(opt->getType() != mType){
                        throw std::runtime_error("cannot resolve option[" + gname + "]" + rKey + "(type mismatch)");
                    }
                    fval = opt->getValue();
                    return true;
                }
                if (strict)
                {

                    throw std::runtime_error("cannot resolve option [" + gname + "]" + rKey + "(no option found)");
                }
            }
            if (strict)
            {
                throw std::runtime_error("cannot resolve option [" + gname + "]" + rKey + "(no group found)");
            }
            return false;
        }

    private:
        std::function<Options::Groups *()> groups;

        template <typename X>
        static std::enable_if_t<hasGroup<X>::value, std::string> resolveGroup()
        {
            return X::Group;
        }

        template <typename X>
        static std::enable_if_t<!hasGroup<X>::value, std::string> resolveGroup()
        {
            throw std::runtime_error("cannot resolve group without X::Group");
        }
    };
};