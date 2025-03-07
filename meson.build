project('libtypec','c',
        version: '0.6.0',
        license: 'MIT',
        default_options : ['warning_level=0']
        )

cc = meson.get_compiler('c')

conf_data = configuration_data()
split = meson.project_version().split('.')
conf_data.set('libtypec_VERSION_MAJOR', split[0])
conf_data.set('libtypec_VERSION_MINOR', split[1])
conf_data.set('libtypec_VERSION_PATCH', split[2])

libudev_dep = dependency('libudev', required: true)
pkg = import('pkgconfig')

configure_file(input : 'libtypec_config.h.in', output : 'libtypec_config.h', configuration : conf_data)

libtypec = library('typec',
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

pkg.generate(libtypec, filebase : 'libtypec')

if get_option('utils')
    subdir('utils')
endif
