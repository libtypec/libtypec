project('libtypec_utils','c',
license: 'MIT',
version: '0.5.1',
default_options : [
	'warning_level=0'])

conf_data = configuration_data()
split = meson.project_version().split('.')
conf_data.set('libtypec_utils_VERSION_MAJOR', split[0])
conf_data.set('libtypec_utils_VERSION_MINOR', split[1])
conf_data.set('libtypec_utils_VERSION_PATCH', split[2])

configure_file(input : 'libtypec_utils_config.h.in', output : 'libtypec_utils_config.h', configuration : conf_data)

cc = meson.get_compiler('c')

dep = declare_dependency(
  dependencies : cc.find_library('typec', dirs : [meson.current_source_dir()]),
  include_directories : include_directories('../../builddir/'),
)
udev_dep = meson.get_compiler('c').find_library('udev')

executable('lstypec', 'lstypec.c', 'names.c' ,dependencies : [dep,udev_dep])
executable('typecstatus', 'typecstatus.c', 'names.c',dependencies : [dep,udev_dep])
