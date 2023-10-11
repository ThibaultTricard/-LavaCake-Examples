#include <LavaCake/Framework/Framework.h>
#include <LavaCake/Math/basics.h>
#include <LavaCake/Math/quaternion.h>
using namespace LavaCake::Framework;
using namespace LavaCake;

#define PI 3.14159265359f

void printtest(const char* test_name, bool passed){
    if(passed){
        std::cout << "\033[32m";
    }else{
        std::cout << "\033[31m";
    }

    std::cout << test_name;

    if(passed){
        std::cout << ": passed";
    }else{
        std::cout << ": failed";
    }

    std::cout << "\033[0m" << std::endl;
}


int main() {

    i_t<float> i{1.0};
    j_t<float> j{1.0};
    k_t<float> k{1.0};
    
    {
        auto i2 = i * i;
        printtest("i * i = -1", i2==-1.0f);
        printtest("i * i is a scalar",typeid(i2) ==typeid(float));
    }

    {    
        auto j2 = j * j;
        printtest("j * j = -1", j2==-1.0f);
        printtest("j * j is a scalar",typeid(j2) ==typeid(float));
    }

    {
        auto k2 = k * k;
        printtest("k * k = -1", k2==-1.0f);
        printtest("k * k is a scalar",typeid(k2) ==typeid(float));
    }

    {
        auto i_ = i * 1.0f;
        auto _i = 1.0f * i;
        printtest("norme of i * 1 is 1 ", i_.i == 1.0f);
        printtest("type of i * 1 is i",typeid(i_) == typeid(i));
        printtest("i * 1 = 1 * i ", i_ == _i);
    }

    {
        auto j_ = j * 1.0f;
        auto _j = 1.0f * j;
        printtest("norme of j * 1 is 1 ", j_.j == 1.0f);
        printtest("type of j * 1 is i",typeid(j_) == typeid(j));
        printtest("j * 1 = 1 * j ", j_ == _j);
    }

    {
        auto k_ = k * 1.0f;
        auto _k = 1.0f * k;
        printtest("norme of k * 1 is 1 ", k_.k == 1.0f);
        printtest("type of k * 1 is i",typeid(k_) == typeid(k));
        printtest("k * 1 = 1 * k ", k_ == _k);
    }
    

    {
        auto k_ = i * j;
        auto _k = j * i;
        printtest("i * j ==  k ", k_ == k);
        printtest("j * i == -k ", _k == -1.0f * k);
    }

    {
        auto i_ = k * j;
        auto _i = j * k;
        printtest("k * j == -i ", i_ == -1.0f * i);
        printtest("j * k ==  i ", _i == i);
    }

    {
        auto j_ = i * k;
        auto _j = k * i;
        printtest("i * k == -j ", j_ == -1.0f * j);
        printtest("k * i ==  j ", _j ==  j);
    }

    

    {
        vec3f x({1.0f,0.0f,0.0f});
        auto q1 = quaternion(PI/2.0f,vec3f({0.0f,0.0f,1.0f})); 
        auto q2 = inverse(q1);

        auto y = (q1 * x) * q2;

        //auto diff = std::abs(y[0] -0.0f) + std::abs(y[1] -1.0f) + std::abs(y[2] -0.0f);

        //printtest("rotating x of PI/2 give y", diff < 0.0000001f);
    }

}