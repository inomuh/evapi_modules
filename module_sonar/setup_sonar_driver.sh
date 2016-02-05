#!/bin/sh
module="driver_sonar"
device="evarobotSonar"

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in .by default
/sbin/insmod /lib/modules/$(uname -r)/kernel/drivers/evarobot/$module.ko || exit 1 

# remove stale nodes
rm  /dev/${device}

#major=$(awk "\\$2==\"$module\" {print \\$1}" /proc/devices)
#major=$(awk -v module=$module '$2 == module { print $1 }' /proc/devices)
major=$(awk "\$2==\"$device\" {print \$1}" /proc/devices)


mknod /dev/${device} c ${major} 0

# give appropriate group/permissions, and change the group.
# Not all distributions have staff, some have "wheel" instead.

# group="staff"
# grep -q '^staff:' /etc/group || group="wheel"

# chgrp $group /dev/${device}
# chmod $mode /dev/${device}
