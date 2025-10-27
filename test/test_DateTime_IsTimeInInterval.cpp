#include <unity.h>
#include "DateTime.h"

using namespace DateTimeNS;

void test_interval_no_midnight_inside()
{
    tTime start{8, 0, 0};
    tTime end{17, 0, 0};
    tTime current{12, 0, 0};
    TEST_ASSERT_TRUE(IsTimeInInterval(current, start, end));
}

void test_interval_no_midnight_outside()
{
    tTime start{8, 0, 0};
    tTime end{17, 0, 0};
    tTime current{7, 59, 59};
    TEST_ASSERT_FALSE(IsTimeInInterval(current, start, end));
}

void test_interval_cross_midnight_inside_before_midnight()
{
    tTime start{22, 0, 0};
    tTime end{6, 0, 0};
    tTime current{23, 0, 0};
    TEST_ASSERT_TRUE(IsTimeInInterval(current, start, end));
}

void test_interval_cross_midnight_inside_after_midnight()
{
    tTime start{22, 0, 0};
    tTime end{6, 0, 0};
    tTime current{2, 0, 0};
    TEST_ASSERT_TRUE(IsTimeInInterval(current, start, end));
}

void test_interval_cross_midnight_outside()
{
    tTime start{22, 0, 0};
    tTime end{6, 0, 0};
    tTime current{12, 0, 0};
    TEST_ASSERT_FALSE(IsTimeInInterval(current, start, end));
}

void test_interval_start_equals_end()
{
    tTime start{8, 0, 0};
    tTime end{8, 0, 0};
    tTime current{8, 0, 0};
    TEST_ASSERT_FALSE(IsTimeInInterval(current, start, end));
}

void setup()
{
    UNITY_BEGIN();
    RUN_TEST(test_interval_no_midnight_inside);
    RUN_TEST(test_interval_no_midnight_outside);
    RUN_TEST(test_interval_cross_midnight_inside_before_midnight);
    RUN_TEST(test_interval_cross_midnight_inside_after_midnight);
    RUN_TEST(test_interval_cross_midnight_outside);
    RUN_TEST(test_interval_start_equals_end);
    UNITY_END();
}

void loop()
{
    // not used
}