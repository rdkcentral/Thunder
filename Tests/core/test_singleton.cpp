#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

class SingletonTypeOne {
    public:
        SingletonTypeOne()
        {
        }
        virtual ~SingletonTypeOne()
        {
        }
};

class SingletonTypeTwo {
    public:
        SingletonTypeTwo(string)
        {
        }
        virtual ~SingletonTypeTwo()
        {
        }
};
class SingletonTypeThree {
    public:
        SingletonTypeThree (string, string)
        {
        }
        virtual ~SingletonTypeThree()
        {
        }
};


TEST(test_singleton, simple_singleton)
{
    static SingletonTypeOne& object1 = SingletonType<SingletonTypeOne>::Instance();
    static SingletonTypeOne& object_sample = SingletonType<SingletonTypeOne>::Instance();
    ASSERT_EQ(&object1,&object_sample);
    static SingletonTypeTwo& object2 = SingletonType<SingletonTypeTwo>::Instance("SingletonTypeTwo");
    static SingletonTypeThree& object3 = SingletonType<SingletonTypeThree>::Instance("SingletonTypeThree","SingletonTypeThree");
    SingletonType<SingletonTypeTwo>* x = (SingletonType<SingletonTypeTwo>*)&object2;
    ASSERT_STREQ(x->ImplementationName().c_str(),"SingletonTypeTwo");
    SingletonType<SingletonTypeThree>* y = (SingletonType<SingletonTypeThree>*)&object3;
    ASSERT_STREQ(y->ImplementationName().c_str(),"SingletonTypeThree");
    Singleton::Dispose();
}

