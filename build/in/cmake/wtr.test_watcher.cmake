wtr_add_test_target(
  "wtr.${WTR_TEST_WATCHER_PROJECT_NAME}"
  "${COMPILE_OPTIONS_HIGH_ERR}"
  "${LINK_OPTIONS}")

wtr_add_test_target(
  "wtr.${WTR_TEST_WATCHER_PROJECT_NAME}.asan"
  "${COMPILE_OPTIONS_HIGH_ERR_ASAN}"
  "${LINK_OPTIONS_ASAN}")

wtr_add_test_target(
  "wtr.${WTR_TEST_WATCHER_PROJECT_NAME}.msan"
  "${COMPILE_OPTIONS_HIGH_ERR_MSAN}"
  "${LINK_OPTIONS_MSAN}")

wtr_add_test_target(
  "wtr.${WTR_TEST_WATCHER_PROJECT_NAME}.tsan"
  "${COMPILE_OPTIONS_HIGH_ERR_TSAN}"
  "${LINK_OPTIONS_TSAN}")

wtr_add_test_target(
  "wtr.${WTR_TEST_WATCHER_PROJECT_NAME}.ubsan"
  "${COMPILE_OPTIONS_HIGH_ERR_UBSAN}"
  "${LINK_OPTIONS_UBSAN}")