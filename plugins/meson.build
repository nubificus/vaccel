opt_plugin_noop = get_option('plugin-noop').enable_if(opt_tests.enabled())
opt_plugin_exec = get_option('plugin-exec')

if opt_plugin_noop.enabled()
  subdir('noop')
endif
if opt_plugin_exec.enabled()
  subdir('exec')
endif
