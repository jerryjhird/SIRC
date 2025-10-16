project('sirc', 'c',
  version: '0.1',
  default_options: ['warning_level=2', 'c_std=c11']
)

incdir = include_directories('include')
event_dep = dependency('libevent_openssl', required: true)
openssl_dep = dependency('openssl', required: true)

executable('server',
  sources: ['src/server.c', 'src/util.c', 'src/secure.c', 'src/packets.c'],
  include_directories: incdir,
  dependencies: [event_dep, openssl_dep],
  install: true
)
