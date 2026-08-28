#include "../config/config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static const char* TEMP_PATH = "settings_test.tmp.yaml";
static const char* MISSING_PATH = "settings_test_missing.yaml";

static void writeFile(const char* path, const char* text) {
  FILE* f = fopen(path, "w");
  assert(f != NULL);
  fputs(text, f);
  fclose(f);
}

static void test_load_missing_file_returns_false_and_defaults(void) {
  Settings s;
  s.theme = 7;
  s.showSparklines = false;
  s.animateValues = false;
  s.showIndicators = false;

  int ok = LoadSettings(MISSING_PATH, &s);
  assert(ok == 0);
  assert(s.theme == 0);
  assert(s.showSparklines == 1);
  assert(s.animateValues == 1);
  assert(s.showIndicators == 1);
}

static void test_roundtrip_preserves_all_fields(void) {
  Settings out;
  for (int theme = 0; theme <= 4; theme++) {
    for (int bits = 0; bits < 8; bits++) {
      Settings s = {
        .theme = theme,
        .showSparklines = (bits & 1) != 0,
        .animateValues = (bits & 2) != 0,
        .showIndicators = (bits & 4) != 0,
      };
      assert(SaveSettings(TEMP_PATH, &s));
      assert(LoadSettings(TEMP_PATH, &out));
      assert(out.theme == s.theme);
      assert(out.showSparklines == s.showSparklines);
      assert(out.animateValues == s.animateValues);
      assert(out.showIndicators == s.showIndicators);
    }
  }
}

static void test_roundtrip_all_false(void) {
  Settings s = { .theme = 2, .showSparklines = false,
                 .animateValues = false, .showIndicators = false };
  assert(SaveSettings(TEMP_PATH, &s));

  Settings out = {0};
  assert(LoadSettings(TEMP_PATH, &out));
  assert(out.theme == 2);
  assert(out.showSparklines == 0);
  assert(out.animateValues == 0);
  assert(out.showIndicators == 0);
}

static void test_backward_compat_theme_only(void) {
  writeFile(TEMP_PATH, "theme: 4\n");

  Settings out = {0};
  assert(LoadSettings(TEMP_PATH, &out));
  assert(out.theme == 4);
  assert(out.showSparklines == 1);
  assert(out.animateValues == 1);
  assert(out.showIndicators == 1);
}

static void test_tolerates_unknown_keys(void) {
  writeFile(TEMP_PATH,
            "theme: 1\n"
            "foo: bar\n"
            "animated_values: false\n"
            "nested:\n"
            "  a: 1\n"
            "  b: 2\n");

  Settings out = {0};
  assert(LoadSettings(TEMP_PATH, &out));
  assert(out.theme == 1);
  assert(out.showSparklines == 1);   // unknown keys ignored, defaults kept
  assert(out.animateValues == 0);
  assert(out.showIndicators == 1);
}

static void test_read_bool_accepts_1_and_0(void) {
  writeFile(TEMP_PATH,
            "theme: 0\n"
            "animated_values: 0\n"
            "change_indicators: 1\n");

  Settings out = {0};
  assert(LoadSettings(TEMP_PATH, &out));
  assert(out.animateValues == 0);
  assert(out.showIndicators == 1);
}

static void test_invalid_bool_value_fails(void) {
  writeFile(TEMP_PATH,
            "theme: 0\n"
            "trend_lines: yes\n");

  Settings out = {0};
  assert(LoadSettings(TEMP_PATH, &out) == 0);
  // on failure the struct is left at defaults
  assert(out.theme == 0);
  assert(out.showSparklines == 1);
}

static void test_corrupt_yaml_fails(void) {
  writeFile(TEMP_PATH, "just a scalar, not a mapping\n");

  Settings out = {0};
  assert(LoadSettings(TEMP_PATH, &out) == 0);
}

static void test_save_unwritable_path_returns_false(void) {
  Settings s = { .theme = 0, .showSparklines = true,
                 .animateValues = true, .showIndicators = true };
  assert(SaveSettings("/no_such_dir_x7y9/settings.yaml", &s) == 0);
}

int main(void) {
  test_load_missing_file_returns_false_and_defaults();
  test_roundtrip_preserves_all_fields();
  test_roundtrip_all_false();
  test_backward_compat_theme_only();
  test_tolerates_unknown_keys();
  test_read_bool_accepts_1_and_0();
  test_invalid_bool_value_fails();
  test_corrupt_yaml_fails();
  test_save_unwritable_path_returns_false();

  remove(TEMP_PATH);
  printf("All settings tests passed\n");
  return 0;
}
