project('libtypec','c',
license: 'MIT',
version: '0.5.1',
default_options : [
	'warning_level=0'])

conf_data = configuration_data()
conf_data.set('libtypec_VERSION_MAJOR', '0')
conf_data.set('libtypec_VERSION_MINOR', '5')
conf_data.set('libtypec_VERSION_PATCH', '1')

libudev_dep = dependency('libudev', required: true)

configure_file(input : 'libtypec_config.h.in', output : 'libtypec_config.h', configuration : conf_data)

library('typec',
	'libtypec.c',
	'libtypec_sysfs_ops.c',
	'libtypec_dbgfs_ops.c',
	version : '0.5.1',
	soversion : '5',
	dependencies: libudev_dep,
)