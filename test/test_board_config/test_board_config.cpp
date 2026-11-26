#include <unity.h>
#include "../../src/board_config.h"

void setUp(void) {}

void tearDown(void) {}

static void test_default_board_is_ttgo(void) {
  TEST_ASSERT_EQUAL(1, BOARD_TTGO_T_DISPLAY);
  TEST_ASSERT_EQUAL_STRING("TTGO T-Display", BOARD_NAME);
}

static void test_ttgo_pins(void) {
  TEST_ASSERT_EQUAL(0, BTN_TOP);
  TEST_ASSERT_EQUAL(35, BTN_BOTTOM);
  TEST_ASSERT_EQUAL(34, BAT_PIN);
  TEST_ASSERT_EQUAL(1, NISKALA_HAS_TFT);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_default_board_is_ttgo);
  RUN_TEST(test_ttgo_pins);
  return UNITY_END();
}