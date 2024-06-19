opt_plugins = get_option('plugins')
opt_plugin_noop = get_option('plugin-noop').enable_if(opt_tests.enabled()).enable_if(opt_plugins.enabled())
opt_plugin_exec = get_option('plugin-exec').enable_if(opt_tests.enabled()).enable_if(opt_plugins.enabled())
opt_plugin_mbench = get_option('plugin-mbench').enable_if(opt_plugins.enabled())

if opt_plugin_noop.enabled()
  subdir('noop')
endif
if opt_plugin_exec.enabled()
  subdir('exec')
endif
if opt_plugin_mbench.enabled()
  subdir('mbench')
endif
