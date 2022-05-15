#include <iostream>
#include "calc_aslib.h"

int main(int argc, char** argv)
{
    if (argc == 2) {
        CalcResult result = calc(argv[1]);
        if (result.type == CalcResult::Error) {
            std::cerr << "Syntax error." << std::endl;
        } else if (result.type == CalcResult::CalcResultType::Int) {
            std::cout << result.intResult() << std::endl;
        } else if (result.type == CalcResult::CalcResultType::Double) {
            std::cout << result.doubleResult() << std::endl;
        } else {
            std::cerr << "Unknown result type." << std::endl;
        }
    } else {
        std::cerr << "Syntax error." << std::endl;
    }
}
