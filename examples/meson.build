example_sources = [
  'result_example.cpp',
]

inc_dir = include_directories('../include')

foreach src : example_sources
  name = src.split('.')[0]
  executable(name,
    src,
    include_directories : inc_dir,
    install : false)
endforeach
