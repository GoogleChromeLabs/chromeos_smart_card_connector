# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# *** sel_ldr tests for Perl5 ***
# A lot of the tests extensively use subprocesses, thus excluded
# TODO(agaurav77) : recheck tests for more potential sel_ldr tests

cd t
# all the directories whose tests we want to run
INCLUDE_DIRS="base cmd comp io lib op opbasic re"
# files to be tested inside the included folders
declare -a INCLUDE_FILES=(
  # base
  "cond if lex num pat rs while"
  # cmd
  "elsif for mod subval switch"
  # comp
  "bproto cmdopt decl final_line_num fold form_scope line_debug our \
   package_block package parser proto redef retainedlines term uproto \
   use utf"
  # io
  "crlf defout eintr_print errnosig iofile layers nargv perlio_fail \
   perlio_open perlio sem socket tell utf8"
  # lib
  "commonsense cygwin"
  # op
  "append args attrproto auto avhv bless bop chars chop chr context \
   cproto crypt current_sub dbm defins delete die dor each_array each \
   evalbytes exp fh filetest_stack_ok grent grep hashassign hashwarn \
   inccode incfilter index int join kvaslice kvhslice length list localref \
   lock lop negate not ord overload_integer pos postfixderef pow protowarn \
   push pwent quotemeta rand range read reverse rt119311 select signatures \
   sleep smartmatch sprintf state study studytied sub_lval svleak switch \
   symbolcache sysio tiearray tie_fetch_count tiehandle undef unlink upgrade \
   utf8magic utfhash vec ver waitpid wantarray yadayada"
  # opbasic
  "arith cmp concat qq"
  # re
  "charset no_utf8_pm qr_gc qrstack qr regexp_noamp regexp_notrie \
   regexp_qr_embed regexp_qr regexp regexp_trielist \
   regex_sets_compat reg_nc_tie reg_pmod rxcode"
  )

# how many tests have gone by? how many successful?
COUNT=0
SUCCESS=0
# which directory are we in?
ARRAY_IDX=0
for DIR in $INCLUDE_DIRS; do
  # enlist files in the directory FOLDER
  FILES=$(ls $DIR/)
  for FILE in $FILES; do
    # if FILE ends with .t and is not excluded, then do a test
    if [[ ${FILE:(-2)} == ".t" &&
          ${INCLUDE_FILES[$ARRAY_IDX]} == *${FILE::(-2)}* ]]; then
      # do the test
      OUTPUT=`../perl.sh ${DIR}/${FILE}`
      RE='^1..[0-9]+$'
      LIMITS=$(echo $OUTPUT | cut -d' ' -f 1)
      if ! [[ "${LIMITS}" =~ $RE ]]; then
        echo "${DIR}/${FILE} .. not ok"
        ((COUNT=COUNT+1))
        continue
      fi
      LOWER=$(echo $LIMITS | cut -d'.' -f 1)
      UPPER=$(echo $LIMITS | cut -d'.' -f 3)
      # echo "going from ${LOWER} to ${UPPER}"
      FOUND=0
      for ((i=LOWER;i<=UPPER;i++)); do
        if [[ "${OUTPUT}" == *"ok $i"* && "${OUTPUT}" != *"not ok $i"* ]]; then
          ((FOUND=FOUND+1))
        else
          break
        fi
      done
      if [[ "${FOUND}" == "${UPPER}" ]]; then
        echo "${DIR}/${FILE} .. ok"
        ((SUCCESS=SUCCESS+1))
      else
        echo "${DIR}/${FILE} .. not ok"
      fi
      ((COUNT=COUNT+1))
    fi
  done
  ((ARRAY_IDX=ARRAY_IDX+1))
done
# print results
echo "Scheduled $COUNT tests, passed $SUCCESS successfully."
echo "$((SUCCESS*100/COUNT))% okay."
