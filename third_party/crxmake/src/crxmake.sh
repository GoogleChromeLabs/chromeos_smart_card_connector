#!/bin/bash -e
#
# Purpose: Pack a Chromium extension directory into crx format

if test $# -ne 2; then
  echo "Usage: crxmake.sh <extension dir> <pem path>"
  exit 1
fi

dir=$1
key=$2
name=$(basename "$dir")
crx="$name.crx"
pub="$name.pub"
sig="$name.sig"
zip="$name.zip"
id="$name.id"
payload="$name.payload"
trap 'rm -f "$pub" "$sig" "$zip" "$id" "$payload"' EXIT

# zip up the crx dir
cwd=$(pwd -P)
(cd "$dir" && zip -qr -9 -X "$cwd/$zip" .)

# generate crx id
openssl rsa -in "$key" -pubout -outform der | \
  openssl dgst -sha256 -binary -out "$id"
truncate -s 16 "$id"

# generate payload to be signed
(
  printf "CRX3 SignedData"
  echo "0012 0000 000A 10" | xxd -r -p # "\x00" + signed_header_size + header_of_SignedData_protobuf
  cat "$id"
  cat "$zip"
) > "$payload"

# signature
openssl dgst -sha256 -binary -sign "$key" < "$payload" > "$sig"

# public key
openssl rsa -pubout -outform der < "$key" > "$pub" 2>/dev/null

crmagic_hex="4372 3234" # Cr24
version_hex="0300 0000" # 3
header_1="4502 0000 12AC 040A A602" # header_length + header_of_CrxFileHeader + header_of_AsymmetricKeyProof + header_of_public_key
header_2="1280 02" # header_of_signature
header_3="82F1 0412 0A10" # header_of_signed_header_data
(
  echo "$crmagic_hex $version_hex $header_1" | xxd -r -p
  cat "$pub"
  echo "$header_2" | xxd -r -p
  cat "$sig"
  echo "$header_3" | xxd -r -p
  cat "$id"
  cat "$zip"
) > "$crx"
echo "Wrote $crx"
