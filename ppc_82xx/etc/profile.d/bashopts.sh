# Edit bash settings
if echo $SHELL |grep -q bash; then
       if [ -z "$NO_BASH_SETTINGS" ]; then
              export CDPATH=.:~:/:/usr/src/rpm
               shopt -s cdspell
       fi
fi
