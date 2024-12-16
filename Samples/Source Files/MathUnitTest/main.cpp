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
    //Quaternion
    {
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

            auto diff = std::abs(y.qx.i -0.0f) + std::abs(y.qy.j -1.0f) + std::abs(y.qz.k -0.0f);

            printtest("rotating x arround z of PI/2 give y", diff < 0.0000001f);
        }

    }

    // Matrices 4x'

    {
        mat4f m1({
            0 ,1 ,2 ,3,
            4 ,5 ,6 ,7,
            8 ,9 ,10,11,
            12,13,14,15,
        });

        auto v = m1[0];

        bool passed = v[0] == 0 && v[1] == 1 && v[2] == 2 && v[3] == 3;

        printtest("m[0] gives the first line", passed);

    }

    {
        mat4f m1({
            0 ,1 ,2 ,3,
            4 ,5 ,6 ,7,
            8 ,9 ,10,11,
            12,13,14,15,
        });

        mat4f m2({
            16,17,18,19,
            20,21,22,23,
            24,25,26,27,
            28,29,30,31,
        });
        

        auto m3 = m1 +m2;
        bool passed = true;
        for(int i = 0; i< 4;i++){
            for(int j = 0; j< 4;j++){
                passed = passed && (m1[i][j] + m2[i][j] == m3[i][j]);
            }
        }
        printtest("summing two matrice gives the right results", passed);
    }

    {
         mat4f m1({
            0 ,1 ,2 ,3,
            4 ,5 ,6 ,7,
            8 ,9 ,10,11,
            12,13,14,15,
        });

        mat4f m2({
            16,17,18,19,
            20,21,22,23,
            24,25,26,27,
            28,29,30,31,
        });
        

        auto m3 = m1 -m2;
        bool passed = true;
        for(int i = 0; i< 4;i++){
            for(int j = 0; j< 4;j++){
                passed = passed && (m1[i][j] - m2[i][j] == m3[i][j]);
            }
        }
        printtest("substracting two matrice gives the right results", passed);
    }

    {

        mat4f I({
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1
        });

       

        auto m3 = I * I;
        bool passed = true;
        for(int i = 0; i< 4;i++){
            for(int j = 0; j< 4;j++){
                passed = passed && ( m3[j][i] == I[j][i]);
            }
        }
        printtest("Multiplying I by I", passed);
    }

    {
        mat4f m1({
            0 ,1 ,2 ,3,
            4 ,5 ,6 ,7,
            8 ,9 ,10,11,
            12,13,14,15,
        });

        mat4f m2({
            16,17,18,19,
            20,21,22,23,
            24,25,26,27,
            28,29,30,31,
        });

        auto m3 = m1 * m2;

        m2 = transpose (m2);
   
        bool passed = true;
        for(int i = 0; i< 4;i++){
            for(int j = 0; j< 4;j++){
                passed = passed && ( m3[i][j] == dot(m1[i], m2[j]));
            }
        }

        printtest("Multiplying two matrices", passed);
    }

    
    {
        auto s = PrepareScalingMatrix(2,3,4);
        vec4f v ({5,6,7,1});

        auto res = s * v;

        bool passed = res[0] == 10 && res[1] == 18 && res[2] == 28 && res[3] == 1;
        printtest("scaling a vector by a scaling matrix", passed);
    }

    {
        auto m1 = PrepareTranslationMatrix(2,3,4);
        vec4f v ({5,6,7,1});

        v = m1 * v;

        bool passed = v[0] == 7 && v[1] == 9 && v[2] == 11 && v[3] == 1;
        printtest("applying a traslation to a vector using a translation matrix", passed);
    }



    {
        auto m1 = PrepareScalingMatrix(2,2,2);
        auto m2 = PrepareTranslationMatrix(2,3,4);

        auto m3 = m1 * m2;


        auto t = transpose(m3)[3];

        bool passed = t[0] == 4 && t[1] == 6 && t[2] == 8 && t[3] == 1;


        printtest("mutliply a scaling matrice by a translate affect the transaltion", passed);
    }

    {
        mat4f m1 = PrepareScalingMatrix(1,2,3);

        auto im1 = inverse(m1);
        auto I = m1 * im1;
        auto I2 =im1 * m1;

        bool passed = I[0][0] == 1 &&  I[0][0] == 1 &&  I[0][0] == 1 &&  I[0][0] == 1;
        printtest("Inverse diag matrix", passed);
    }

    {
        mat4f m1 = PrepareRotationMatrix(75.0f, vec3f({1.0,0.0,0.0}));

        auto im1 = inverse(m1);
        auto I = m1 * im1;
        auto I2 =im1 * m1;

        bool passed = I[0][0] == 1 &&  I[0][0] == 1 &&  I[0][0] == 1 &&  I[0][0] == 1;
        printtest("Inverse rotation matrix", passed);
    }

    {
        mat4f m1({
            13 ,56 ,87 ,45,
            36 ,25 ,125 ,78,
            91 ,23 ,45,86,
            169,78,35,7,
        });

        auto im1 = inverse(m1);
        auto I = m1 * im1;
        auto I2 =im1 * m1;

        bool passed = I[0][0] == 1 &&  I[0][0] == 1 &&  I[0][0] == 1 &&  I[0][0] == 1;
        printtest("Inverse rotation matrix", passed);
    }
}