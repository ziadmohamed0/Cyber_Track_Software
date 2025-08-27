savedcmd_/home/pi/Device_Driver/serdev_echo.mod := printf '%s\n'   serdev_echo.o | awk '!x[$$0]++ { print("/home/pi/Device_Driver/"$$0) }' > /home/pi/Device_Driver/serdev_echo.mod
