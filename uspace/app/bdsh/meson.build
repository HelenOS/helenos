deps = [ 'clui', 'fmtutil' ]
includes += include_directories('.', 'cmds', 'cmds/builtins', 'cmds/modules')
src = files(
	'cmds/builtin_cmds.c',
	'cmds/builtins/batch/batch.c',
	'cmds/builtins/builtin_aliases.c',
	'cmds/builtins/builtins.c',
	'cmds/builtins/cd/cd.c',
	'cmds/builtins/exit/exit.c',
	'cmds/mod_cmds.c',
	'cmds/modules/alias/alias.c',
	'cmds/modules/cat/cat.c',
	'cmds/modules/cmp/cmp.c',
	'cmds/modules/cp/cp.c',
	'cmds/modules/echo/echo.c',
	'cmds/modules/help/help.c',
	'cmds/modules/kcon/kcon.c',
	'cmds/modules/ls/ls.c',
	'cmds/modules/mkdir/mkdir.c',
	'cmds/modules/mkfile/mkfile.c',
	'cmds/modules/module_aliases.c',
	'cmds/modules/modules.c',
	'cmds/modules/mount/mount.c',
	'cmds/modules/mv/mv.c',
	'cmds/modules/printf/printf.c',
	'cmds/modules/pwd/pwd.c',
	'cmds/modules/rm/rm.c',
	'cmds/modules/sleep/sleep.c',
	'cmds/modules/touch/touch.c',
	'cmds/modules/unalias/unalias.c',
	'cmds/modules/unmount/unmount.c',
	'compl.c',
	'errors.c',
	'exec.c',
	'input.c',
	'scli.c',
	'tok.c',
	'util.c',
)

test_src = files(
	'tok.c',
	'test/toktest.c',
)

# TODO: install this file somewhere sane
install_data('demo.txt', install_dir: '/')
