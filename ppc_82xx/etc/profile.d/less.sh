# less initialization script (sh)
[ -x /usr/bin/lesspipe.sh ] && export LESSOPEN="|/usr/bin/lesspipe.sh %s"

if [ x`echo $LANG | cut -b 1-2` = x"ja" ]; then
	export JLESSCHARSET=japanese;
elif [ x`echo $LANG | cut -b 1-2` = x"ko" ]; then
	export JLESSCHARSET=ko;
fi

