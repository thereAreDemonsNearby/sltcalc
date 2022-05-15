#include <tuple>
#include <iostream>

int main()
{
    int i = 1;
    double d = 2.2;
    std::tuple<int&, double&> a(i, d);
    std::get<0>(a) = 32;
    std::cout << i << "\n";
}
