#!/usr/bin/env bash
# Run the ASan-instrumented mythfrontend once, saving output to a timestamped
# log under /tmp/mf_asan_runs/. Reproduce the HDMI switch-away-then-back while
# it's running, then let it crash (or Ctrl-C it after a minute or two if it
# doesn't). Reports whether this run captured an AddressSanitizer report.
set -u
cd /home/donald/src/mythtv36-fork/build-qt6/MythTV-prefix/src/MythTV-build || exit 1

mkdir -p /tmp/mf_asan_runs
logfile="/tmp/mf_asan_runs/run_$(date +%Y%m%d_%H%M%S).log"
echo "Logging to: $logfile"
echo "Reproduce the HDMI switch now. Ctrl-C to stop once it crashes or after a while with no crash."

ASAN_OPTIONS=detect_leaks=0:detect_odr_violation=0 \
    ./programs/mythfrontend/mythfrontend --loglevel debug > "$logfile" 2>&1

if grep -q "ERROR: AddressSanitizer" "$logfile"; then
    echo "*** CAPTURED: AddressSanitizer report found in $logfile ***"
    grep -n "ERROR: AddressSanitizer" "$logfile"
else
    echo "No AddressSanitizer report in this run ($logfile). Try again."
fi
