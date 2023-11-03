#!/bin/bash

data_input=$1

# [ -n ${data_input} ] || echo "Usage: $1 <file>"; exit 1

# VSIZE
OLD_FILE_VSIZE=$( cat ${data_input} | grep 5.19 | awk -F, '{ sum += $3; n++ } END { if (n > 0) print sum / n; }')
NEW_FILE_VSIZE=$( cat ${data_input} | grep 6.4.3 | awk -F, '{ sum += $3; n++ } END { if (n > 0) print sum / n; }')

# RSS
OLD_FILE_RSS=$( cat ${data_input} | grep 5.19 | awk -F, '{ sum += $4; n++ } END { if (n > 0) print sum / n; }')
NEW_FILE_RSS=$( cat ${data_input} | grep 6.4.3 | awk -F, '{ sum += $4; n++ } END { if (n > 0) print sum / n; }')

# ANON
OLD_ANON_VSIZE=$( cat ${data_input} | grep 5.19 | awk -F, '{ sum += $7; n++ } END { if (n > 0) print sum / n; }')
NEW_ANON_VSIZE=$( cat ${data_input} | grep 6.4.3 | awk -F, '{ sum += $7; n++ } END { if (n > 0) print sum / n; }')

# RSS
OLD_ANON_RSS=$( cat ${data_input} | grep 5.19 | awk -F, '{ sum += $8; n++ } END { if (n > 0) print sum / n; }')
NEW_ANON_RSS=$( cat ${data_input} | grep 6.4.3 | awk -F, '{ sum += $8; n++ } END { if (n > 0) print sum / n; }')


echo FILE VSIZE:
echo -e 6.4\\t ${NEW_FILE_VSIZE}
echo -e 5.19\\t ${OLD_FILE_VSIZE}

echo FILE RSS:
echo -e 6.4\\t ${NEW_FILE_RSS}
echo -e 5.19\\t ${OLD_FILE_RSS}

echo ANON VSIZE:
echo -e 6.4\\t ${NEW_ANON_VSIZE}
echo -e 5.19\\t ${OLD_ANON_VSIZE}

echo ANON RSS:
echo -e 6.4\\t ${NEW_ANON_RSS}
echo -e 5.19\\t ${OLD_ANON_RSS}
