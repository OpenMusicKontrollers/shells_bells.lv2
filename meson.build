project('shells_bells.lv2', 'c', default_options : [
	'buildtype=release',
	'warning_level=3',
	'werror=false',
	'b_lto=false',
	'c_std=gnu11'])

d2tk = subproject('d2tk')

lv2libdir = get_option('lv2libdir')
build_tests = get_option('build-tests')

inst_dir = join_paths(lv2libdir, meson.project_name())

if get_option('use-backend-nanovg').enabled()
	d2tk_dep = d2tk.get_variable('d2tk_nanovg')
elif get_option('use-backend-cairo').enabled()
	d2tk_dep = d2tk.get_variable('d2tk_cairo')
else
	error('no valid UI backend given')
endif

source_root = meson.source_root()
build_root = meson.build_root()

add_project_arguments('-D_GNU_SOURCE', language : 'c')

conf_data = configuration_data()
cc = meson.get_compiler('c')

lv2_validate = find_program('lv2_validate', native : true, required : false)
sord_validate = find_program('sord_validate', native : true, required : false)
lv2lint = find_program('lv2lint', required : false)

lv2_dep = dependency('lv2', version : '>=1.14.0')

dsp_deps = [lv2_dep]
ui_deps = [lv2_dep, d2tk_dep]

props_inc = include_directories('props.lv2')
ser_inc = include_directories('ser_atom.lv2')
d2tk_inc = include_directories(join_paths('subprojects', 'd2tk'))
inc_dir = [props_inc, ser_inc, d2tk_inc]

dsp_srcs = ['shells_bells.c']

ui_srcs = ['shells_bells_ui.c']

c_args = ['-fvisibility=hidden']

version = run_command('cat', 'VERSION').stdout().strip().split('.')
add_project_arguments('-DSHELLS_BELLS_VERSION="'
	+ version[0] + '.' + version[1] + '.' + version[2] + '"', language : 'c')
conf_data.set('MAJOR_VERSION', version[0])
conf_data.set('MINOR_VERSION', version[1])
conf_data.set('MICRO_VERSION', version[2])

mod = shared_module('shells_bells', dsp_srcs,
	c_args : c_args,
	include_directories : inc_dir,
	name_prefix : '',
	dependencies : dsp_deps,
	install : true,
	install_dir : inst_dir)

ui = shared_module('shells_bells_ui', ui_srcs,
	c_args : c_args,
	include_directories : inc_dir,
	name_prefix : '',
	dependencies : ui_deps,
	install : true,
	install_dir : inst_dir)

suffix = mod.full_path().strip().split('.')[-1]
conf_data.set('MODULE_SUFFIX', '.' + suffix)

manifest_ttl = configure_file(
	input : 'manifest.ttl.in',
	output : 'manifest.ttl',
	configuration : conf_data,
	install : true,
	install_dir : inst_dir)

dsp_ttl = configure_file(
	input : 'shells_bells.ttl.in',
	output : 'shells_bells.ttl',
	configuration : conf_data,
	install : true,
	install_dir : inst_dir)

ui_ttl = configure_file(
	input : 'shells_bells_ui.ttl',
	output : 'shells_bells_ui.ttl',
	copy: true,
	install : true,
	install_dir : inst_dir)

if build_tests
	if lv2_validate.found() and sord_validate.found()
		test('LV2 validate', lv2_validate,
			args : [manifest_ttl, dsp_ttl, ui_ttl])
	endif

	if lv2lint.found()
		test('LV2 lint', lv2lint,
			args : ['-M', 'pack',
				'-E', 'warn',
				'-I', join_paths(build_root, ''),
				'http://open-music-kontrollers.ch/lv2/shells_bells#bells'
			])
	endif
endif
