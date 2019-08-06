cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors
echo ondemand > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq

