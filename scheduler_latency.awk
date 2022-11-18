# Generate ksoftirqd scheduling stats
# Usage:
#
# 1. Record traces, note that you must select a single core (-C)
# which is running network processing
#
#  $ perf record -e 'sched:sched_switch' -e 'sched:sched_wakeup' -C 3 sleep 60
#
# 2. Run this script
#
#  $ perf script -F -comm | awk -f ksoftirqd_sched_stats.awk
BEGIN {
	first = 1;
	wake = 0;
	woke = 0;
	woken = 0;
	wake_t = 0;
	out_t = 0;
	lat_sum = 0;
	lat_max = 0;
	long1 = 0;
	long2 = 0;
	long4 = 0;
	long8 = 0;
	resched = 0;
	rewake = 0;
	relat_sum = 0;
	relat_max = 0;
	to_run = 0;
	to_run_sing = 0;
	to_run_max = 0;
}
/sched:sched_wakeup: comm=ksoftirqd/ {
	wake++;
	if (!woke) {
		woke = 1;
		wake_t = $3;
	}
}
/next_comm=ksoftirqd/ {
	run++;
	if (to_run_max < to_run_sing) {
		to_run_max = to_run_sing;
		print "M",to_run_max,wake_t,$3;
	}
	to_run_sing = 0;
	if (woke) {
		woken++;
		lat = $3 - wake_t;
		lat_sum += lat;
		if (lat > lat_max) {
			lat_max = lat;
			print "[",lat,"]",wake_t,$3;
		}
		if (lat < 0)
			print lat, wake_t, $3;
		if (lat > 0.008)
			long8++;
		else if (lat > 0.004)
			long4++;
		else if (lat > 0.002)
			long2++;
		else if (lat > 0.001)
			long1++;
	} else if (out_t) {
		rewake++;
		lat = $3 - out_t;
		relat_sum += lat;
		if (lat > relat_max) {
			relat_max = lat;
			print "<",lat,">",out_t,$3;
		}
	} else {
		print "full miss?";
	}
}
/prev_comm=ksoftirqd.*prev_state=S/ {
	out_t = 0;
	woke = 0;
}
/prev_comm=ksoftirqd.*prev_state=R/ {
	resched++;
	out_t = $3;
	woke = 0;
}
/sched:sched_switch:/ {
	if (woke) {
		to_run ++;
		to_run_sing ++;
	}
}
{
	if (first) {
		start_t = $3;
		first = 0;
	}
	end_t = $3;
}
END {
	print "Wake ups:", wake, woken;
	print "Sched lat avg:", lat_sum / wake;
	print "Sched lat max:", lat_max;
	# Other processes got scheduled even tho ksoftirqd was pending
	print "Sched other:", (to_run - wake);
	print "Sched other avg:", (to_run - wake) / run;
	print "Sched other chain:", to_run_max;
	# Waits longer than X msec
	print "wait 1, 2:", long1;
	print "wait 2, 4:", long2;
	print "wait 4, 8:", long4;
	print "wait >= 8:", long8;
	# Ksoftirqd got rescheduled
	print "Resched:", resched, rewake;
	if (resched)
		print "RE avg lat:", relat_sum / resched;
	else
		print "RE avg lat:", 0;
	print "RE max lat:", relat_max;
	print "Trace time:", end_t - start_t, start_t, end_t;
}
