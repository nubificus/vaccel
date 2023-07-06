project('slog', 'c')

slog_headers = files([
  'src/slog.h',
])

slog_sources = files([
  'src/slog.c',
])

slog_sources += slog_headers
slog_includes = include_directories('src')


libslog_deps = [
  dependency('threads'),
]

libslog = static_library('slog',
  slog_sources,
  include_directories : slog_includes,
  dependencies : libslog_deps,
  install : false)

libslog_dep = declare_dependency(
  #sources : slog_sources,
  include_directories : slog_includes,
  dependencies : libslog_deps,
  link_with: libslog)

install_headers(slog_headers)
