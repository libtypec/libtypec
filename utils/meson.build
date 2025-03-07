configure_file(input : 'libtypec_utils_config.h.in', output : 'libtypec_utils_config.h', configuration : conf_data)

# Include current build dir for the above generated file
inc_dir = include_directories('.')
# Include to find library includes
inc_dir = include_directories('..')

cc = meson.get_compiler('c')

udev_dep = meson.get_compiler('c').find_library('udev')
gtk3_dep = dependency('gtk+-3.0')

executable(
	'lstypec',
	'lstypec.c', 'names.c',
	link_with: libtypec,
	dependencies: [udev_dep],
	include_directories: inc_dir,
	install: true,
	install_dir: get_option('bindir')
)
executable(
	'typecstatus',
	'typecstatus.c', 'names.c',
	link_with: libtypec,
	dependencies: [udev_dep],
	include_directories: inc_dir,
	install: true,
	install_dir: get_option('bindir')
)
executable(
	'ucsicontrol',
	'ucsicontrol.c', 'names.c',
	link_with: libtypec,
	dependencies: [udev_dep],
	include_directories: inc_dir,
	install: true,
	install_dir: get_option('bindir')
)
executable(
	'usbcview',
	'usbcview.c', 'names.c',
	link_with: libtypec,
	dependencies: [gtk3_dep,udev_dep],
	include_directories: inc_dir,
	install: true,
	install_dir: get_option('bindir')
)
