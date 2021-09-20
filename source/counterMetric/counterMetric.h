#ifndef COUNTERMETRIC_H
#define COUNTERMETRIC_H

#include <stdio.h>
#include <cassert>

#ifndef SINGLE_HEADER
#include "../metricBase/metricBase.h"
#endif

namespace kleins {

namespace metrics {

class counterMetric : public metricBase
{
private:
    uint64_t counterValue = 0;
public:
    counterMetric(/* args */);
    ~counterMetric();

    uint64_t get();
    void set(uint64_t value);
    void inc(uint64_t value = 1);
    void reset();

    virtual const char* getType();

    virtual std::unique_ptr<char*> construct() = 0;
};



}

} // namespace kleins

#endif