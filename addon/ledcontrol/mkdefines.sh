#!/bin/bash

usage="Usage: $0 <system-map> <relprog>"

# Check for the required path to the System.map file

sysmap=$1
if [ -z "$sysmap" ]; then
  echo "ERROR: Missing <system-map>"
  echo ""
  echo $usage
  exit 1
fi

# Check for the required partially linked file

relprog=$2
if [ -z "$relprog" ]; then
  echo "ERROR: Missing <program-list>"
  echo ""
  echo $usage
  exit 1
fi

# Verify the System.map and the partially linked file

if [ ! -r "$sysmap" ]; then
  echo "ERROR:  $sysmap does not exist"
  echo ""
  echo $usage
  exit 1
fi

if [ ! -r "$relprog" ]; then
  echo "ERROR:  $relprog does not exist"
  echo ""
  echo $usage
  exit 1
fi

# Extract all of the undefined symbols from the partially linked file and create a
# list of sorted, unique undefined variable names.

varlist=`nm $relprog | fgrep ' U ' | sed -e "s/^[ ]*//g" | cut -d' ' -f2 | sort - | uniq`

# Now output the linker script that provides a value for all of the undefined symbols

for var in $varlist; do
  map=`grep " ${var}$" ${sysmap}`
  if [ -z "$map" ]; then
    echo "ERROR:  Variable $var not found in $sysmap"
    echo ""
    echo $usage
    exit 1
  fi

  varaddr=`echo ${map} | cut -d' ' -f1`
  echo "${var} = 0x${varaddr} | 0x00000001;"
done