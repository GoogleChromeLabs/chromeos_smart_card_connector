#!/bin/bash

# This is ld substitution which makes possible to process all checks during
# configuration process inside native client untrusted code. To use it you have
# to make some changes in your environmentals:
# 1) export CFLAGS+=" -B<path to the directory containing this file>".
# 2) Put path to ld from native client toolchain in REAL_LD variable.
# 3) Export NACL_LOADER variable to point to a Native Client loader (sel_ldr).

$REAL_LD "$@"
if [ "$?" = "0" ] ; then
  outfile=a.out
  W=0
  for out ; do
    if [ "$W" = "1" ] ; then
      W=0
      outfile="$out"
    elif [ "$out" = "-o" ] ; then
      W=1
    fi
  done
  if [ "${outfile:0:1}" != "/" ] ; then
    outfile="`pwd`/$outfile"
  fi
  /bin/mv -f "$outfile" "$outfile.nexe"
  echo "#!/bin/bash
( NACL_DANGEROUS_ENABLE_FILE_ACCESS=1 "$NACL_LOADER " \"$outfile\".nexe \"\$@\" 3>&2 2>&1 1>&3- | grep -v '] BYPASSING ALL ACL CHECKS' | grep -v '] DANGER: ENABLED FILE ACCESS' ; exit \${PIPESTATUS[0]} ) 3>&2 2>&1 1>&3-" > "$outfile"
  chmod a+x "$outfile"
  exit 0
fi
