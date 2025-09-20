cc -D_POSIX_C_SOURCE=199309L -Wall -Werror -Wno-error=unused-but-set-variable -Wno-error=switch -Wno-switch -Wpedantic -ansi -g \
    -Iextern/bluey -Iextern/lucid -Iextern/metrify\
    -o serpent \
    serpent.c \
    extern/bluey/bluey.c \
    extern/bluey/bluey_utils.c \
    extern/lucid/lucid.c \
    extern/metrify/metrify.c
