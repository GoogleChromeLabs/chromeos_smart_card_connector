/*
 * Copyright (c) 1982, 1986, 1988, 1993
 *      The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2016 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <google_smart_card_common/logging/syslog/syslog.h>

#include <cstdarg>
#include <string>

#include <google_smart_card_common/formatting.h>
#include <google_smart_card_common/logging/logging.h>

namespace {

constexpr char kLoggingPrefix[] = "[syslog] ";

}  // namespace

void syslog(int priority, const char* format, ...) {
  va_list var_args;
  va_start(var_args, format);
  const std::string message =
      kLoggingPrefix +
      google_smart_card::FormatPrintfTemplate(format, var_args);
  va_end(var_args);

  switch (priority) {
    case LOG_EMERG:
    case LOG_ALERT:
    case LOG_CRIT:
    case LOG_ERR:
      GOOGLE_SMART_CARD_LOG_ERROR << message;
      return;
    case LOG_WARNING:
      GOOGLE_SMART_CARD_LOG_WARNING << message;
      return;
    case LOG_NOTICE:
    case LOG_INFO:
      GOOGLE_SMART_CARD_LOG_INFO << message;
      return;
    case LOG_DEBUG:
    default:
      GOOGLE_SMART_CARD_LOG_DEBUG << message;
      return;
  }
}
