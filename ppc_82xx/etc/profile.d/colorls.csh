# color-ls initialization
set COLORS=/etc/DIR_COLORS
if ($?TERM) then
    test -e "/etc/DIR_COLORS.$TERM" && set COLORS="/etc/DIR_COLORS.$TERM"
endif
test -f ~/.dircolors && set COLORS=~/.dircolors
if ($?TERM) then
    test -f "~/.dircolors.$TERM" && set COLORS="~/.dircolors.$TERM"
endif
test -f ~/.dir_colors && set COLORS=~/.dir_colors
if ($?TERM) then
    test -f "~/.dir_colors.$TERM" && set COLORS="~/.dir_colors.$TERM"
endif

test -e "$COLORS" || exit

eval `dircolors -c $COLORS`

if ( "$LS_COLORS" == '' ) then
   exit
endif

egrep -qi "^COLOR.*none" $COLORS

if ( $? != 0 ) then
alias ll 'ls -l --color=tty'
alias l. 'ls -d .* --color=tty'
alias ls 'ls --color=tty'
else
alias ll 'ls -l'
alias l. 'ls -d .*'
endif
