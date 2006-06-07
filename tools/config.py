#!/usr/bin/env python
"""
User space configuration script
"""
import sys
import os
import re
import commands

INPUT = 'uspace.config'
OUTPUT = 'Makefile.config'
TMPOUTPUT = 'Makefile.config.tmp'

class DefaultDialog:
    "Wrapper dialog that tries to return default values"
    def __init__(self, dlg):
        self.dlg = dlg

    def set_title(self,text):
        self.dlg.set_title(text)
        
    def yesno(self, text, default=None):
        if default is not None:
            return default
        return self.dlg.yesno(text, default)
    def noyes(self, text, default=None):
        if default is not None:
            return default
        return self.dlg.noyes(text, default)
    
    def choice(self, text, choices, defopt=None):
        if defopt is not None:
            return choices[defopt][0]
        return self.dlg.choice(text, choices, defopt)

class NoDialog:
    def __init__(self):
        self.printed = None
        self.title = 'HelenOS Configuration'

    def print_title(self):
        if not self.printed:
            sys.stdout.write("\n*** %s ***\n" % self.title)
            self.printed = True

    def set_title(self, text):
        self.title = text
        self.printed = False
    
    def noyes(self, text, default=None):
        if not default:
            default = 'n'
        return self.yesno(text, default)
    
    def yesno(self, text, default=None):
        self.print_title()
        
        if default != 'n':
            default = 'y'
        while 1:
            sys.stdout.write("%s (y/n)[%s]: " % (text,default))
            inp = sys.stdin.readline()
            if not inp:
                raise EOFError
            inp = inp.strip().lower()
            if not inp:
                return default
            if inp == 'y':
                return 'y'
            elif inp == 'n':
                return 'n'

    def _print_choice(self, text, choices, defopt):
        sys.stdout.write('%s:\n' % text)
        for i,(text,descr) in enumerate(choices):
            sys.stdout.write('\t%2d. %s\n' % (i, descr))
        if defopt is not None:
            sys.stdout.write('Enter choice number[%d]: ' % defopt)
        else:
            sys.stdout.write('Enter choice number: ')

    def menu(self, text, choices, button, defopt=None):
        self.title = 'Main menu'
        menu = []
        for key, descr in choices:
            txt = key + (45-len(key))*' ' + ': ' + descr
            menu.append((key, txt))
            
        return self.choice(text, [button] + menu)
        
    def choice(self, text, choices, defopt=None):
        self.print_title()
        while 1:
            self._print_choice(text, choices, defopt)
            inp = sys.stdin.readline()
            if not inp:
                raise EOFError
            if not inp.strip():
                if defopt is not None:
                    return choices[defopt][0]
                continue
            try:
                number = int(inp.strip())
            except ValueError:
                continue
            if number < 0 or number >= len(choices):
                continue
            return choices[number][0]


def eof_checker(fnc):
    def wrapper(self, *args, **kw):
        try:
            return fnc(self, *args, **kw)
        except EOFError:
            return getattr(self.bckdialog,fnc.func_name)(*args, **kw)
    return wrapper

class Dialog(NoDialog):
    def __init__(self):
        NoDialog.__init__(self)
        self.dlgcmd = os.environ.get('DIALOG','dialog')
        self.title = ''
        self.backtitle = 'HelenOS Kernel Configuration'
        
        if os.system('%s --print-maxsize >/dev/null 2>&1' % self.dlgcmd) != 0:
            raise NotImplementedError
        
        self.bckdialog = NoDialog()

    def set_title(self,text):
        self.title = text
        self.bckdialog.set_title(text)
        
    def calldlg(self,*args,**kw):
        "Wrapper for calling 'dialog' program"
        indesc, outdesc = os.pipe()
        pid = os.fork()
        if not pid:
            os.close(2)
            os.dup(outdesc)
            os.close(indesc)
            
            dlgargs = [self.dlgcmd,'--title',self.title,
                       '--backtitle', self.backtitle]
            for key,val in kw.items():
                dlgargs.append('--'+key)
                dlgargs.append(val)
            dlgargs += args            
            os.execlp(self.dlgcmd,*dlgargs)

        os.close(outdesc)
        
        try:
            errout = os.fdopen(indesc,'r')
            data = errout.read()
            errout.close()
            pid,status = os.wait()
        except:
            os.system('reset') # Reset terminal
            raise
        
        if not os.WIFEXITED(status):
            os.system('reset') # Reset terminal
            raise EOFError
        
        status = os.WEXITSTATUS(status)
        if status == 255:
            raise EOFError
        return status,data
        
    def yesno(self, text, default=None):
        if text[-1] not in ('?',':'):
            text = text + ':'
        width = '50'
        height = '5'
        if len(text) < 48:
            text = ' '*int(((48-len(text))/2)) + text
        else:
            width = '0'
            height = '0'
        if default == 'n':
            res,data = self.calldlg('--defaultno','--yesno',text,height,width)
        else:
            res,data = self.calldlg('--yesno',text,height,width)

        if res == 0:
            return 'y'
        return 'n'
    yesno = eof_checker(yesno)

    def menu(self, text, choices, button, defopt=None):
        self.title = 'Main menu'
        text = text + ':'
        width = '70'
        height = str(8 + len(choices))
        args = []
        for key,val in choices:
            args.append(key)
            args.append(val)

        kw = {}
        if defopt:
            kw['default-item'] = choices[defopt][0] 
        res,data = self.calldlg('--ok-label','Change',
                                '--extra-label',button[1],
                                '--extra-button',
                                '--menu',text,height,width,
                                str(len(choices)),*args,**kw)
        if res == 3:
            return button[0]
        if res == 1: # Cancel
            sys.exit(1)
        elif res:
            print data
            raise EOFError
        return data
    menu = eof_checker(menu)
    
    def choice(self, text, choices, defopt=None):
        text = text + ':'
        width = '50'
        height = str(8 + len(choices))
        args = []
        for key,val in choices:
            args.append(key)
            args.append(val)

        kw = {}
        if defopt:
            kw['default-item'] = choices[defopt][0] 
        res,data = self.calldlg('--nocancel','--menu',text,height,width,
                                str(len(choices)),*args, **kw)
        if res:
            print data
            raise EOFError
        return data
    choice = eof_checker(choice)
    
def read_defaults(fname,defaults):
    "Read saved values from last configuration run"
    f = file(fname,'r')
    for line in f:
        res = re.match(r'^(?:#!# )?([^#]\w*)\s*=\s*(.*?)\s*$', line)
        if res:
            defaults[res.group(1)] = res.group(2)
    f.close()

def check_condition(text, defaults, asked_names):
    seen_vars = [ x[0] for x in asked_names ]
    ctype = 'cnf'
    if ')|' in text or '|(' in text:
        ctype = 'dnf'
    
    if ctype == 'cnf':
        conds = text.split('&')
    else:
        conds = text.split('|')

    for cond in conds:
        if cond.startswith('(') and cond.endswith(')'):
            cond = cond[1:-1]
            
        inside = check_inside(cond, defaults, ctype, seen_vars)
        
        if ctype == 'cnf' and not inside:
            return False
        if ctype == 'dnf' and inside:
            return True

    if ctype == 'cnf':
        return True
    return False

def check_inside(text, defaults, ctype, seen_vars):
    """
    Check that the condition specified on input line is True

    only CNF is supported
    """
    if ctype == 'cnf':
        conds = text.split('|')
    else:
        conds = text.split('&')
    for cond in conds:
        res = re.match(r'^(.*?)(!?=)(.*)$', cond)
        if not res:
            raise RuntimeError("Invalid condition: %s" % cond)
        condname = res.group(1)
        oper = res.group(2)
        condval = res.group(3)
        if condname not in seen_vars:
            varval = ''
##             raise RuntimeError("Variable %s not defined before being asked." %\
##                                condname)
        elif not defaults.has_key(condname):
            raise RuntimeError("Condition var %s does not exist: %s" % \
                               (condname,text))
        else:
            varval = defaults[condname]
        if ctype == 'cnf':
            if oper == '=' and  condval == varval:
                return True
            if oper == '!=' and condval != varval:
                return True
        else:
            if oper== '=' and condval != varval:
                return False
            if oper== '!=' and condval == varval:
                return False
    if ctype=='cnf':
        return False
    return True

def parse_config(input, output, dlg, defaults={}, askonly=None):
    "Parse configuration file and create Makefile.config on the fly"
    def ask_the_question(dialog):
        "Ask question based on the type of variables to ask"
        # This is quite a hack, this thingy is written just to
        # have access to local variables..
        if vartype == 'y/n':
            return dialog.yesno(comment, default)
        elif vartype == 'n/y':
            return dialog.noyes(comment, default)
        elif vartype == 'choice':
            defopt = None
            if default is not None:
                for i,(key,val) in enumerate(choices):
                    if key == default:
                        defopt = i
                        break
            return dialog.choice(comment, choices, defopt)
        else:
            raise RuntimeError("Bad method: %s" % vartype)

    
    f = file(input, 'r')
    outf = file(output, 'w')

    outf.write('#########################################\n')
    outf.write('## AUTO-GENERATED FILE, DO NOT EDIT!!! ##\n')
    outf.write('#########################################\n\n')

    asked_names = []

    comment = ''
    default = None
    choices = []
    for line in f:
        if line.startswith('%'):
            res = re.match(r'^%\s*(?:\[(.*?)\])?\s*(.*)$', line)
            if not res:
                raise RuntimeError('Invalid command: %s' % line)
            if res.group(1):
                if not check_condition(res.group(1), defaults,
                                       asked_names):
                    continue
            args = res.group(2).strip().split(' ')
            cmd = args[0].lower()
            args = args[1:]
            if cmd == 'saveas':
                outf.write('%s = %s\n' % (args[1],defaults[args[0]]))
            elif cmd == 'shellcmd':
                varname = args[0]
                args = args[1:]
                for i,arg in enumerate(args):
                    if arg.startswith('$'):
                        args[i] = defaults[arg[1:]]
                data,status = commands.getstatusoutput(' '.join(args))
                if status:
                    raise RuntimeError('Error running: %s' % ' '.join(args))
                outf.write('%s = %s\n' % (varname,data.strip()))
            continue
            
        if line.startswith('!'):
            # Ask a question
            res = re.search(r'!\s*(?:\[(.*?)\])?\s*([^\s]+)\s*\((.*)\)\s*$', line)
            if not res:
                raise RuntimeError("Weird line: %s" % line)
            varname = res.group(2)
            vartype = res.group(3)

            default = defaults.get(varname,None)
            
            if res.group(1):
                if not check_condition(res.group(1), defaults,
                                       asked_names):
                    if default is not None:
                        outf.write('#!# %s = %s\n' % (varname, default))
                    # Clear cumulated values
                    comment = ''
                    default = None
                    choices = []
                    continue
                
            asked_names.append((varname,comment))

            if default is None or not askonly or askonly == varname:
                default = ask_the_question(dlg)
            else:
                default = ask_the_question(DefaultDialog(dlg))

            outf.write('%s = %s\n' % (varname, default))
            # Remeber the selected value
            defaults[varname] = default
            # Clear cumulated values
            comment = ''
            default = None
            choices = []
            continue
        
        if line.startswith('@'):
            # Add new line into the 'choice array' 
            res = re.match(r'@\s*(?:\[(.*?)\])?\s*"(.*?)"\s*(.*)$', line)
            if not res:
                raise RuntimeError("Bad line: %s" % line)
            if res.group(1):
                if not check_condition(res.group(1),defaults,
                                       asked_names):
                    continue
            choices.append((res.group(2), res.group(3)))
            continue

        # All other things print to output file
        outf.write(line)
        if re.match(r'^#[^#]', line):
            # Last comment before question will be displayed to the user
            comment = line[1:].strip()
        elif line.startswith('## '):
            # Set title of the dialog window
            dlg.set_title(line[2:].strip())

    outf.write('\n')
    outf.write('REVISION = %s\n' % commands.getoutput('svnversion . 2> /dev/null'))
    outf.write('TIMESTAMP = %s\n' % commands.getoutput('date "+%Y-%m-%d %H:%M:%S"'))
    outf.close()
    f.close()
    return asked_names

def main():
    defaults = {}
    try:
        dlg = Dialog()
    except NotImplementedError:
        dlg = NoDialog()

    if len(sys.argv) >= 2 and sys.argv[1]=='default':
        defmode = True
    else:
        defmode = False

    # Default run will update the configuration file
    # with newest options
    if os.path.exists(OUTPUT):
        read_defaults(OUTPUT, defaults)

	# Get ARCH from command line if specified	
    if len(sys.argv) >= 3:
        defaults['ARCH'] = sys.argv[2]

    # Dry run only with defaults
    varnames = parse_config(INPUT, TMPOUTPUT, DefaultDialog(dlg), defaults)
    # If not in default mode, present selection of all possibilities
    if not defmode:
        defopt = 0
        while 1:
            # varnames contains variable names that were in the
            # last question set
            choices = [ (x[1],defaults[x[0]]) for x in varnames ]
            res = dlg.menu('Configuration',choices,('save','Save'),defopt)
            if res == 'save':
                parse_config(INPUT, TMPOUTPUT, DefaultDialog(dlg), defaults)
                break
            # transfer description back to varname
            for i,(vname,descr) in enumerate(varnames):
                if res == descr:
                    defopt = i
                    break
            # Ask the user a simple question, produce output
            # as if the user answered all the other questions
            # with default answer
            varnames = parse_config(INPUT, TMPOUTPUT, dlg, defaults,
                                    askonly=varnames[i][0])
        
    
    if os.path.exists(OUTPUT):
        os.unlink(OUTPUT)
    os.rename(TMPOUTPUT, OUTPUT)
    
    if not defmode and dlg.yesno('Rebuild user space?') == 'y':
        os.execlp('make','make','clean','build')

if __name__ == '__main__':
    main()
