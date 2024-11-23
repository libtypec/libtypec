project('libtypec','c',
license: 'MIT',
version: '0.6.0',
default_options : [
	'warning_level=0'])

conf_data = configuration_data()
split = meson.project_version().split('.')
conf_data.set('libtypec_VERSION_MAJOR', split[0])
conf_data.set('libtypec_VERSION_MINOR', split[1])
conf_data.set('libtypec_VERSION_PATCH', split[2])

libudev_dep = dependency('libudev', required: true)

configure_file(input : 'libtypec_config.h.in', output : 'libtypec_config.h', configuration : conf_data)

library('typec',
	'libtypec.c',
	'libtypec_sysfs_ops.c',
	'libtypec_dbgfs_ops.c',
	version : meson.project_version(),
	soversion : '0',
	dependencies: libudev_dep,
	install: true,
)

install_headers(
	'libtypec.h',
	meson.current_build_dir() + '/libtypec_config.h',
	install_dir: 'include'
)