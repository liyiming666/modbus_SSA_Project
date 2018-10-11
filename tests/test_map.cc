
#include "map.h"
#include <string>
using namespace std;

int main(void)
{
        const char* p1 = "111";
        const char* p2 = "222";
        const char* p3 = "333";
        const char* p4 = "444";
        Map<string, const char*> m;
        m["1"] = p1;
        m["2"] = p2;
        m["3"] = p3;
        m["4"] = p4;

        cerr << m["1"] << endl;
        const char* tmp = m["5"];
        cerr << m["1"] << endl;
        cerr << m.size() << endl;
        cerr << m["5"] << endl;


        return 0;
}
