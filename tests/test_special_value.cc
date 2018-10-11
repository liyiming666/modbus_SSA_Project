
#include "special_value.h"
#include <iostream>
#include <string>
#include <cassert>

using namespace std;

int main(void)
{
        Special sp;
        sp.init();
        string cal_key = "test_cal", cal_value_1 = "123", cal_value_2 = "456";
        assert(sp.setValue(cal_key, cal_value_1));
        assert(sp.setValue(cal_key, cal_value_2));
        assert(cal_value_2 == sp.getValue(cal_key));

        string thre_key = "test_thre", thre_value_1 = "111", thre_value_2 = "222";
        assert(sp.setValue(thre_key, thre_value_1));
        assert(sp.setValue(thre_key, thre_value_2));
        assert(thre_value_2 == sp.getValue(thre_key));

        string dz_key = "test_dz", dz_value_1 = "111", dz_value_2 = "222";
        assert(sp.setValue(dz_key, dz_value_1));
        assert(sp.setValue(dz_key, dz_value_2));
        assert(dz_value_2 == sp.getValue(dz_key));

        string temp_key = "test_temp";
        unsigned int temp_1[] = {10, 10, 10, 10};
        unsigned int temp_2[] = {20, 20, 20, 20};

        assert(sp.setTempThreValue(temp_1));
        assert(sp.setTempThreValue(temp_2));
        // unsigned int* p = sp.getTempThreValue(temp_key);
        // assert(p);
        // for (int i = 0; i < 4; i++) {
        //         assert(p[i] == temp_2[i]);
        // }

        string s = sp.getTempThreValue();
        cerr << s << endl;

        cerr << "ok!" << endl;

        return 0;
}
