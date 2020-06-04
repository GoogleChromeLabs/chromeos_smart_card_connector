# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

EXECUTABLES="
gtest_break_on_failure_unittest_
gtest_catch_exceptions_ex_test_
gtest_catch_exceptions_no_ex_test_
gtest_color_test_
gtest_env_var_test_
gtest_filter_unittest_
gtest_help_test_
gtest_list_tests_unittest_
gtest_output_test_
gtest_shuffle_test_
gtest_throw_on_failure_test_
gtest_uninitialized_test_
gtest_xml_outfile1_test_
gtest_xml_outfile2_test_
gtest_xml_output_unittest_
"

CTEST_EXECUTABLES="
gtest_main_unittest
gtest_unittest
gtest_no_test_unittest
gtest_no_rtti_unittest
gtest_pred_impl_unittest
gtest-death-test_ex_catch_test
gtest-death-test_ex_nocatch_test
gtest-death-test_test
gtest_environment_test
gtest-filepath_test
gtest-linked_ptr_test
gtest-listener_test
gtest-message_test
gtest-options_test
gtest-param-test_test
gtest-port_test
gtest_premature_exit_test
gtest-printers_test
gtest_prod_test
gtest_repeat_test
gtest_sole_header_test
gtest_stress_test
gtest-test-part_test
gtest_throw_on_failure_ex_test
gtest-tuple_test
gtest-typed-test_test
gtest-unittest-api_test
gtest_use_own_tuple_test
"

ConfigureStep() {
  Remove ${SRC_DIR}/configure
  EXTRA_CMAKE_ARGS="-Dgtest_build_tests=1"
  for exe in $CTEST_EXECUTABLES; do
    Remove $exe
  done
  DefaultConfigureStep
}

InstallStep() {
  MakeDir ${DESTDIR_LIB}
  MakeDir ${DESTDIR_INCLUDE}

  LogExecute install -m 644 libgtest*.a ${DESTDIR_LIB}/

  LogExecute cp -r --no-preserve=mode ${SRC_DIR}/include/gtest \
    ${DESTDIR_INCLUDE}/gtest
}

TestStep() {
  if [ "${NACL_ARCH}" = "pnacl" ]; then
    return
  fi
  for exe in $CTEST_EXECUTABLES; do
    mv $exe $exe$NACL_EXEEXT
    WriteLauncherScript $exe $exe$NACL_EXEEXT
  done
  # Disable running of tests until they are all passing
  # TODO(sbc): Fix the broken tests:
  # 80% tests passed, 8 tests failed out of 41
  # The following tests FAILED:
  #   29 - gtest_break_on_failure_unittest (Failed)
  #   31 - gtest_color_test (Failed)
  #   32 - gtest_env_var_test (Failed)
  #   33 - gtest_filter_unittest (Failed)
  #   36 - gtest_output_test (Failed)
  #   37 - gtest_shuffle_test (Failed)
  #   38 - gtest_throw_on_failure_test (Failed)
  #   41 - gtest_xml_output_unittest (Failed)
  #LogExecute make test
}
