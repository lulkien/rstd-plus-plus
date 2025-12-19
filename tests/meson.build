if get_option('tests')
  gtest_dep  = dependency('gtest', required : false)
  gtest_main = dependency('gtest_main', required : false)
  gmock_dep  = dependency('gmock', required : false)

  if not (gtest_dep.found() and gtest_main.found())
    cmake = import('cmake')
    gtest_proj = cmake.subproject('googletest', required : true)

    gtest_dep = declare_dependency(
      dependencies : [
        gtest_proj.dependency('gtest'),
        gtest_proj.dependency('gtest_main')
      ]
    )
    gmock_dep = declare_dependency(
      dependencies : [ gtest_proj.dependency('gmock') ]
    )
  endif

  inc_dir = include_directories('../include')
  tests_src = [
    'rstd++/result_test.cpp'
  ]

  test_exe = executable('result_test',
    tests_src,
    include_directories : [inc_dir],
    dependencies : [gtest_dep, gtest_main, gmock_dep],
    install : false)

    test('gtest tests', test_exe)
endif

