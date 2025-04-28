project('ringfs', 'c',
  version : '0.1',
  default_options : ['warning_level=3'])

ringfs = declare_dependency(
  sources: files(
    'ringfs.c',
  ),
  include_directories: include_directories('.')
)

subdir('test_new')

