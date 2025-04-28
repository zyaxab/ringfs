t = executable('test_ringfs',
  sources: files(
    'flashsim.c',
    'tests.c',
  ),
  include_directories: include_directories('.'),
  dependencies: [ringfs, dependency('criterion')]
)

test(t.name(), t)