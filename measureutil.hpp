/**
 * Author: Chunhao Xie
 * Date: 2022-03-04 @ Ruitian Capital
 *
 * This is a time measurement util. It will be faster and more accurate than callgrind because there are rarely pit fulls.
 */

#ifndef UNTITLED_MEASUREUTIL_HPP
#define UNTITLED_MEASUREUTIL_HPP

#include <chrono>
#include <cstdio>
#include <utility>

#define __MeasurePaste(a, b) a ## b

#define __MeasureHelper(line, num) \
auto __MeasurePaste(before,num) = chrono::high_resolution_clock::now().time_since_epoch().count(); \
line;                                       \
auto __MeasurePaste(after,num) = chrono::high_resolution_clock::now().time_since_epoch().count();  \
printf("\033[0;36mMeasure-> [line %4d] %20.06lf ms\t\t\t %s\n\033[0m", num, ((__MeasurePaste(after,num) - __MeasurePaste(before,num)) / 1000.0 / 1000.0), #line);

#define Measure(line) __MeasureHelper(line, __LINE__)

#define __BlockMeasureHelper(file, line) BlockMeasureGuard __MeasurePaste(measureGuard,line)(file, line);
#define MeasureBlock __BlockMeasureHelper(__FILE__, __LINE__)

class BlockMeasureGuard final{
public:
    BlockMeasureGuard(std::string file, int line): file(std::move(file)), line(line){
        this->before = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    }
    ~BlockMeasureGuard(){
        printf("\033[0;36mBlock Measure Guard-> [%s : %4d] %20.06lf ms\n\033[0m", this->file.c_str(), this->line,
               (std::chrono::high_resolution_clock::now().time_since_epoch().count() - this->before) / 1000.0 / 1000.0);
    }
private:
    decltype(std::chrono::high_resolution_clock::now().time_since_epoch().count()) before;
    std::string file;
    int line;
};


#endif //UNTITLED_MEASUREUTIL_HPP
