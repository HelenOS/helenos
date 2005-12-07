#!/usr/bin/env python
"""
Kernel configuration script
"""
import sys
import os
import re

INPUT = 'kernel.config'
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


class Dialog(NoDialog):
    def __init__(self):
        NoDialog.__init__(self)
        self.dlgcmd = os.environ.get('DIALOG','dialog')
        self.title = ''
        self.backtitle = 'HelenOS Kernel Configuration'
        
        if os.system('%s --print-maxsize >/dev/null 2>&1' % self.dlgcmd) != 0:
            raise NotImplementedError

    def set_title(self,text):
        self.title = text
        
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
        errout = os.fdopen(indesc,'r')
        data = errout.read()
        errout.close()
            
        pid,status = os.wait()
        if not os.WIFEXITED(status):
            raise EOFError
        status = os.WEXITSTATUS(status)
        if status == 255:
            raise EOFError
        return status,data
        
    def yesno(self, text, default=None):
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

    def menu(self, text, choices, button, defopt=None):
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
        res,data = self.calldlg('--cancel-label',button[1],
                                '--menu',text,height,width,
                                str(len(choices)),*args,**kw)
        if res == 1:
            return button[0]
        elif res:
            print data
            raise EOFError
        return data
    
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
    
def read_defaults(fname,defaults):
    "Read saved values from last configuration run"
    f = file(fname,'r')
    for line in f:
        res = re.match(r'^(?:#!# )?([^#]\w*)\s*=\s*(.*?)\s*$', line)
        if res:
            defaults[res.group(1)] = res.group(2)
    f.close()

def check_condition(text, defaults):
    result = True
    conds = text.split('&')
    for cond in conds:
        if cond.startswith('(') and cond.endswith(')'):
            cond = cond[1:-1]
        if not check_dnf(cond, defaults):
            return False
    return True

def check_dnf(text, defaults):
    """
    Check that the condition specified on input line is True

    only CNF is supported
    """
    conds = text.split('|')
    for cond in conds:
        res = re.match(r'^(.*?)(!?=)(.*)$', cond)
        if not res:
            raise RuntimeError("Invalid condition: %s" % cond)
        condname = res.group(1)
        oper = res.group(2)
        condval = res.group(3)
        if not defaults.has_key(condname):
            raise RuntimeError("Condition var %s does not exist: %s" % \
                               (condname,text))

        if oper=='=' and  condval == defaults[condname]:
            return True
        if oper == '!=' and condval != defaults[condname]:
            return True
    return False

def parse_config(input, output, dlg, defaults={}, askonly=None):
    "Parse configuration file and create Makefile.config on the fly"
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
                if not check_condition(res.group(1), defaults):
                    continue
            args = res.group(2).strip().split(' ')
            cmd = args[0].lower()
            args = args[1:]
            if cmd == 'saveas':
                outf.write('%s = %s\n' % (args[1],defaults[args[0]]))
                
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
                if not check_condition(res.group(1), defaults):
                    if default is not None:
                        outf.write('#!# %s = %s\n' % (varname, default))
                    # Clear cumulated values
                    comment = ''
                    default = None
                    choices = []
                    continue
                
            asked_names.append((varname,comment))

            if default is not None and askonly and askonly != varname:
                outf.write('%s = %s\n' % (varname, default))
                continue

            if vartype == 'y/n':
                result = dlg.yesno(comment, default)
            elif vartype == 'n/y':
                result = dlg.noyes(comment, default)
            elif vartype == 'choice':
                defopt = None
                if default is not None:
                    for i,(key,val) in enumerate(choices):
                        if key == default:
                            defopt = i
                            break
                result = dlg.choice(comment, choices, defopt)
            else:
                raise RuntimeError("Bad method: %s" % vartype)
            outf.write('%s = %s\n' % (varname, result))
            # Remeber the selected value
            defaults[varname] = result
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
                if not check_condition(res.group(1),defaults):
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
        
    outf.close()
    f.close()
    return asked_names

def main():
    defaults = {}
    try:
        dlg = Dialog()
    except NotImplementedError:
        dlg = NoDialog()

    if len(sys.argv) == 2 and sys.argv[1]=='default':
        defmode = True
    else:
        defmode = False

    # Default run will update the configuration file
    # with newest options
    if os.path.exists(OUTPUT):
        read_defaults(OUTPUT, defaults)

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
        

if __name__ == '__main__':
    main()
