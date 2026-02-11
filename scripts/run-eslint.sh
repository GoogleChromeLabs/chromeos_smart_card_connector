# Initializes NPM (i.e., creates package.json). The "--yes" flag is used to
# suppress interactive questions (about "package name", "version", etc.).
npm init --yes
# Not strictly necessary, but works as a smoke test that NPM works fine (and
# this package will be installed anyway when installing eslint later).
npm install js-yaml
log_message "npm was installed successfully."

# Initialize eslint
if npm list eslint >/dev/null; then
  log_message "eslint already present, skipping."
  return
fi
npm install eslint eslint-plugin-no-floating-promise
log_message "eslint was installed successfully."


./eslint.py $1
log_message "eslint ran successfully."
